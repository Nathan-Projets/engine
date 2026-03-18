#pragma once

#include <atomic>
#include <cstdint>
#include <string>

#include "resource_state.hpp"

namespace resources
{

    /**
     * @class Resource
     * @brief Base class for all resources
     *
     * Provides common functionality for resource lifetime management, including:
     * - Unique ID tracking
     * - Reference counting
     * - State tracking (loading, loaded, failed)
     * - Path tracking for hot-reload and deduplication
     */
    class Resource
    {
    public:
        friend class ResourceManager;

        virtual ~Resource() = default;

        uint32_t GetID() const noexcept { return m_id; }
        const std::string &GetPath() const noexcept { return m_path; }

        ResourceState GetState() const noexcept
        {
            return m_state.load(std::memory_order_acquire);
        }

        void SetState(ResourceState state) noexcept
        {
            m_state.store(state, std::memory_order_release);
        }

        bool IsLoaded() const noexcept
        {
            return GetState() == ResourceState::Loaded;
        }

        bool IsLoading() const noexcept
        {
            return GetState() == ResourceState::Loading;
        }

        bool HasFailed() const noexcept
        {
            return GetState() == ResourceState::Failed;
        }

        uint32_t AcquireRef() noexcept
        {
            return m_refCount.fetch_add(1, std::memory_order_acq_rel) + 1; // equivalent to m_refCount++ without atomic
        }

        uint32_t ReleaseRef() noexcept
        {
            return m_refCount.fetch_sub(1, std::memory_order_acq_rel) - 1; // equivalent to --m_refCount without atomic
        }

        uint32_t GetRefCount() const noexcept
        {
            return m_refCount.load(std::memory_order_acquire);
        }

    protected:
        Resource(uint32_t id, const std::string &path = "") : m_id(id), m_path(path), m_state(ResourceState::Unloaded), m_refCount(0) {}

        void SetID(uint32_t id) noexcept
        {
            m_id = id;
        }

    private:
        uint32_t m_id;
        std::string m_path;
        std::atomic<ResourceState> m_state;
        std::atomic<uint32_t> m_refCount;
    };
}
