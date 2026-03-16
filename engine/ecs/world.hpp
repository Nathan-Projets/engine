#pragma once

#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <typeindex>
#include <stdexcept>
#include <vector>

#include "entity.hpp"
#include "component.hpp"
#include "system.hpp"

class World
{
public:
    World() : m_nextEntityID(0) {}
    ~World() = default;

    World(const World &) = delete;
    World &operator=(const World &) = delete;
    World(World &&) noexcept = default;
    World &operator=(World &&) noexcept = default;

    // === Entity Management ===

    Entity CreateEntity()
    {
        Entity entity(m_nextEntityID++);
        m_activeEntities.insert(entity);
        return entity;
    }

    void DestroyEntity(Entity entity)
    {
        if (!m_activeEntities.count(entity))
            return;

        // Remove all components for this entity
        for (auto it = m_components.begin(); it != m_components.end();)
        {
            it->second.erase(entity);
            if (it->second.empty())
            {
                it = m_components.erase(it);
            }
            else
            {
                ++it;
            }
        }

        m_activeEntities.erase(entity);
    }

    bool IsEntityActive(Entity entity) const
    {
        return m_activeEntities.count(entity) > 0;
    }

    // === Component Management ===

    template <typename T, typename... Args>
    T *AddComponent(Entity entity, Args &&...args)
    {
        if (!IsEntityActive(entity))
            throw std::runtime_error("Entity is not active");

        std::type_index typeIndex(typeid(T));

        auto component = std::make_unique<T>(std::forward<Args>(args)...);
        T *componentPtr = component.get();

        componentPtr->OnAttach(entity);

        // Store in registry
        m_components[typeIndex][entity] = std::move(component);

        return componentPtr;
    }

    template <typename T>
    T *GetComponent(Entity entity)
    {
        std::type_index typeIndex(typeid(T));

        auto typeIt = m_components.find(typeIndex);
        if (typeIt == m_components.end())
            return nullptr;

        auto entityIt = typeIt->second.find(entity);
        if (entityIt == typeIt->second.end())
            return nullptr;

        return static_cast<T *>(entityIt->second.get());
    }

    template <typename T>
    const T *GetComponent(Entity entity) const
    {
        std::type_index typeIndex(typeid(T));

        auto typeIt = m_components.find(typeIndex);
        if (typeIt == m_components.end())
            return nullptr;

        auto entityIt = typeIt->second.find(entity);
        if (entityIt == typeIt->second.end())
            return nullptr;

        return static_cast<const T *>(entityIt->second.get());
    }

    template <typename T>
    bool HasComponent(Entity entity) const
    {
        return GetComponent<T>(entity) != nullptr;
    }

    template <typename T>
    bool RemoveComponent(Entity entity)
    {
        std::type_index typeIndex(typeid(T));

        auto typeIt = m_components.find(typeIndex);
        if (typeIt == m_components.end())
            return false;

        auto entityIt = typeIt->second.find(entity);
        if (entityIt == typeIt->second.end())
            return false;

        if (auto *component = static_cast<T *>(entityIt->second.get()))
            component->OnDetach(entity);

        typeIt->second.erase(entityIt);

        if (typeIt->second.empty())
            m_components.erase(typeIt);

        return true;
    }

    // === System Management ===

    template <typename T, typename... Args>
    T *AddSystem(Args &&...args)
    {
        auto system = std::make_unique<T>(std::forward<Args>(args)...);
        T *systemPtr = system.get();
        m_systems.push_back(std::move(system));
        return systemPtr;
    }

    void UpdateSystems(float deltaTime)
    {
        for (auto &system : m_systems)
        {
            system->Update(*this, deltaTime);
        }
    }

    void RenderSystems(float deltaTime)
    {
        for (auto &system : m_systems)
        {
            system->Render(*this, deltaTime);
        }
    }

    // === Queries ===

    const std::unordered_set<Entity> &GetAllEntities() const
    {
        return m_activeEntities;
    }

    // Get all entities with a specific component
    template <typename T>
    std::vector<Entity> GetEntitiesWith() const
    {
        std::vector<Entity> result;
        std::type_index typeIndex(typeid(T));

        auto it = m_components.find(typeIndex);
        if (it != m_components.end())
        {
            for (const auto &[entity, _] : it->second)
            {
                result.push_back(entity);
            }
        }

        return result;
    }

    // Get all entities with multiple components
    template <typename T, typename U, typename... Rest>
    std::vector<Entity> GetEntitiesWith() const
    {
        auto results = GetEntitiesWith<T>();
        auto others = GetEntitiesWith<U, Rest...>();

        std::vector<Entity> intersection;
        for (const auto &entity : results)
        {
            if (std::find(others.begin(), others.end(), entity) != others.end())
            {
                intersection.push_back(entity);
            }
        }

        return intersection;
    }

private:
    Entity::ID m_nextEntityID;
    std::unordered_set<Entity> m_activeEntities;

    // Component storage: type → (entity → component)
    std::unordered_map<
        std::type_index,
        std::unordered_map<Entity, std::unique_ptr<Component>>>
        m_components;

    std::vector<std::unique_ptr<System>> m_systems;
};
