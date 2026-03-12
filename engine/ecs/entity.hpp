#pragma once

#include <cstdint>
#include <limits>

class Entity
{
public:
    using ID = uint32_t;

    Entity() : m_id(INVALID_ID) {}
    explicit Entity(ID id) : m_id(id) {}

    ID GetID() const { return m_id; }
    bool IsValid() const { return m_id != INVALID_ID; }

    bool operator==(const Entity &other) const { return m_id == other.m_id; }
    bool operator!=(const Entity &other) const { return m_id != other.m_id; }
    bool operator<(const Entity &other) const { return m_id < other.m_id; }

    static constexpr ID INVALID_ID = std::numeric_limits<ID>::max();

private:
    ID m_id;
};

namespace std
{
    // to use in map/set as key
    template <>
    struct hash<Entity>
    {
        std::size_t operator()(const Entity &entity) const
        {
            return std::hash<Entity::ID>()(entity.GetID());
        }
    };
}
