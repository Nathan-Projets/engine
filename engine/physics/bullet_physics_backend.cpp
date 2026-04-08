#include "physics_world.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <unordered_map>

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <btBulletDynamicsCommon.h>

namespace
{
    constexpr float kMinColliderExtent = 0.001f;
    constexpr float kMaxAngularSpeedRadians = 8.0f;
    constexpr float kComparisonEpsilon = 0.0001f;
    constexpr float kShapeMargin = 0.002f;
    constexpr float kSleepLinearThreshold = 0.02f;
    constexpr float kSleepAngularThreshold = 0.02f;

    enum class ColliderKind
    {
        None,
        Box,
        Sphere,
        Capsule,
    };

    struct ColliderConfig
    {
        ColliderKind kind = ColliderKind::None;
        glm::vec3 center = glm::vec3(0.0f);
        glm::vec3 halfExtents = glm::vec3(0.0f);
        float radius = 0.0f;
        float height = 0.0f;
        bool isTrigger = false;
        PhysicsMaterial material;
    };

    struct RigidbodyConfig
    {
        bool present = false;
        RigidbodyType type = RigidbodyType::Static;
        float mass = 0.0f;
        float gravityScale = 1.0f;
        float linearDamping = 0.05f;
        float angularDamping = 0.8f;
        bool useGravity = true;
        bool lockRotation = false;
        bool isTrigger = false;
    };

    glm::vec3 ToGlm(const btVector3 &value)
    {
        return glm::vec3(value.x(), value.y(), value.z());
    }

    btVector3 ToBullet(const glm::vec3 &value)
    {
        return btVector3(value.x, value.y, value.z);
    }

    btVector3 ClampMagnitude(const btVector3 &value, float maxMagnitude)
    {
        const btScalar lengthSquared = value.length2();
        const btScalar maxMagnitudeSquared = maxMagnitude * maxMagnitude;
        if (lengthSquared <= maxMagnitudeSquared)
        {
            return value;
        }

        return value.normalized() * maxMagnitude;
    }

    btQuaternion ToBulletRotation(const glm::vec3 &rotationDegrees)
    {
        glm::mat4 rotationMatrix(1.0f);
        rotationMatrix = glm::rotate(rotationMatrix, glm::radians(rotationDegrees.x), glm::vec3(1.0f, 0.0f, 0.0f));
        rotationMatrix = glm::rotate(rotationMatrix, glm::radians(rotationDegrees.y), glm::vec3(0.0f, 1.0f, 0.0f));
        rotationMatrix = glm::rotate(rotationMatrix, glm::radians(rotationDegrees.z), glm::vec3(0.0f, 0.0f, 1.0f));

        const glm::mat3 basis(rotationMatrix);
        btMatrix3x3 bulletBasis(
            basis[0][0], basis[1][0], basis[2][0],
            basis[0][1], basis[1][1], basis[2][1],
            basis[0][2], basis[1][2], basis[2][2]);

        btQuaternion rotation;
        bulletBasis.getRotation(rotation);
        return rotation;
    }

    glm::vec3 ToEulerDegrees(const btQuaternion &rotation)
    {
        const btMatrix3x3 basis(rotation);
        const float r00 = basis[0][0];
        const float r01 = basis[0][1];
        const float r02 = basis[0][2];
        const float r12 = basis[1][2];
        const float r22 = basis[2][2];

        const float y = std::asin(glm::clamp(r02, -1.0f, 1.0f));
        const float x = std::atan2(-r12, r22);
        const float z = std::atan2(-r01, r00);
        return glm::degrees(glm::vec3(x, y, z));
    }

    glm::quat ToGlm(const btQuaternion &value)
    {
        return glm::normalize(glm::quat(value.w(), value.x(), value.y(), value.z()));
    }

    btTransform MakeWorldTransform(const Transform &transform)
    {
        btTransform worldTransform;
        worldTransform.setIdentity();
        worldTransform.setOrigin(ToBullet(transform.position));
        worldTransform.setRotation(ToBulletRotation(transform.rotation));
        return worldTransform;
    }

    btVector3 GetSafeScale(const glm::vec3 &scale)
    {
        return btVector3(
            std::max(std::abs(scale.x), kMinColliderExtent),
            std::max(std::abs(scale.y), kMinColliderExtent),
            std::max(std::abs(scale.z), kMinColliderExtent));
    }

