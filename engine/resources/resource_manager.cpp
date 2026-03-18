#include "resource_manager.hpp"

namespace resources
{
    ResourceManager::ResourceManager(size_t numLoaderThreads) : m_meshPool(256), m_shaderPool(64), m_texturePool(256), m_materialPool(128)
    {
        for (size_t i = 0; i < numLoaderThreads; ++i)
        {
            m_loaderThreads.emplace_back([this]()
                                         { LoaderThreadMain(); });
        }
    }

    ResourceManager::~ResourceManager()
    {
        Shutdown();
    }

    void ResourceManager::LoaderThreadMain()
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

                job = std::move(m_loadJobs.front());
                m_loadJobs.pop();
            }

            try
            {
                job.execute();
            }
            catch (const std::exception &)
            {
                // Errors already handled in EnqueueLoadJob
            }
        }
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
                gpuUploadCallback(resource);
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
        m_meshPool.Clear();
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
    }

} // namespace resources
