#include "resource_manager.hpp"

#include <thread>

namespace resources
{
    ResourceManager::ResourceManager(size_t numLoaderThreads) : m_modelPool(256), m_shaderPool(64), m_texturePool(256), m_materialPool(128)
    {
        m_activeThreadDebugStates.resize(numLoaderThreads);

        for (size_t i = 0; i < numLoaderThreads; ++i)
        {
            m_loaderThreads.emplace_back([this, i]()
                                         { LoaderThreadMain(i); });
        }
    }

    ResourceManager::~ResourceManager()
    {
        Shutdown();
    }

    void ResourceManager::LoaderThreadMain(size_t threadIndex)
    {
        while (true)
        {
            LoadJob job;

            {
                std::unique_lock<std::mutex> lock(m_jobQueueMutex);

                // Wait for jobs or shutdown signal
                m_jobCV.wait(lock, [this]()
                             { return !m_loadJobs.empty() || m_shutdown; });

                if (m_shutdown && m_loadJobs.empty())
                {
                    break;
                }

                if (m_loadJobs.empty())
                {
                    continue;
                }

                m_activeLoaderCount.fetch_add(1, std::memory_order_acq_rel);
                job = std::move(m_loadJobs.front());
                m_loadJobs.pop();
            }

            MarkJobStarted(threadIndex, job);

            bool succeeded = false;

            try
            {
                succeeded = job.execute();
            }
            catch (const std::exception &)
            {
                succeeded = false;
            }

            MarkJobFinished(threadIndex, succeeded);
            m_activeLoaderCount.fetch_sub(1, std::memory_order_acq_rel);
            m_loaderIdleCV.notify_all();
        }
    }

    ResourceManager::DebugSnapshot ResourceManager::GetDebugSnapshot() const noexcept
    {
        DebugSnapshot snapshot;

        {
            std::lock_guard<std::mutex> lock(m_debugMutex);
            snapshot.queuedItems.assign(m_debugQueuedJobs.begin(), m_debugQueuedJobs.end());
            snapshot.recentHistory.assign(m_debugRecentHistory.begin(), m_debugRecentHistory.end());
            snapshot.threads.reserve(m_activeThreadDebugStates.size());
            snapshot.aggregateStats.reserve(m_debugLoadAggregates.size());

            const auto now = std::chrono::steady_clock::now();
            for (size_t index = 0; index < m_activeThreadDebugStates.size(); ++index)
            {
                const ActiveThreadDebugState &activeState = m_activeThreadDebugStates[index];

                DebugThreadState threadState;
                threadState.threadIndex = index;
                threadState.active = activeState.active;
                threadState.current.jobId = activeState.jobId;
                threadState.current.resourceType = activeState.resourceType;
                threadState.current.path = activeState.path;
                threadState.current.stage = activeState.stage;
                threadState.current.progress = activeState.progress;

                if (activeState.active)
                {
                    threadState.current.elapsedMs = std::chrono::duration<double, std::milli>(now - activeState.startedAt).count();
                    ++snapshot.activeThreadCount;
                }

                snapshot.threads.push_back(std::move(threadState));
            }

            snapshot.queuedJobCount = m_debugQueuedJobCount;
            snapshot.completedJobCount = m_debugCompletedJobCount;
            snapshot.failedJobCount = m_debugFailedJobCount;

            for (const auto &[key, aggregate] : m_debugLoadAggregates)
            {
                (void)key;
                DebugLoadAggregate exported;
                exported.resourceType = aggregate.resourceType;
                exported.path = aggregate.path;
                exported.sampleCount = aggregate.sampleCount;
                exported.successCount = aggregate.successCount;
                exported.failureCount = aggregate.failureCount;
                exported.avgMs = aggregate.sampleCount > 0 ? (aggregate.totalMs / static_cast<double>(aggregate.sampleCount)) : 0.0;
                exported.minMs = aggregate.minMs;
                exported.maxMs = aggregate.maxMs;
                exported.lastMs = aggregate.lastMs;
                snapshot.aggregateStats.push_back(std::move(exported));
            }
        }

        {
            std::lock_guard<std::mutex> lock(m_gpuUploadMutex);
            snapshot.pendingGpuUploadCount = m_pendingGPUUploads.size();
        }

        {
            std::lock_guard<std::mutex> lock(m_deletionMutex);
            snapshot.pendingDeleteCount = m_pendingDeletions.size();
        }

        return snapshot;
    }

