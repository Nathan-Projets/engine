#pragma once

#include "handle.hpp"
#include "resource.hpp"
#include "resource_pool.hpp"
#include "resource_loader.hpp"

#include "units/mesh.hpp"
#include "units/model.hpp"
#include "units/shader.hpp"
#include "units/texture.hpp"
#include "units/material.hpp"

#include "loaders/model_loader.hpp"
#include "loaders/shader_loader.hpp"
#include "loaders/texture_loader.hpp"
#include "loaders/material_loader.hpp"

#include "../helpers/log.hpp"

#include <unordered_map>
#include <memory>
#include <thread>
#include <iostream>
#include <queue>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <stdexcept>
#include <algorithm>
#include <deque>
#include <atomic>
#include <typeinfo>
#include <type_traits>

namespace resources
{

    template <typename>
    inline constexpr bool always_false_v = false;

    /**
     * @class ResourceManager
     * @brief Central hub for all resource management
     *
     * Responsibilities:
     * - Centralized resource creation and lifetime tracking
     * - Async loading with worker threads
     * - Deduplication (same path = same handle)
     * - Reference counting and garbage collection
     * - Two-phase loading (CPU thread + Render thread callbacks)
     */
    class ResourceManager
    {
    public:
        using GPUUploadCallback = std::function<void(Resource *)>;
        using GPUReleaseCallback = std::function<void(Resource *)>;

        struct DebugLoadEntry
        {
            uint64_t jobId = 0;
            std::string resourceType;
            std::string path;
            std::string stage;
            float progress = 0.0f;
            double elapsedMs = 0.0;
        };

        struct DebugThreadState
        {
            size_t threadIndex = 0;
            bool active = false;
            DebugLoadEntry current;
        };

        struct DebugLoadAggregate
        {
            std::string resourceType;
            std::string path;
            uint64_t sampleCount = 0;
            uint64_t successCount = 0;
            uint64_t failureCount = 0;
            double avgMs = 0.0;
            double minMs = 0.0;
            double maxMs = 0.0;
            double lastMs = 0.0;
        };

        struct DebugSnapshot
        {
            std::vector<DebugThreadState> threads;
            std::vector<DebugLoadEntry> queuedItems;
            std::vector<DebugLoadEntry> recentHistory;
            std::vector<DebugLoadAggregate> aggregateStats;
            size_t pendingGpuUploadCount = 0;
            size_t pendingDeleteCount = 0;
            size_t activeThreadCount = 0;
            uint64_t queuedJobCount = 0;
            uint64_t completedJobCount = 0;
            uint64_t failedJobCount = 0;
        };

        explicit ResourceManager(size_t numLoaderThreads = 2);
        ~ResourceManager();

        ResourceManager(const ResourceManager &) = delete;
        ResourceManager &operator=(const ResourceManager &) = delete;
        ResourceManager(ResourceManager &&) = delete;
        ResourceManager &operator=(ResourceManager &&) = delete;

        /**
         * @brief Load a resource asynchronously
         *
         * If the resource is already loaded or loading, returns the existing handle.
         * Returns immediately; actual loading happens on worker thread.
         *
         * @tparam T The resource type
         * @param path The resource path/identifier
         * @return Handle to the resource
         */
        template <typename T>
        Handle<T> Load(const std::string &path)
        {
            static_assert(std::is_base_of_v<Resource, T>, "T must derive from Resource");

            std::lock_guard<std::mutex> lock(m_pathMapMutex);

            // Check if already loaded/loading
            auto key = MakePathKey<T>(path);
            auto it = m_pathToHandle.find(key);
            if (it != m_pathToHandle.end())
            {
                Handle<T> handle;
                handle.id = it->second;
                return handle;
            }

            Handle<T> handle = GetPool<T>().Allocate();

            m_pathToHandle[key] = handle.id;

            EnqueueLoadJob<T>(handle, path);

            return handle;
        }

        /**
         * @brief Get a loaded resource by handle
         *
         * @tparam T The resource type
         * @param handle The resource handle
         * @return Pointer to the resource, or nullptr if invalid/not loaded
         */
        template <typename T>
        T *Get(Handle<T> handle) noexcept
        {
            static_assert(std::is_base_of_v<Resource, T>, "T must derive from Resource");
            return GetPool<T>().Get(handle);
        }

        /**
         * @brief Get a const resource by handle
         */
        template <typename T>
        const T *Get(Handle<T> handle) const noexcept
        {
            static_assert(std::is_base_of_v<Resource, T>, "T must derive from Resource");
            return GetPool<T>().Get(handle);
        }

        /**
         * @brief Acquire a reference to a resource (increments ref count)
         */
        template <typename T>
        void Acquire(Handle<T> handle) noexcept
        {
            if (auto res = Get(handle))
            {
                res->AcquireRef();
            }
        }