    bool NearlyEqual(float left, float right)
    {
        return std::abs(left - right) <= kComparisonEpsilon;
    }

    bool NearlyEqual(const glm::vec3 &left, const glm::vec3 &right)
    {
        return NearlyEqual(left.x, right.x) && NearlyEqual(left.y, right.y) && NearlyEqual(left.z, right.z);
    }

    bool SameMaterial(const PhysicsMaterial &left, const PhysicsMaterial &right)
    {
        return NearlyEqual(left.friction, right.friction) && NearlyEqual(left.restitution, right.restitution);
    }

    ColliderConfig GetColliderConfig(const World &world, Entity entity)
    {
        if (const BoxCollider *box = world.GetComponent<BoxCollider>(entity))
        {
            ColliderConfig config;
            config.kind = ColliderKind::Box;
            config.center = box->center;
            config.halfExtents = box->halfExtents;
            config.isTrigger = box->isTrigger;
            config.material = box->material;
            return config;
        }

        if (const SphereCollider *sphere = world.GetComponent<SphereCollider>(entity))
        {
            ColliderConfig config;
            config.kind = ColliderKind::Sphere;
            config.center = sphere->center;
            config.radius = sphere->radius;
            config.isTrigger = sphere->isTrigger;
            config.material = sphere->material;
            return config;
        }

        if (const CapsuleCollider *capsule = world.GetComponent<CapsuleCollider>(entity))
        {
            ColliderConfig config;
            config.kind = ColliderKind::Capsule;
            config.center = capsule->center;
            config.radius = capsule->radius;
            config.height = capsule->height;
            config.isTrigger = capsule->isTrigger;
            config.material = capsule->material;
            return config;
        }

        return {};
    }

    RigidbodyConfig GetRigidbodyConfig(const World &world, Entity entity)
    {
        const Rigidbody *rigidbody = world.GetComponent<Rigidbody>(entity);
        if (!rigidbody)
        {
            return {};
        }

        RigidbodyConfig config;
        config.present = true;
        config.type = rigidbody->type;
        config.mass = rigidbody->mass;
        config.gravityScale = rigidbody->gravityScale;
        config.linearDamping = rigidbody->linearDamping;
        config.angularDamping = rigidbody->angularDamping;
        config.useGravity = rigidbody->useGravity;
        config.lockRotation = rigidbody->lockRotation;
        config.isTrigger = rigidbody->isTrigger;
        return config;
    }

    bool SameColliderConfig(const ColliderConfig &left, const ColliderConfig &right)
    {
        return left.kind == right.kind &&
               NearlyEqual(left.center, right.center) &&
               NearlyEqual(left.halfExtents, right.halfExtents) &&
               NearlyEqual(left.radius, right.radius) &&
               NearlyEqual(left.height, right.height) &&
               left.isTrigger == right.isTrigger &&
               SameMaterial(left.material, right.material);
    }

    bool SameRigidbodyConfig(const RigidbodyConfig &left, const RigidbodyConfig &right)
    {
        return left.present == right.present &&
               left.type == right.type &&
               NearlyEqual(left.mass, right.mass) &&
               NearlyEqual(left.gravityScale, right.gravityScale) &&
               NearlyEqual(left.linearDamping, right.linearDamping) &&
               NearlyEqual(left.angularDamping, right.angularDamping) &&
               left.useGravity == right.useGravity &&
               left.lockRotation == right.lockRotation &&
               left.isTrigger == right.isTrigger;
    }