    void ResourceManager::MarkJobStarted(size_t threadIndex, const LoadJob &job)
    {
        std::lock_guard<std::mutex> lock(m_debugMutex);

        auto queuedIt = std::find_if(
            m_debugQueuedJobs.begin(),
            m_debugQueuedJobs.end(),
            [&job](const DebugLoadEntry &entry)
            {
                return entry.jobId == job.jobId;
            });

        if (queuedIt != m_debugQueuedJobs.end())
        {
            m_debugQueuedJobs.erase(queuedIt);
        }

        if (threadIndex >= m_activeThreadDebugStates.size())
        {
            return;
        }

        ActiveThreadDebugState &threadState = m_activeThreadDebugStates[threadIndex];
        threadState.active = true;
        threadState.jobId = job.jobId;
        threadState.resourceType = job.resourceType;
        threadState.path = job.path;
        threadState.stage = "CPU load";
        threadState.progress = 0.5f;
        threadState.startedAt = std::chrono::steady_clock::now();
    }

    void ResourceManager::MarkJobFinished(size_t threadIndex, bool succeeded)
    {
        std::lock_guard<std::mutex> lock(m_debugMutex);

        if (threadIndex >= m_activeThreadDebugStates.size())
        {
            return;
        }

        ActiveThreadDebugState &threadState = m_activeThreadDebugStates[threadIndex];
        const double elapsedMs = std::chrono::duration<double, std::milli>(
                                     std::chrono::steady_clock::now() - threadState.startedAt)
                                     .count();

        DebugLoadEntry historyEntry;
        historyEntry.jobId = threadState.jobId;
        historyEntry.resourceType = threadState.resourceType;
        historyEntry.path = threadState.path;
        historyEntry.stage = succeeded ? "Completed" : "Failed";
        historyEntry.progress = succeeded ? 1.0f : 0.0f;
        historyEntry.elapsedMs = elapsedMs;
        m_debugRecentHistory.push_front(std::move(historyEntry));
        if (m_debugRecentHistory.size() > DEBUG_HISTORY_LIMIT)
        {
            m_debugRecentHistory.pop_back();
        }

        const std::string aggregateKey = threadState.resourceType + ":" + threadState.path;
        auto aggregateIt = m_debugLoadAggregates.find(aggregateKey);
        if (aggregateIt == m_debugLoadAggregates.end())
        {
            DebugLoadAggregateInternal aggregate;
            aggregate.resourceType = threadState.resourceType;
            aggregate.path = threadState.path;
            aggregate.sampleCount = 1;
            aggregate.successCount = succeeded ? 1 : 0;
            aggregate.failureCount = succeeded ? 0 : 1;
            aggregate.totalMs = elapsedMs;
            aggregate.minMs = elapsedMs;
            aggregate.maxMs = elapsedMs;
            aggregate.lastMs = elapsedMs;
            m_debugLoadAggregates.emplace(aggregateKey, std::move(aggregate));
        }
        else
        {
            DebugLoadAggregateInternal &aggregate = aggregateIt->second;
            ++aggregate.sampleCount;
            if (succeeded)
            {
                ++aggregate.successCount;
            }
            else
            {
                ++aggregate.failureCount;
            }

            aggregate.totalMs += elapsedMs;
            aggregate.minMs = std::min(aggregate.minMs, elapsedMs);
            aggregate.maxMs = std::max(aggregate.maxMs, elapsedMs);
            aggregate.lastMs = elapsedMs;
        }

        if (succeeded)
        {
            ++m_debugCompletedJobCount;
        }
        else
        {
            ++m_debugFailedJobCount;
        }

        threadState.active = false;
        threadState.jobId = 0;
        threadState.resourceType.clear();
        threadState.path.clear();
        threadState.stage.clear();
        threadState.progress = 0.0f;
        threadState.startedAt = {};
    }

    void ResourceManager::ProcessGPUUploads()
    {
        std::queue<Resource *> pendingUploads;
        GPUUploadCallback gpuUploadCallback;

        {
            std::lock_guard<std::mutex> lock(m_gpuUploadMutex);
            pendingUploads.swap(m_pendingGPUUploads);
        }

        {
            std::lock_guard<std::mutex> callbackLock(m_callbackMutex);
            gpuUploadCallback = m_gpuUploadCallback;
        }

        if (!gpuUploadCallback)
        {
            return;
        }

        while (!pendingUploads.empty())
        {
            Resource *resource = pendingUploads.front();
            pendingUploads.pop();

            if (resource)
            {
                try
                {
                    gpuUploadCallback(resource);
                }
                catch (const std::exception &e)
                {
                    WARNING("GPU upload failed for resource '" << resource->GetPath() << "': " << e.what());
                }
                catch (...)
                {
                    WARNING("GPU upload failed for resource '" << resource->GetPath() << "' with unknown error");
                }
            }
        }
    }

    void ResourceManager::ProcessGarbageCollection()
    {
        ++m_frameIndex;

        std::lock_guard<std::mutex> lock(m_deletionMutex);

        // Remove entries that are past the grace period
        m_pendingDeletions.erase(
            std::remove_if(
                m_pendingDeletions.begin(),
                m_pendingDeletions.end(),
                [this](const PendingDeletion &entry)
                {
                    if (m_frameIndex - entry.frameIndex >= DELETION_GRACE_PERIOD)
                    {
                        {
                            std::lock_guard<std::mutex> callbackLock(m_callbackMutex);
                            if (m_gpuReleaseCallback && entry.resource)
                            {
                                m_gpuReleaseCallback(entry.resource);
                            }
                        }

                        entry.deleter();
                        return true;
                    }
                    return false;
                }),
            m_pendingDeletions.end());
    }

