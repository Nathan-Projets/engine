#pragma once

#include <vector>
#include <memory>
#include <stdexcept>
#include <mutex>
#include <limits>
#include <cstddef>

#include "handle.hpp"

namespace resources
{

    /**
     * @template ResourcePool
     * @brief Type-safe pool for managing resources of a specific type
     *
     * Provides efficient allocation/deallocation with handle-based access.
     * Uses a free-list for reusing slots and ensures handles remain stable.
     *
     * @tparam T The resource type to pool
     */
    template <typename T>
    class ResourcePool
    {
    public:
        ResourcePool(size_t initialCapacity = 128) : m_lastID(0)
        {
            m_resources.reserve(initialCapacity);
        }

        ~ResourcePool() = default;

        // Delete copy operations - pools are managed centrally (in singleton)
        ResourcePool(const ResourcePool &) = delete;
        ResourcePool &operator=(const ResourcePool &) = delete;

        /**
         * @brief Allocate a new resource slot
         * @return Handle to the allocated resource
         */
        Handle<T> Allocate()
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            uint32_t id;
            size_t index;

            if (!m_freeList.empty())
            {
                index = m_freeList.back();
                m_freeList.pop_back();
                id = m_idMap[index];
            }
            else
            {
                id = ++m_lastID;
                index = m_resources.size();
                m_resources.push_back(nullptr);
                m_idMap.push_back(id);
            }

            Handle<T> handle;
            handle.id = id;
            return handle;
        }

        /**
         * @brief Store a resource at the given handle
         * @param handle The handle allocated via Allocate()
         * @param resource The resource to store
         */
        void Store(Handle<T> handle, std::unique_ptr<T> resource)
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            size_t index = FindIndex(handle);
            if (index == std::numeric_limits<size_t>::max())
            {
                throw std::invalid_argument("Invalid handle");
            }

            m_resources[index] = std::move(resource);
        }

        /**
         * @brief Get a resource by handle
         * @param handle The resource handle
         * @return Pointer to the resource, or nullptr if invalid/unallocated
         */
        T *Get(Handle<T> handle) noexcept
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            size_t index = FindIndex(handle);
            if (index == std::numeric_limits<size_t>::max())
            {
                return nullptr;
            }

            return m_resources[index].get();
        }

        /**
         * @brief Get a const resource by handle
         * @param handle The resource handle
         * @return Const pointer to the resource, or nullptr if invalid/unallocated
         */
        const T *Get(Handle<T> handle) const noexcept
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            size_t index = FindIndex(handle);
            if (index == std::numeric_limits<size_t>::max())
            {
                return nullptr;
            }

            return m_resources[index].get();
        }

        /**
         * @brief Deallocate a resource
         * @param handle The handle to deallocate
         */
        void Deallocate(Handle<T> handle) noexcept
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            size_t index = FindIndex(handle);
            if (index == std::numeric_limits<size_t>::max())
            {
                return;
            }

            m_resources[index].reset();
            m_freeList.push_back(index);
        }

        /**
         * @brief Check if a handle is valid and allocated
         */
        bool IsValid(Handle<T> handle) const noexcept
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return FindIndex(handle) != std::numeric_limits<size_t>::max();
        }

        /**
         * @brief Get the current size of the pool
         */
        size_t Size() const noexcept
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_resources.size() - m_freeList.size();
        }

        /**
         * @brief Get the capacity of the pool
         */
        size_t Capacity() const noexcept
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_resources.capacity();
        }

        /**
         * @brief Clear all resources
         */
        void Clear() noexcept
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_resources.clear();
            m_idMap.clear();
            m_freeList.clear();
            m_lastID = 0;
        }

        /**
         * @brief Move all currently allocated resources out of the pool and reset it.
         */
        std::vector<std::unique_ptr<T>> ExtractAll() noexcept
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            std::vector<std::unique_ptr<T>> extracted;
            extracted.reserve(m_resources.size());
            for (std::unique_ptr<T> &resource : m_resources)
            {
                if (resource)
                {
                    extracted.push_back(std::move(resource));
                }
            }

            m_resources.clear();
            m_idMap.clear();
            m_freeList.clear();
            m_lastID = 0;

            return extracted;
        }

    private:
        /**
         * @brief Find the index of a resource by its handle
         * @return Index if found, std::numeric_limits<size_t>::max() if not found
         */
        size_t FindIndex(Handle<T> handle) const noexcept
        {
            if (!handle.IsValid())
            {
                return std::numeric_limits<size_t>::max();
            }

            for (size_t i = 0; i < m_idMap.size(); ++i)
            {
                if (m_idMap[i] == handle.id)
                {
                    return i;
                }
            }

            return std::numeric_limits<size_t>::max();
        }

        std::vector<std::unique_ptr<T>> m_resources;
        std::vector<uint32_t> m_idMap; // Maps pool indices to handle IDs
        std::vector<size_t> m_freeList;
        uint32_t m_lastID;
        mutable std::mutex m_mutex;
    };
}