    std::unique_ptr<btCollisionShape> CreateCollisionShape(const ColliderConfig &colliderConfig, const glm::vec3 &scale)
    {
        const btVector3 safeScale = GetSafeScale(scale);
        std::unique_ptr<btCollisionShape> shape;
        btTransform colliderOffset;
        colliderOffset.setIdentity();
        bool hasOffset = false;

        if (colliderConfig.kind == ColliderKind::Box)
        {
            const glm::vec3 extents = glm::max(glm::abs(colliderConfig.halfExtents), glm::vec3(kMinColliderExtent));
            shape = std::make_unique<btBoxShape>(ToBullet(extents));
            shape->setLocalScaling(safeScale);
            colliderOffset.setOrigin(ToBullet(colliderConfig.center));
            hasOffset = glm::dot(colliderConfig.center, colliderConfig.center) > 0.0f;
        }
        else if (colliderConfig.kind == ColliderKind::Sphere)
        {
            const float maxScale = std::max({safeScale.x(), safeScale.y(), safeScale.z()});
            shape = std::make_unique<btSphereShape>(std::max(colliderConfig.radius * maxScale, kMinColliderExtent));
            colliderOffset.setOrigin(ToBullet(colliderConfig.center));
            hasOffset = glm::dot(colliderConfig.center, colliderConfig.center) > 0.0f;
        }
        else if (colliderConfig.kind == ColliderKind::Capsule)
        {
            const float radialScale = std::max(safeScale.x(), safeScale.z());
            const float radius = std::max(colliderConfig.radius * radialScale, kMinColliderExtent);
            const float height = std::max(colliderConfig.height * safeScale.y(), radius * 2.0f);
            shape = std::make_unique<btCapsuleShape>(radius, std::max(height - radius * 2.0f, 0.0f));
            colliderOffset.setOrigin(ToBullet(colliderConfig.center));
            hasOffset = glm::dot(colliderConfig.center, colliderConfig.center) > 0.0f;
        }

        if (!shape)
        {
            return nullptr;
        }

        shape->setMargin(kShapeMargin);

        if (!hasOffset)
        {
            return shape;
        }

        auto compound = std::make_unique<btCompoundShape>();
        compound->addChildShape(colliderOffset, shape.release());
        return compound;
    }

    struct BulletBodyRecord
    {
        Entity entity;
        glm::vec3 scale = glm::vec3(1.0f);
        ColliderConfig colliderConfig;
        RigidbodyConfig rigidbodyConfig;
        std::unique_ptr<btCollisionShape> shape;
        std::unique_ptr<btDefaultMotionState> motionState;
        std::unique_ptr<btRigidBody> body;
    };

    struct PhysicsPairKey
    {
        Entity first;
        Entity second;

        bool operator==(const PhysicsPairKey &other) const
        {
            return first == other.first && second == other.second;
        }
    };

    struct PhysicsPairKeyHash
    {
        size_t operator()(const PhysicsPairKey &key) const
        {
            const uint64_t combined = (static_cast<uint64_t>(key.first.GetID()) << 32u) | static_cast<uint64_t>(key.second.GetID());
            return std::hash<uint64_t>()(combined);
        }
    };

    struct ActivePairState
    {
        bool trigger = false;
        glm::vec3 point = glm::vec3(0.0f);
        glm::vec3 normal = glm::vec3(0.0f, 1.0f, 0.0f);
        bool hasContact = false;
    };

    PhysicsPairKey MakePairKey(Entity first, Entity second)
    {
        if (second < first)
        {
            std::swap(first, second);
        }

        return PhysicsPairKey{first, second};
    }

    PhysicsEventType MakeEventType(bool trigger, bool entering, bool exiting)
    {
        if (trigger)
        {
            if (entering)
            {
                return PhysicsEventType::TriggerEnter;
            }

            return exiting ? PhysicsEventType::TriggerExit : PhysicsEventType::TriggerStay;
        }

        if (entering)
        {
            return PhysicsEventType::CollisionEnter;
        }

        return exiting ? PhysicsEventType::CollisionExit : PhysicsEventType::CollisionStay;
    }

    class BulletPhysicsBackend : public IPhysicsBackend
    {
    public:
        BulletPhysicsBackend()
            : m_dispatcher(&m_collisionConfiguration),
              m_world(&m_dispatcher, &m_broadphase, &m_solver, &m_collisionConfiguration)
        {
            m_world.getDispatchInfo().m_enableSPU = false;
            m_world.getSolverInfo().m_numIterations = 24;
            m_world.getSolverInfo().m_splitImpulse = true;
            m_world.getSolverInfo().m_splitImpulsePenetrationThreshold = -0.02f;
        }

        void Step(World &world, const glm::vec3 &gravity, float deltaTime) override
        {
            if (deltaTime <= 0.0f)
            {
                m_events.clear();
                return;
            }

            SyncWorld(world, gravity);
            m_world.stepSimulation(deltaTime, 4, 1.0f / 120.0f);
            UpdateEvents();

            for (auto &[entity, record] : m_records)
            {
                (void)entity;
                Rigidbody *rigidbody = world.GetComponent<Rigidbody>(record.entity);
                Transform *transform = world.GetComponent<Transform>(record.entity);
                if (!rigidbody || !transform || rigidbody->type != RigidbodyType::Dynamic)
                {
                    continue;
                }

                const btVector3 clampedAngularVelocity = ClampMagnitude(record.body->getAngularVelocity(), kMaxAngularSpeedRadians);
                record.body->setAngularVelocity(clampedAngularVelocity);

                const btTransform &worldTransform = record.body->getWorldTransform();
                transform->position = ToGlm(worldTransform.getOrigin());
                transform->rotation = ToEulerDegrees(worldTransform.getRotation());
                transform->SetRotationOverride(ToGlm(worldTransform.getRotation()));
                rigidbody->linearVelocity = ToGlm(record.body->getLinearVelocity());
                rigidbody->angularVelocity = glm::degrees(ToGlm(clampedAngularVelocity));
            }
        }