    bool ResourceManager::AreLoadersIdle()
    {
        std::lock_guard<std::mutex> queueLock(m_jobQueueMutex);
        return m_loadJobs.empty() && m_activeLoaderCount.load(std::memory_order_acquire) == 0;
    }

    void ResourceManager::ClearDebugLoadHistory() noexcept
    {
        std::lock_guard<std::mutex> lock(m_debugMutex);
        m_debugRecentHistory.clear();
        m_debugLoadAggregates.clear();
    }

    bool ResourceManager::ResetForCleanReload()
    {
        constexpr auto kLoaderDrainTimeout = std::chrono::seconds(2);
        {
            std::unique_lock<std::mutex> queueLock(m_jobQueueMutex);
            const auto deadline = std::chrono::steady_clock::now() + kLoaderDrainTimeout;
            const bool drained = m_loaderIdleCV.wait_until(
                queueLock,
                deadline,
                [this]()
                {
                    return m_loadJobs.empty() && m_activeLoaderCount.load(std::memory_order_acquire) == 0;
                });

            if (!drained)
            {
                WARNING("ResetForCleanReload timed out waiting for loader threads to become idle");
                return false;
            }

            while (!m_loadJobs.empty())
            {
                m_loadJobs.pop();
            }
        }

        {
            std::lock_guard<std::mutex> debugLock(m_debugMutex);
            m_debugQueuedJobs.clear();
        }

        {
            std::lock_guard<std::mutex> uploadLock(m_gpuUploadMutex);
            while (!m_pendingGPUUploads.empty())
            {
                m_pendingGPUUploads.pop();
            }
        }

        GPUReleaseCallback gpuReleaseCallback;
        {
            std::lock_guard<std::mutex> callbackLock(m_callbackMutex);
            gpuReleaseCallback = m_gpuReleaseCallback;
        }

        auto releaseResources = [&gpuReleaseCallback](auto &resourceList)
        {
            if (!gpuReleaseCallback)
            {
                return;
            }

            for (auto &resource : resourceList)
            {
                if (resource)
                {
                    gpuReleaseCallback(resource.get());
                }
            }
        };

        auto modelResources = m_modelPool.ExtractAll();
        auto shaderResources = m_shaderPool.ExtractAll();
        auto textureResources = m_texturePool.ExtractAll();
        auto materialResources = m_materialPool.ExtractAll();

        releaseResources(modelResources);
        releaseResources(shaderResources);
        releaseResources(textureResources);
        releaseResources(materialResources);

        {
            std::lock_guard<std::mutex> deletionLock(m_deletionMutex);
            m_pendingDeletions.clear();
            m_frameIndex = 0;
        }

        {
            std::lock_guard<std::mutex> pathLock(m_pathMapMutex);
            m_pathToHandle.clear();
        }

        return true;
    }

    void ResourceManager::Shutdown()
    {
        {
            std::lock_guard<std::mutex> lock(m_jobQueueMutex);
            m_shutdown = true;
        }
        m_jobCV.notify_all();

        // Wait for all loader threads to finish
        for (auto &thread : m_loaderThreads)
        {
            if (thread.joinable())
            {
                thread.join();
            }
        }

        // Clear all resources
        m_modelPool.Clear();
        m_shaderPool.Clear();
        m_texturePool.Clear();
        m_materialPool.Clear();

        {
            std::lock_guard<std::mutex> lock(m_pathMapMutex);
            m_pathToHandle.clear();
        }

        {
            std::lock_guard<std::mutex> lock(m_gpuUploadMutex);
            while (!m_pendingGPUUploads.empty())
            {
                m_pendingGPUUploads.pop();
            }
        }

        {
            std::lock_guard<std::mutex> lock(m_deletionMutex);
            m_pendingDeletions.clear();
        }

        {
            std::lock_guard<std::mutex> lock(m_callbackMutex);
            m_gpuUploadCallback = nullptr;
            m_gpuReleaseCallback = nullptr;
        }

        {
            std::lock_guard<std::mutex> lock(m_debugMutex);
            m_debugQueuedJobs.clear();
            m_debugRecentHistory.clear();
            m_debugLoadAggregates.clear();
            m_activeThreadDebugStates.clear();
            m_nextDebugJobId = 1;
            m_debugQueuedJobCount = 0;
            m_debugCompletedJobCount = 0;
            m_debugFailedJobCount = 0;
        }
    }

    void ResourceManager::ClearPathCache()
    {
        std::lock_guard<std::mutex> lock(m_pathMapMutex);
        m_pathToHandle.clear();
    }

} // namespace resources
