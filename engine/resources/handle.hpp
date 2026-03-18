#pragma once

#include <cstdint>
#include <functional>

namespace resources
{
    /**
     * @template Handle
     * @brief Type-safe handle to a resource
     *
     * Provides indirection between user code and actual resource data.
     * This enables hot reloading and safe resource management without exposing raw pointers.
     *
     * @tparam T The resource type this handle refers to
     */
    template <typename T>
    struct Handle
    {
        uint32_t id = 0;

        constexpr bool operator==(const Handle &other) const noexcept
        {
            return id == other.id;
        }

        constexpr bool operator!=(const Handle &other) const noexcept
        {
            return id != other.id;
        }

        constexpr explicit operator bool() const noexcept
        {
            return id != 0;
        }

        constexpr bool IsValid() const noexcept
        {
            return id != 0;
        }
    };

}

/**
 * @brief Hash specialization for Handle to allow use in unordered containers
 */
template <typename T>
struct std::hash<resources::Handle<T>>
{
    std::size_t operator()(const resources::Handle<T> &handle) const noexcept
    {
        return std::hash<uint32_t>()(handle.id);
    }
};
