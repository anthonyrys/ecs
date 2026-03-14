// ECS.hpp

#ifndef ECS_HPP
#define ECS_HPP

#include "Logging.hpp"
#include "SparseSet.hpp"

#include <bitset>
#include <cstdint>
#include <limits>
#include <memory>
#include <queue>
#include <typeindex>
#include <type_traits>
#include <unordered_map>

#define ECS_ASSERT_ENTITY_EXISTS(e) ECS_LOG_ASSERT((m_CurrentEntities.Get(e) != nullptr))

namespace ECS
{


using Entity = uint64_t;
using ComponentMask = uint64_t;

class ECS;

// ISystem
struct ISystem {
    virtual ~ISystem() = default;

    virtual void Update() = 0;
    virtual void Render() = 0;
};

// View
template<typename... Components>
class View
{
public:
    View(std::shared_ptr<ECS> ecs);
    ~View(void);

    template <typename F>
    void GetAll(F&& lambda);

private:
    std::shared_ptr<ECS> m_ECS;
    ComponentMask m_Mask;
};

// ECS
class ECS : public std::enable_shared_from_this<ECS>
{
    template<typename... Components>
    friend class View;

public:
    static constexpr size_t MAX_ENTITIES = 2048u;
    static constexpr size_t MAX_COMPONENTS = 64u;

    using ComponentID = std::type_index;
    using ComponentBitset = std::bitset<MAX_COMPONENTS>;

public:
    ECS(void);
    ~ECS(void);

    Entity CreateEntity(void);
    void DestroyEntity(Entity e);

    template <typename... Components>
    std::unique_ptr<View<Components...>> GetView();

    // Components
    template <typename T>
    T &AddComponent(Entity e, T &&component = {});

    template <typename T>
    T &UpdateComponent(Entity e, T &&component = {});

    template <typename T>
    T &GetComponent(Entity e);

    template <typename T>
    void RemoveComponent(Entity e);

    template <typename T>
    bool HasComponent(Entity e);
    
    // Systems
    template <typename T, typename... Args>
    void AddSystem(Args&&... args);

    // Runtime
    void Update(void);
    void Render(void);

private:
    SparseSet<Entity, ComponentMask> m_CurrentEntities;
    std::queue<Entity> m_AvailableEntities;

    Entity m_NextEntity = 0;

    std::unordered_map<ComponentID, ComponentMask> m_ComponentMasks;

    std::unordered_map<ComponentMask, std::unique_ptr<ISparseSet>> m_ComponentSets;
    std::unordered_map<ComponentMask, SparseSet<Entity, Entity>> m_EntitySets;

    std::vector<std::unique_ptr<ISystem>> m_Systems;

private:
    // Components
    template <typename T>
    void RegisterComponentMask();
    
    template <typename... Components>
    ComponentMask GetComponentMask();

    template <typename T>
    ComponentMask GetComponentMaskImpl();

    ComponentMask CreateComponentMask() const;
    
    template <typename T>
    void RegisterComponentSet();