        RaycastResult Raycast(const World &world, const PhysicsRay &ray, float maxDistance) const override
        {
            const float rayLength = glm::length(ray.direction);
            if (rayLength <= 0.00001f || maxDistance <= 0.0f)
            {
                return std::nullopt;
            }

            World &mutableWorld = const_cast<World &>(world);
            SyncWorld(mutableWorld, m_lastGravity);

            const glm::vec3 direction = ray.direction / rayLength;
            const btVector3 rayFrom = ToBullet(ray.origin);
            const btVector3 rayTo = ToBullet(ray.origin + direction * maxDistance);
            btCollisionWorld::ClosestRayResultCallback callback(rayFrom, rayTo);
            m_world.rayTest(rayFrom, rayTo, callback);

            if (!callback.hasHit())
            {
                return std::nullopt;
            }

            auto entityIt = m_entityLookup.find(callback.m_collisionObject);
            if (entityIt == m_entityLookup.end())
            {
                return std::nullopt;
            }

            const float distance = callback.m_closestHitFraction * maxDistance;
            return RaycastHit{
                entityIt->second,
                ToGlm(callback.m_hitPointWorld),
                glm::normalize(ToGlm(callback.m_hitNormalWorld)),
                distance};
        }

        const std::vector<PhysicsEvent> &GetEvents() const override
        {
            return m_events;
        }

    private:
        static bool HasCollider(const World &world, Entity entity)
        {
            return world.HasComponent<BoxCollider>(entity) ||
                   world.HasComponent<SphereCollider>(entity) ||
                   world.HasComponent<CapsuleCollider>(entity);
        }

        void RemoveBody(Entity entity) const
        {
            auto recordIt = m_records.find(entity);
            if (recordIt == m_records.end())
            {
                return;
            }

            m_entityLookup.erase(recordIt->second.body.get());
            m_world.removeRigidBody(recordIt->second.body.get());
            m_records.erase(recordIt);
        }

        void CreateBody(World &world, Entity entity, const glm::vec3 &gravity) const
        {
            Transform *transform = world.GetComponent<Transform>(entity);
            if (!transform)
            {
                return;
            }

            const ColliderConfig colliderConfig = GetColliderConfig(world, entity);
            if (colliderConfig.kind == ColliderKind::None)
            {
                return;
            }

            const RigidbodyConfig rigidbodyConfig = GetRigidbodyConfig(world, entity);
            const RigidbodyType bodyType = rigidbodyConfig.present ? rigidbodyConfig.type : RigidbodyType::Static;
            const float mass = bodyType == RigidbodyType::Dynamic ? std::max(rigidbodyConfig.mass, 0.001f) : 0.0f;

            BulletBodyRecord record;
            record.entity = entity;
            record.scale = transform->scale;
            record.colliderConfig = colliderConfig;
            record.rigidbodyConfig = rigidbodyConfig;
            record.shape = CreateCollisionShape(colliderConfig, transform->scale);
            if (!record.shape)
            {
                return;
            }

            btVector3 localInertia(0.0f, 0.0f, 0.0f);
            if (mass > 0.0f)
            {
                record.shape->calculateLocalInertia(mass, localInertia);
            }

            const btTransform worldTransform = MakeWorldTransform(*transform);
            record.motionState = std::make_unique<btDefaultMotionState>(worldTransform);
            btRigidBody::btRigidBodyConstructionInfo bodyInfo(mass, record.motionState.get(), record.shape.get(), localInertia);
            record.body = std::make_unique<btRigidBody>(bodyInfo);

            m_world.addRigidBody(record.body.get());
            m_entityLookup.emplace(record.body.get(), entity);
            m_records.emplace(entity, std::move(record));
            SyncBodyState(world, m_records.at(entity), gravity, true);
        }