        /**
         * @brief Release a reference to a resource
         *
         * Decrements reference count. Resource may be garbage collected after a grace period to ensure GPU safety.
         */
        template <typename T>
        void Release(Handle<T> handle) noexcept
        {
            if (auto res = Get(handle))
            {
                uint32_t refCount = res->ReleaseRef();
                if (refCount == 0)
                {
                    MarkForDeletion(handle);
                }
            }
        }

        /**
         * @brief Check if a handle is valid
         */
        template <typename T>
        bool IsValid(Handle<T> handle) const noexcept
        {
            return Get(handle) != nullptr;
        }

        /**
         * @brief Register a GPU upload callback for async GPU work
         *
         * Called from the main render thread after CPU loading completes.
         * This allows GPU work (buffer creation, etc) on the render thread.
         */
        void SetGPUUploadCallback(GPUUploadCallback callback) noexcept
        {
            std::lock_guard<std::mutex> lock(m_callbackMutex);
            m_gpuUploadCallback = callback;
        }

        void SetGPUReleaseCallback(GPUReleaseCallback callback) noexcept
        {
            std::lock_guard<std::mutex> lock(m_callbackMutex);
            m_gpuReleaseCallback = callback;
        }

        /**
         * @brief Process pending GPU uploads
         *
         * Call this from your render thread to handle GPU work for newly loaded resources.
         */
        void ProcessGPUUploads();

        /**
         * @brief Perform garbage collection
         *
         * Deletes resources that have been marked for deletion and are past the grace period.
         * Call this periodically (e.g., once per frame).
         */
        void ProcessGarbageCollection();

        /**
         * @brief Clear all resources and stop loader threads
         */
        void Shutdown();

        /**
         * @brief Reset resource manager state for scene hot-reload.
         *
         * Waits for loader activity to settle, drops queued work, releases GPU resources,
         * and clears pools and caches so the next scene load starts from a cold state.
         */
        bool ResetForCleanReload();

        /**
         * @brief Clear type+path cache so subsequent Load() calls enqueue fresh jobs.
         *
         * Existing resources already stored in pools are not destroyed by this call.
         */
        void ClearPathCache();

        /****************************
         ********** Getters *********
         ****************************/

        template <typename T>
        size_t GetPoolSize() noexcept
        {
            return GetPool<T>().Size();
        }

        size_t GetPendingLoadCount() noexcept
        {
            std::lock_guard<std::mutex> lock(m_jobQueueMutex);
            return m_loadJobs.size();
        }

        size_t GetPendingDeleteCount() noexcept
        {
            std::lock_guard<std::mutex> lock(m_deletionMutex);
            return m_pendingDeletions.size();
        }

        DebugSnapshot GetDebugSnapshot() const noexcept;
        void ClearDebugLoadHistory() noexcept;

    private:
        struct LoadJob
        {
            std::function<bool()> execute;
            std::function<void(Resource *)> onComplete;
            uint64_t jobId = 0;
            std::string resourceType;
            std::string path;
        };

        struct ActiveThreadDebugState
        {
            bool active = false;
            uint64_t jobId = 0;
            std::string resourceType;
            std::string path;
            std::string stage;
            float progress = 0.0f;
            std::chrono::steady_clock::time_point startedAt = {};
        };

        struct DebugLoadAggregateInternal
        {
            std::string resourceType;
            std::string path;
            uint64_t sampleCount = 0;
            uint64_t successCount = 0;
            uint64_t failureCount = 0;
            double totalMs = 0.0;
            double minMs = 0.0;
            double maxMs = 0.0;
            double lastMs = 0.0;
        };

        // Deferred deletion entry
        struct PendingDeletion
        {
            std::function<void()> deleter;
            uint32_t frameIndex;
            Resource *resource = nullptr;
        };

        // Helper to create path key for type+path
        // Example: key = "class resources::Shader:assets/default.shader"
        template <typename T>
        static std::string MakePathKey(const std::string &path)
        {
            return typeid(T).name() + std::string(":") + path;
        }

        template <typename T>
        static const char *GetResourceTypeName()
        {
            if constexpr (std::is_same_v<T, Model>)
            {
                return "Model";
            }
            else if constexpr (std::is_same_v<T, Shader>)
            {
                return "Shader";
            }
            else if constexpr (std::is_same_v<T, Texture>)
            {
                return "Texture";
            }
            else if constexpr (std::is_same_v<T, Material>)
            {
                return "Material";
            }
            else
            {
                static_assert(always_false_v<T>, "Unsupported resource type");
            }
        }

        template <typename T>
        ResourcePool<T> &GetPool()
        {
            if constexpr (std::is_same_v<T, Model>)
            {
                return m_modelPool;
            }
            else if constexpr (std::is_same_v<T, Shader>)
            {
                return m_shaderPool;
            }
            else if constexpr (std::is_same_v<T, Texture>)
            {
                return m_texturePool;
            }
            else if constexpr (std::is_same_v<T, Material>)
            {
                return m_materialPool;
            }
            else
            {
                static_assert(always_false_v<T>, "Unsupported resource type");
            }
        }

