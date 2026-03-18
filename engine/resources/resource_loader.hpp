#pragma once

#include <string>
#include <memory>

#include "resource.hpp"

namespace resources
{

    /**
     * @template ResourceLoader
     * @brief Base template for type-specific resource loaders
     *
     * Specializations of this template provide the loading logic for specific resource types.
     *
     * @tparam T The resource type being loaded
     */
    template <typename T>
    struct ResourceLoader
    {
        /**
         * @brief Load a resource from disk/memory
         * @param path The path/identifier of the resource
         * @return A unique_ptr to the newly created resource
         */
        static std::unique_ptr<T> Load(const std::string &path);
    };
}
