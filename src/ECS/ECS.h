#ifndef ECS_H
#define ECS_H

#include <bitset>
#include <set>
#include <spdlog/spdlog.h>
#include <typeindex>
#include <unordered_map>
#include <vector>

const unsigned int MAX_COMPONENTS = 32;

typedef std::bitset<MAX_COMPONENTS> Signature;

struct IComponent {
protected:
  static unsigned int nextId;
};

template <typename T> class Component : public IComponent {
public:
  static unsigned int GetId() {
    static auto id = nextId++;
    return id;
  }
};

class Entity {
private:
  unsigned int id;

public:
  Entity(unsigned int id) : id(id){};
  unsigned int GetId() const;

  bool operator==(const Entity &other) const { return other.id == id; }
  bool operator!=(const Entity &other) const { return other.id != id; }
  bool operator>(const Entity &other) const { return id > other.id; }
  bool operator<(const Entity &other) const { return id < other.id; }

  template <typename TComponent, typename... TArgs>
  void AddComponent(TArgs &&...args);
  template <typename TComponent> void RemoveComponent();
  template <typename TComponent> bool HasComponent() const;
  template <typename TComponent> TComponent &GetComponent() const;

  class Registry *registry;
};

class System {
private:
  Signature componentSignature;
  std::vector<Entity> entities;

public:
  System() = default;
  ~System() = default;

  void AddEntityToSystem(Entity entity);
  void RemoveEntityFromSystem(Entity entity);
  std::vector<Entity> GetSystemEntities() const;
  const Signature &GetComponentSignature() const;

  template <typename T> void RequireComponent();
};

class IPool {
public:
  virtual ~IPool() {}
};

template <typename T> class Pool : public IPool {
private:
  std::vector<T> data;

public:
  Pool(unsigned int size = 100) { data.resize(size); }
  virtual ~Pool() = default;

  bool IsEmpty() const { return data.empty(); }

  unsigned int GetSize() const { return data.size(); }

  void Resize(unsigned int n) { data.resize(n); }

  void Clear() { data.clear(); }

  void Add(T object) { data.push_back(object); }

  void Set(unsigned int index, T object) { data[index] = object; }

  T &Get(unsigned int index) { return static_cast<T &>(data[index]); }

  T &operator[](unsigned int index) { return data[index]; }
};

class Registry {
private:
  unsigned int numEntities = 0;
  std::set<Entity> entitiesToBeAdded;
  std::set<Entity> entitiesToBeKilled;

  std::vector<std::shared_ptr<IPool>> componentPools;

  std::vector<Signature> entityComponentSignatures;

  std::unordered_map<std::type_index, std::shared_ptr<System>> systems;

public:
  Registry() { spdlog::info("Registry constructor called"); }
  ~Registry() { spdlog::info("Registry destructor called"); }

  Entity CreateEntity();

  void Update();

  template <typename TComponent, typename... TArgs>
  void AddComponent(Entity entity, TArgs &&...args);
  template <typename TComponent> void RemoveComponent(Entity entity);
  template <typename TComponent> bool HasComponent(Entity entity) const;
  template <typename TComponent> TComponent &GetComponent(Entity entity) const;

  template <typename TSystem, typename... TArgs>
  void AddSystem(TArgs &&...args);
  template <typename TSystem> void RemoveSystem();
  template <typename TSystem> bool HasSystem() const;
  template <typename TSystem> TSystem &GetSystem() const;

  void AddEntityToSystems(Entity entity);
};

template <typename TSystem, typename... TArgs>
void
Registry::AddSystem(TArgs &&...args) {
  std::shared_ptr<TSystem> newSystem =
      std::make_shared<TSystem>(std::forward<TArgs>(args)...);
  systems.insert(std::make_pair(std::type_index(typeid(TSystem)), newSystem));
}

template <typename TSystem>
void
Registry::RemoveSystem() {
  auto system = systems.find(std::type_index(typeid(TSystem)));
  systems.erase(system);
}

template <typename TSystem>
bool
Registry::HasSystem() const {
  return systems.find(std::type_index(typeid(TSystem))) != systems.end();
}

template <typename TSystem>
TSystem &
Registry::GetSystem() const {
  auto system = systems.find(std::type_index(typeid(TSystem)));
  return *static_cast<TSystem *>(system->second.get());
}

template <typename T>
void
System::RequireComponent() {
  const auto componentId = Component<T>::GetId();
  componentSignature.set(componentId);
}

template <typename TComponent, typename... TArgs>
void
Registry::AddComponent(Entity entity, TArgs &&...args) {
  const auto componentId = Component<TComponent>::GetId();

  const auto entityId = entity.GetId();

  if (componentId >= componentPools.size()) {
    componentPools.resize(componentId + 1, nullptr);
  }

  if (!componentPools[componentId]) {
    std::shared_ptr<Pool<TComponent>> newComponentPool =
        std::make_shared<Pool<TComponent>>();
    componentPools[componentId] = newComponentPool;
  }

  std::shared_ptr<Pool<TComponent>> componentPool =
      std::static_pointer_cast<Pool<TComponent>>(componentPools[componentId]);

  if (entityId >= componentPool->GetSize()) {
    componentPool->Resize(numEntities);
  }

  TComponent newComponent(std::forward<TArgs>(args)...);

  componentPool->Set(entityId, newComponent);

  entityComponentSignatures[entityId].set(componentId);

  spdlog::info("Component id =  " + std::to_string(componentId) +
               " was added to entity id " + std::to_string(entityId));
}

template <typename TComponent>
void
Registry::RemoveComponent(Entity entity) {
  const auto componentId = Component<TComponent>::GetId();
  const auto entityId = entity.GetId();

  entityComponentSignatures[entityId].set(componentId, false);

  spdlog::info("Component id =  " + std::to_string(componentId) +
               " was removed to entity id " + std::to_string(entityId));
}

template <typename TComponent>
TComponent &
Registry::GetComponent(Entity entity) const {
  const auto componentId = Component<TComponent>::GetId();
  const auto entityId = entity.GetId();

  auto componentPool =
      std::static_pointer_cast<Pool<TComponent>>(componentPools[componentId]);

  return componentPool->Get(entityId);
}

template <typename TComponent>
TComponent &
Entity::GetComponent() const {
  return registry->GetComponent<TComponent>(*this);
}

template <typename TComponent, typename... TArgs>
void
Entity::AddComponent(TArgs &&...args) {
  registry->AddComponent<TComponent>(*this, std::forward<TArgs>(args)...);
}

template <typename TComponent>
bool
Entity::HasComponent() const {
  return registry->HasComponent<TComponent>(*this);
}

template <typename TComponent>
void
Entity::RemoveComponent() {
  registry->RemoveComponent<TComponent>(*this);
}

#endif