#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include <stdexcept>

class IRegistry
{
public:
    virtual ~IRegistry() = default;
    virtual void CollectGarbage() = 0;
};

template <typename T>
class ResourceRegistry : public IRegistry
{
public:
    struct ResourceEntry
    {
        std::shared_ptr<T> Ref;
    };

    std::unordered_map<std::string, ResourceEntry> m_resources;

    explicit ResourceRegistry() {}

    template <typename... Args>
    std::shared_ptr<T> Register(const std::string &iId, Args &&...iArgs)
    {
        auto aResourceIterator = m_resources.find(iId);
        if (aResourceIterator != m_resources.end())
        {
            return aResourceIterator->second.Ref;
        }

        std::shared_ptr<T> aResource = std::make_shared<T>(std::forward<Args>(iArgs)...);

        m_resources[iId] = ResourceEntry{aResource};

        return aResource;
    }

    /**
     * Unregister a resource from the registry by its ID.
     * If the resource is held somewhere else, it will not delete the resource in memory until no one else holds a reference to it.
     */
    void Unregister(const std::string &iId)
    {
        m_resources.erase(iId);
    }

    void Clear()
    {
        m_resources.clear();
    }

    void CollectGarbage() override
    {
        for (auto aResourceIterator = m_resources.begin(); aResourceIterator != m_resources.end();)
        {
            if (aResourceIterator->second.Ref.use_count() == 0)
            {
                aResourceIterator = m_resources.erase(aResourceIterator);
            }
            else
            {
                ++aResourceIterator;
            }
        }
    }

    std::shared_ptr<T> Get(const std::string &iId) const
    {
        auto aResourceIterator = m_resources.find(iId);
        if (aResourceIterator != m_resources.end())
        {
            return aResourceIterator->second.Ref;
        }
        throw std::runtime_error("Resource not found: " + iId);
    }

    std::shared_ptr<T> operator[](const std::string &iId) const
    {
        return Get(iId);
    }
};