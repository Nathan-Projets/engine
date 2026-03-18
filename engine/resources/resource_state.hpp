#pragma once

namespace resources
{
    /**
     * @enum ResourceState
     * @brief Represents the lifecycle state of a resource
     */
    enum class ResourceState
    {
        Unloaded,
        Loading,
        Loaded,
        Failed
    };

}