    template <typename T>
    SparseSet<Entity, T> &GetComponentSet();
};

// View Impl
template <typename... Components>
View<Components...>::View(std::shared_ptr<ECS> ecs)
    : m_ECS(ecs)
{
    m_Mask = ecs->GetComponentMask<Components...>();
}

template <typename... Components>
View<Components...>::~View(void)
{
}

template <typename... Components>
template <typename F>
void View<Components...>::GetAll(F &&lambda)
{
    for (const auto &[mask, entities] : m_ECS->m_EntitySets)
    {
        // Retrieve all entities containing the mask
        if ((mask & m_Mask) != m_Mask)
            continue;

        for (const Entity &e : entities.Values())
            lambda(e, m_ECS->GetComponent<Components>(e)...);
    }
}

// ECS Impl
ECS::ECS(void)
{
}

ECS::~ECS(void)
{
}

Entity ECS::CreateEntity(void)
{
    ECS_LOG_ASSERT((m_NextEntity < MAX_ENTITIES));

    if (m_AvailableEntities.empty())
    {
        m_CurrentEntities.Set(m_NextEntity, 0);
        return m_NextEntity++;
    }
    
    Entity e = m_AvailableEntities.front();
    m_AvailableEntities.pop();

    m_CurrentEntities.Set(e, 0);
    return e;
}

void ECS::DestroyEntity(Entity e)
{
    ECS_ASSERT_ENTITY_EXISTS(e);

    ComponentBitset bs(*m_CurrentEntities.Get(e));
    m_EntitySets[bs.to_ullong()].Delete(e);

    for (size_t i = 0; i < MAX_COMPONENTS; i++)
    {
        if (bs[i] == 0)
            continue;
        
        ComponentBitset b((1 << i));
        m_ComponentSets[b.to_ullong()]->Delete(e);
    }

    m_CurrentEntities.Delete(e);
    m_AvailableEntities.push(e);
}

template <typename... Components>
std::unique_ptr<View<Components...>> ECS::GetView()
{
    return std::make_unique<View<Components...>>(this->shared_from_this());
}

template <typename T, typename... Args>
void ECS::AddSystem(Args&&... args)
{
    // For virtual functions Update/Render
    ECS_LOG_ASSERT((std::is_base_of<ISystem, T>()));

    // All systems must have a constructor for shared_ptr<ECS>
    m_Systems.push_back(std::make_unique<T>(this->shared_from_this(), std::forward<Args>(args)...));
}

void ECS::Update(void)
{
    for (const std::unique_ptr<ISystem> &s : m_Systems)
        s->Update();
}

void ECS::Render(void)
{
     for (const std::unique_ptr<ISystem> &s : m_Systems)
        s->Render();
}

template <typename T>
T &ECS::AddComponent(Entity e, T &&component)
{
    ECS_ASSERT_ENTITY_EXISTS(e);

    SparseSet<Entity, T> &set = this->GetComponentSet<T>();
    T *t = set.Set(e, std::move(component));

    ComponentMask componentMask = this->GetComponentMask<T>();
    ComponentMask &entityMask = *m_CurrentEntities.Get(e);

    // Update masks
    ComponentBitset bs(ComponentBitset(entityMask) | ComponentBitset(componentMask));
    ComponentMask m = bs.to_ullong();

    m_EntitySets[entityMask].Delete(e);
    m_EntitySets[m].Set(e, e);

    entityMask = m;
    return *t;
}

template <typename T>
T &ECS::UpdateComponent(Entity e, T &&component)
{
    ECS_LOG_ASSERT(this->HasComponent<T>(e));

    SparseSet<Entity, T> &set = this->GetComponentSet<T>();
    T *t = set.Set(e, std::move(component));

    return *t;
}

template <typename T>
T &ECS::GetComponent(Entity e)
{
    ECS_ASSERT_ENTITY_EXISTS(e);
    ECS_LOG_ASSERT(this->HasComponent<T>(e));

    SparseSet<Entity, T> &set = this->GetComponentSet<T>();
    T *component = set.Get(e);

    return *component;
}

template <typename T>
void ECS::RemoveComponent(Entity e)
{
    ECS_LOG_ASSERT(this->HasComponent<T>(e));

    SparseSet<Entity, T> &set = this->GetComponentSet<T>();
    set.Delete(e);

    ComponentMask componentMask = this->GetComponentMask<T>();
    ComponentMask &entityMask = *m_CurrentEntities.Get(e);

    // Update entity mask
    ComponentBitset bs(ComponentBitset(entityMask) & ~ComponentBitset(componentMask));
    ComponentMask m = bs.to_ullong();

    m_EntitySets[entityMask].Delete(e);
    m_EntitySets[m].Set(e, e);

    entityMask = m;
}

template <typename T>
bool ECS::HasComponent(Entity e)
{
    ECS_ASSERT_ENTITY_EXISTS(e);

    SparseSet<Entity, T> &set = this->GetComponentSet<T>();
    T *component = set.Get(e);

    return component != nullptr;
}

template <typename T>
void ECS::RegisterComponentMask()
{
    ComponentID id = ComponentID(typeid(T));
    m_ComponentMasks[id] = this->CreateComponentMask();
}

template <typename... Components>
ComponentMask ECS::GetComponentMask()
{
    return (this->GetComponentMaskImpl<Components>() | ...);
}

template <typename T>
ComponentMask ECS::GetComponentMaskImpl()
{
    ComponentID id = ComponentID(typeid(T));
    if (m_ComponentMasks.find(id) == m_ComponentMasks.end())
        this->RegisterComponentMask<T>();

    return m_ComponentMasks[id];
}

ComponentMask ECS::CreateComponentMask() const
{
    static size_t s_NextComponentMaskIndex = 0;
    ECS_LOG_ASSERT((s_NextComponentMaskIndex < MAX_COMPONENTS));

    ComponentBitset bs(1ull << (s_NextComponentMaskIndex++));
    return bs.to_ullong();
}

template <typename T>
void ECS::RegisterComponentSet()
{
    ComponentMask mask = this->GetComponentMask<T>();
    m_ComponentSets[mask] = std::make_unique<SparseSet<Entity, T>>();
}

template <typename T>
SparseSet<Entity, T> &ECS::GetComponentSet()
{
    ComponentMask mask = this->GetComponentMask<T>();
    if (m_ComponentSets.find(mask) == m_ComponentSets.end())
        this->RegisterComponentSet<T>();

    ISparseSet *ptr = m_ComponentSets[mask].get();
    return *static_cast<SparseSet<Entity, T> *>(ptr);
}


}

#endif
