#pragma once

#include <functional>

template <typename T>
struct Handle
{
    unsigned int id = 0;

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

    constexpr Handle operator++() noexcept
    {
        ++id;
        return *this;
    }

    constexpr Handle operator++(int) noexcept
    {
        Handle result = *this;
        ++id;
        return result;
    }

    constexpr Handle operator--() noexcept
    {
        --id;
        return *this;
    }

    constexpr Handle operator--(int) noexcept
    {
        Handle result = *this;
        --id;
        return result;
    }

    constexpr bool IsValid() const noexcept
    {
        return id != 0;
    }
};

template <typename T>
struct std::hash<Handle<T>>
{
    std::size_t operator()(const Handle<T> &handle) const noexcept
    {
        return std::hash<uint32_t>()(handle.id);
    }
};