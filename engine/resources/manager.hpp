#pragma once

#include <unordered_map>
#include <stdexcept>
#include <memory>
#include <typeindex>
#include <typeinfo>

#include "registry.hpp"

class ResourceManager
{
public:
    template <typename T, typename... Args>
    std::shared_ptr<T> Load(const std::string &iId, Args &&...iArgs)
    {
        auto aRegistryIterator = m_registries.find(typeid(T));
        if (aRegistryIterator == m_registries.end())
        {
            m_registries[typeid(T)] = std::make_unique<ResourceRegistry<T>>();
            ResourceRegistry<T> *aRegistry = static_cast<ResourceRegistry<T> *>(m_registries[typeid(T)].get());
            return aRegistry->Register(iId, std::forward<Args>(iArgs)...);
        }
        else
        {
            ResourceRegistry<T> *aRegistry = static_cast<ResourceRegistry<T> *>(aRegistryIterator->second.get());
            return aRegistry->Register(iId, std::forward<Args>(iArgs)...);
        }
    }

    template <typename T>
    void Unload(const std::string &iId)
    {
        auto aRegistryIterator = m_registries.find(typeid(T));
        if (aRegistryIterator != m_registries.end())
        {
            ResourceRegistry<T> *aRegistry = static_cast<ResourceRegistry<T> *>(aRegistryIterator->second.get());
            aRegistry->Unload(iId);
        }
    }

    template <typename T>
    void Clear()
    {
        auto aRegistryIterator = m_registries.find(typeid(T));
        if (aRegistryIterator != m_registries.end())
        {
            ResourceRegistry<T> *aRegistry = static_cast<ResourceRegistry<T> *>(aRegistryIterator->second.get());
            aRegistry->Clear();
        }
    }

    void CollectGarbage()
    {
        for (auto &[_, aRegistry] : m_registries)
        {
            aRegistry->CollectGarbage();
        }
    }

    void ClearAll()
    {
        m_registries.clear();
    }

    template <typename T>
    std::shared_ptr<T> Get(const std::string &iId) const
    {
        auto aRegistryIterator = m_registries.find(typeid(T));
        if (aRegistryIterator != m_registries.end())
        {
            ResourceRegistry<T> *aRegistry = static_cast<ResourceRegistry<T> *>(aRegistryIterator->second.get());
            return aRegistry->Get(iId);
        }
        throw std::runtime_error("Registry not found: " + std::string(typeid(T).name()));
    }

    template <typename T>
    std::shared_ptr<T> operator[](const std::string &iId) const
    {
        return Get(iId);
    }

private:
    std::unordered_map<std::type_index, std::unique_ptr<IRegistry>> m_registries;
};