        template <typename T>
        const ResourcePool<T> &GetPool() const
        {
            if constexpr (std::is_same_v<T, Model>)
            {
                return m_modelPool;
            }
            else if constexpr (std::is_same_v<T, Shader>)
            {
                return m_shaderPool;
            }
            else if constexpr (std::is_same_v<T, Texture>)
            {
                return m_texturePool;
            }
            else if constexpr (std::is_same_v<T, Material>)
            {
                return m_materialPool;
            }
            else
            {
                static_assert(always_false_v<T>, "Unsupported resource type");
            }
        }

        template <typename T>
        void EnqueueLoadJob(Handle<T> handle, const std::string &path)
        {
            LoadJob job;
            job.resourceType = GetResourceTypeName<T>();
            job.path = path;

            {
                std::lock_guard<std::mutex> debugLock(m_debugMutex);
                job.jobId = m_nextDebugJobId++;
                m_debugQueuedJobs.push_back({job.jobId, job.resourceType, job.path, "Queued", 0.0f, 0.0});
                ++m_debugQueuedJobCount;
            }

            job.execute = [this, handle, path]()
            {
                try
                {
                    auto resource = ResourceLoader<T>::Load(path);
                    resource->SetID(handle.id);
                    resource->SetState(ResourceState::Loaded);

                    GetPool<T>().Store(handle, std::move(resource));

                    // Queue for GPU upload
                    {
                        std::lock_guard<std::mutex> lock(m_gpuUploadMutex);
                        m_pendingGPUUploads.push(GetPool<T>().Get(handle));
                    }

                    return true;
                }
                catch (const std::exception &e)
                {
                    WARNING("Failed to load resource '" << path << "' of type '" << typeid(T).name() << "': " << e.what());

                    {
                        const std::string key = MakePathKey<T>(path);
                        std::lock_guard<std::mutex> pathLock(m_pathMapMutex);
                        auto it = m_pathToHandle.find(key);
                        if (it != m_pathToHandle.end() && it->second == handle.id)
                        {
                            m_pathToHandle.erase(it); // remove empty slot
                        }
                    }

                    GetPool<T>().Deallocate(handle);
                    return false;
                }
            };

            {
                std::lock_guard<std::mutex> lock(m_jobQueueMutex);
                m_loadJobs.push(std::move(job));
            }
            m_jobCV.notify_one();
        }

        template <typename T>
        void MarkForDeletion(Handle<T> handle) noexcept
        {
            if (auto res = GetPool<T>().Get(handle))
            {
                std::lock_guard<std::mutex> lock(m_deletionMutex);
                m_pendingDeletions.push_back({[this, handle]()
                                              { GetPool<T>().Deallocate(handle); },
                                              m_frameIndex,
                                              res});
            }
        }

        void LoaderThreadMain(size_t threadIndex);
        void MarkJobStarted(size_t threadIndex, const LoadJob &job);
        void MarkJobFinished(size_t threadIndex, bool succeeded);
        bool AreLoadersIdle();

        // Thread pool
        std::vector<std::thread> m_loaderThreads;
        bool m_shutdown = false;

        // Resource pools
        ResourcePool<Model> m_modelPool;
        ResourcePool<Shader> m_shaderPool;
        ResourcePool<Texture> m_texturePool;
        ResourcePool<Material> m_materialPool;

        std::unordered_map<std::string, uint32_t> m_pathToHandle; // key -> handle id
        mutable std::mutex m_pathMapMutex;

        // Load job queue
        std::queue<LoadJob> m_loadJobs;
        std::mutex m_jobQueueMutex;
        std::condition_variable m_jobCV;
        std::condition_variable m_loaderIdleCV;
        std::atomic<size_t> m_activeLoaderCount{0};

        // Debug state for async loading
        mutable std::mutex m_debugMutex;
        std::deque<DebugLoadEntry> m_debugQueuedJobs;
        std::deque<DebugLoadEntry> m_debugRecentHistory;
        std::unordered_map<std::string, DebugLoadAggregateInternal> m_debugLoadAggregates;
        std::vector<ActiveThreadDebugState> m_activeThreadDebugStates;
        uint64_t m_nextDebugJobId = 1;
        uint64_t m_debugQueuedJobCount = 0;
        uint64_t m_debugCompletedJobCount = 0;
        uint64_t m_debugFailedJobCount = 0;
        static constexpr size_t DEBUG_HISTORY_LIMIT = 512;

        // GPU uploads pending
        std::queue<Resource *> m_pendingGPUUploads;
        mutable std::mutex m_gpuUploadMutex;

        // Deferred deletions
        std::deque<PendingDeletion> m_pendingDeletions;
        mutable std::mutex m_deletionMutex;
        uint32_t m_frameIndex = 0;
        static constexpr uint32_t DELETION_GRACE_PERIOD = 2; // frames

        // GPU upload callback
        GPUUploadCallback m_gpuUploadCallback;
        GPUReleaseCallback m_gpuReleaseCallback;
        std::mutex m_callbackMutex;
    };

}