        void SyncBodyState(World &world, BulletBodyRecord &record, const glm::vec3 &gravity, bool forceTransformSync) const
        {
            Transform *transform = world.GetComponent<Transform>(record.entity);
            Rigidbody *rigidbody = world.GetComponent<Rigidbody>(record.entity);
            if (!transform)
            {
                return;
            }

            const RigidbodyType bodyType = rigidbody ? rigidbody->type : RigidbodyType::Static;
            record.body->setFriction(record.colliderConfig.material.friction);
            record.body->setRestitution(record.colliderConfig.material.restitution);
            record.body->setRollingFriction(record.colliderConfig.material.friction * 0.25f);
            record.body->setSpinningFriction(record.colliderConfig.material.friction * 0.25f);

            if ((rigidbody && rigidbody->isTrigger) || record.colliderConfig.isTrigger)
            {
                record.body->setCollisionFlags(record.body->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
            }
            else
            {
                record.body->setCollisionFlags(record.body->getCollisionFlags() & ~btCollisionObject::CF_NO_CONTACT_RESPONSE);
            }

            if (bodyType == RigidbodyType::Kinematic)
            {
                record.body->setCollisionFlags(record.body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
                record.body->setActivationState(DISABLE_DEACTIVATION);
            }
            else
            {
                record.body->setCollisionFlags(record.body->getCollisionFlags() & ~btCollisionObject::CF_KINEMATIC_OBJECT);
            }

            if (rigidbody)
            {
                record.body->setDamping(rigidbody->linearDamping, rigidbody->angularDamping);
                record.body->setLinearVelocity(ToBullet(rigidbody->linearVelocity));
                record.body->setAngularVelocity(ClampMagnitude(ToBullet(glm::radians(rigidbody->angularVelocity)), kMaxAngularSpeedRadians));

                if (rigidbody->lockRotation)
                {
                    record.body->setAngularFactor(btVector3(0.0f, 0.0f, 0.0f));
                }
                else
                {
                    record.body->setAngularFactor(btVector3(1.0f, 1.0f, 1.0f));
                }

                if (!rigidbody->useGravity || rigidbody->gravityScale == 0.0f)
                {
                    record.body->setGravity(btVector3(0.0f, 0.0f, 0.0f));
                }
                else
                {
                    record.body->setGravity(ToBullet(gravity * rigidbody->gravityScale));
                }

                if (bodyType == RigidbodyType::Dynamic)
                {
                    record.body->setSleepingThresholds(kSleepLinearThreshold, kSleepAngularThreshold);
                }
            }

            if (forceTransformSync || bodyType != RigidbodyType::Dynamic)
            {
                const btTransform worldTransform = MakeWorldTransform(*transform);
                record.motionState->setWorldTransform(worldTransform);
                record.body->setWorldTransform(worldTransform);
                transform->ClearRotationOverride();
                record.body->activate(true);
            }
        }

        bool NeedsRebuild(World &world, Entity entity, const BulletBodyRecord &record) const
        {
            const Transform *transform = world.GetComponent<Transform>(entity);
            if (!transform)
            {
                return true;
            }

            return !NearlyEqual(record.scale, transform->scale) ||
                   !SameColliderConfig(record.colliderConfig, GetColliderConfig(world, entity)) ||
                   !SameRigidbodyConfig(record.rigidbodyConfig, GetRigidbodyConfig(world, entity));
        }

        void SyncWorld(World &world, const glm::vec3 &gravity) const
        {
            m_lastGravity = gravity;
            m_world.setGravity(ToBullet(gravity));

            std::vector<Entity> toRemove;
            toRemove.reserve(m_records.size());
            for (const auto &[entity, record] : m_records)
            {
                (void)record;
                if (!world.IsEntityActive(entity) || !HasCollider(world, entity) || !world.HasComponent<Transform>(entity))
                {
                    toRemove.push_back(entity);
                }
            }

            for (Entity entity : toRemove)
            {
                RemoveBody(entity);
            }

            for (Entity entity : world.GetEntitiesWith<Transform>())
            {
                if (!HasCollider(world, entity))
                {
                    continue;
                }

                auto recordIt = m_records.find(entity);
                if (recordIt == m_records.end())
                {
                    CreateBody(world, entity, gravity);
                    continue;
                }

                if (NeedsRebuild(world, entity, recordIt->second))
                {
                    RemoveBody(entity);
                    CreateBody(world, entity, gravity);
                    continue;
                }

                SyncBodyState(world, recordIt->second, gravity, false);
            }
        }

        void UpdateEvents() const
        {
            std::unordered_map<PhysicsPairKey, ActivePairState, PhysicsPairKeyHash> currentPairs;
            const int manifoldCount = m_dispatcher.getNumManifolds();
            for (int manifoldIndex = 0; manifoldIndex < manifoldCount; ++manifoldIndex)
            {
                btPersistentManifold *manifold = m_dispatcher.getManifoldByIndexInternal(manifoldIndex);
                if (!manifold)
                {
                    continue;
                }

                const btCollisionObject *objectA = manifold->getBody0();
                const btCollisionObject *objectB = manifold->getBody1();
                auto entityAIt = m_entityLookup.find(objectA);
                auto entityBIt = m_entityLookup.find(objectB);
                if (entityAIt == m_entityLookup.end() || entityBIt == m_entityLookup.end())
                {
                    continue;
                }

                ActivePairState pairState;
                pairState.trigger = (objectA->getCollisionFlags() & btCollisionObject::CF_NO_CONTACT_RESPONSE) != 0 ||
                                    (objectB->getCollisionFlags() & btCollisionObject::CF_NO_CONTACT_RESPONSE) != 0;

                float bestDistance = std::numeric_limits<float>::max();
                for (int contactIndex = 0; contactIndex < manifold->getNumContacts(); ++contactIndex)
                {
                    const btManifoldPoint &contact = manifold->getContactPoint(contactIndex);
                    if (contact.getDistance() > 0.02f)
                    {
                        continue;
                    }

                    if (contact.getDistance() < bestDistance)
                    {
                        bestDistance = contact.getDistance();
                        const glm::vec3 pointA = ToGlm(contact.getPositionWorldOnA());
                        const glm::vec3 pointB = ToGlm(contact.getPositionWorldOnB());
                        pairState.point = (pointA + pointB) * 0.5f;
                        pairState.normal = glm::normalize(ToGlm(contact.m_normalWorldOnB));
                        pairState.hasContact = true;
                    }
                }

                if (!pairState.hasContact)
                {
                    continue;
                }

                Entity first = entityAIt->second;
                Entity second = entityBIt->second;
                if (second < first)
                {
                    std::swap(first, second);
                    pairState.normal *= -1.0f;
                }

                currentPairs[MakePairKey(first, second)] = pairState;
            }

            m_events.clear();
            m_events.reserve(currentPairs.size() + m_activePairs.size());
            for (const auto &[pairKey, pairState] : currentPairs)
            {
                const auto previousIt = m_activePairs.find(pairKey);
                const bool entering = previousIt == m_activePairs.end() || previousIt->second.trigger != pairState.trigger;
                m_events.push_back(PhysicsEvent{
                    MakeEventType(pairState.trigger, entering, false),
                    pairKey.first,
                    pairKey.second,
                    pairState.point,
                    pairState.normal,
                    pairState.hasContact});
            }

            for (const auto &[pairKey, pairState] : m_activePairs)
            {
                if (currentPairs.find(pairKey) != currentPairs.end())
                {
                    continue;
                }

                m_events.push_back(PhysicsEvent{
                    MakeEventType(pairState.trigger, false, true),
                    pairKey.first,
                    pairKey.second,
                    pairState.point,
                    pairState.normal,
                    pairState.hasContact});
            }

            m_activePairs = std::move(currentPairs);
        }

    private:
        mutable btDefaultCollisionConfiguration m_collisionConfiguration;
        mutable btCollisionDispatcher m_dispatcher;
        mutable btDbvtBroadphase m_broadphase;
        mutable btSequentialImpulseConstraintSolver m_solver;
        mutable btDiscreteDynamicsWorld m_world;
        mutable std::unordered_map<Entity, BulletBodyRecord> m_records;
        mutable std::unordered_map<const btCollisionObject *, Entity> m_entityLookup;
        mutable std::unordered_map<PhysicsPairKey, ActivePairState, PhysicsPairKeyHash> m_activePairs;
        mutable std::vector<PhysicsEvent> m_events;
        mutable glm::vec3 m_lastGravity = glm::vec3(0.0f, -9.81f, 0.0f);
    };
}

namespace physics
{
    std::unique_ptr<IPhysicsBackend> CreateDefaultPhysicsBackend()
    {
        return std::make_unique<BulletPhysicsBackend>();
    }
}