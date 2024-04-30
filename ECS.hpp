#pragma once
#ifndef ECS_H
#define ECS_H

// Include
#include <array>
#include <bitset>
#include <cassert>
#include <memory>
#include <queue>
#include <set>
#include <string>
#include <unordered_map>

// Figure out cpu word size
#if defined(_WIN32) || defined(_WIN64)
#	if defined(_WIN64)
#		define ECS64
#	else
#		define ECS32
#	endif
#endif
#if defined(__GNUC__)
#	if defined(__x86_64__) || defined(__ppc64__)
#		define ECS64
#	else
#		define ECS32
#	endif
#endif

// Get component data type
#if !defined(ECS_COMPONENT_TYPE)
#	if defined(ECS64)
#		define ECS_COMPONENT_TYPE std::uint64_t
#	elif defined(ECS32)
#		define ECS_COMPONENT_TYPE std::uint32_t
#	else
#		error Failed to detect cpu word size!
#	endif
#endif

// Get entity data type
#if !defined(ECS_ENTITY_TYPE)
#	if defined(ECS64)
#		define ECS_ENTITY_TYPE std::uint64_t
#	elif defined(ECS32)
#		define ECS_ENTITY_TYPE std::uint32_t
#	else
#		error Failed to detect cpu word size!
#	endif
#endif

// Get max possible components
#if !defined(ECS_COMPONENT_MAX)
#	define ECS_COMPONENT_MAX 32
#endif

// Get max possible entities
#if !defined(ECS_ENTITY_MAX)
#	define ECS_ENTITY_MAX 5000
#endif

// Namespace ECS = Entity Component System
// Based on: https://austinmorlan.com/posts/entity_component_system/
namespace ECS
{
	#pragma region Configuration

	// Component
	typedef ECS_COMPONENT_TYPE ComponentType;

	// Entity
	typedef ECS_ENTITY_TYPE Entity;

	// Max possible components
	static constexpr ComponentType COMPONENT_MAX = ECS_COMPONENT_MAX;

	// Max possible entities
	static constexpr Entity ENTITY_MAX = ECS_ENTITY_MAX;

	// Signature
	typedef std::bitset<COMPONENT_MAX> Signature;

	#pragma endregion Configuration

	#pragma region Entity

	// Entity manager
	class EntityManager final
	{
	public:

		// Constructor
		EntityManager():
			entitiesAvailable_(std::queue<Entity>()),
			entitiesLiving_(0),
			signatures_(std::array<Signature, ENTITY_MAX>())
		{
			// Fill queue with available entities
			for (Entity entity = 0; entity < ENTITY_MAX; ++entity)
				entitiesAvailable_.push(entity);
		}

		// Destructor
		~EntityManager()
		{}

		// Create entity
		Entity create()
		{
			// Check bounds
			assert(entitiesLiving_ < ENTITY_MAX && "Entity for construction out of range!");

			// Get entity from queue
			const auto entity = entitiesAvailable_.front();

			// Remove entity from queue
			entitiesAvailable_.pop();

			// Increment counter
			++entitiesLiving_;

			// Done
			return entity;
		};

		// Destroy entity
		void destroy(Entity entity)
		{
			// Check bounds
			assert(entity < ENTITY_MAX && "Entity for destruction out of range!");

			// Invalidate signature
			signatures_[entity].reset();

			// Put destroyed entity into the queue
			entitiesAvailable_.push(entity);

			// Decrement counter
			--entitiesLiving_;
		};

		// Get signature
		Signature signature(Entity entity)
		{
			// Check bounds
			assert(entity < ENTITY_MAX && "Entity for signature read out of range!");

			// Get signature from array
			return signatures_[entity];
		};

		// Set signature
		void signature(Entity entity, Signature signature)
		{
			// Check bounds
			assert(entity < ENTITY_MAX && "Entity for signature write out of range!");

			// Put signature into the array
			signatures_[entity] = signature;
		};

	private:

		// Entities that are available to be used
		std::queue<Entity> entitiesAvailable_;

		// Sum of living entities
		Entity entitiesLiving_;

		// Signatures
		std::array<Signature, ENTITY_MAX> signatures_;
	};

	#pragma endregion Entity

	#pragma region Component

	// Component base
	class ComponentBase
	{
	public:

		// Destructor
		virtual ~ComponentBase() = default;

		// Destroyed given entity
		virtual void destroyed(Entity entity) = 0;
	};

	// Component data
	template<typename T> class ComponentContainer final : public ComponentBase
	{
	public:

		// Constructor
		ComponentContainer():
			components_(std::array<T, ENTITY_MAX>),
			mapEntityToIndex_(std::unordered_map<Entity, std::size_t>()),
			mapIndexToEntity_(std::unordered_map<std::size_t, Entity>()),
			size_(0)
		{};

		// Destructor
		~ComponentContainer() override
		{};

		// Destroyed given entity
		void destroyed(Entity entity) override
		{
			// Entity not found
			if (mapEntityToIndex_.find(entity) == mapEntityToIndex_.end())
				return;

			// Remove entity
			remove(entity);
		};

		// Get component data for entity
		T& get(Entity entity)
		{
			// Check bounds
			assert(mapEntityToIndex_.find(entity) != mapEntityToIndex_.end() && "Entity does not exist!");

			// Return a reference to the entity's component
			return components_[mapEntityToIndex_[entity]];
		};

		// Insert component for entity
		void insert(Entity entity, T component)
		{
			// Check bounds
			assert(mapEntityToIndex_.find(entity) == mapEntityToIndex_.end() && "Component added to same entity more than once!");

			// Get new index
			const auto index = size_;

			// Set index and entity to the maps
			mapEntityToIndex_[entity] = index;
			mapIndexToEntity_[index] = entity;

			// Set component to array
			components_[index] = component;

			// Increment size
			++size_;
		};

		// Remove components for entity
		void remove(Entity entity)
		{
			// Check bounds
			assert(mapEntityToIndex_.find(entity) != mapEntityToIndex_.end() && "Removing non-existent component!");

			// Get index for removed entity
			const auto indexRemoved = mapEntityToIndex_[entity];

			// Get last index
			const auto indexLast = (size_ - 1);

			// Move last component to the spot of removed component
			components_[indexRemoved] = components_[indexLast];

			// Get last entity
			const auto entityLast = mapIndexToEntity_[indexLast];

			// Update map to point to moved spot
			mapEntityToIndex_[entityLast] = indexRemoved;
			mapIndexToEntity_[indexRemoved] = entityLast;

			// Erase index and entity from maps
			mapEntityToIndex_.erase(entity);
			mapIndexToEntity_.erase(indexLast);

			// Decrement counter
			--size_;
		};

	private:

		// Components
		std::array<T, ENTITY_MAX> components_;

		// Map entity to index
		std::unordered_map<Entity, std::size_t> mapEntityToIndex_;

		// Map index to entity
		std::unordered_map<std::size_t, Entity> mapIndexToEntity_;

		// Total size
		std::size_t size_;
	};

	// Component manager
	class ComponentManager final
	{
	public:

		// Add component for entity
		template<typename T> void add(Entity entity, T component)
		{
			getContainer_<T>()->insert(entity, component);
		};

		// Destroyed given entity
		void destroyed(Entity entity)
		{
			// Tell all components, that the entity has been destroyed
			for (const auto& pair : componentContainer_)
			{
				const auto& component = pair.second;
				component->destroyed(entity);
			}
		}

		// Get component for entity
		template<typename T> T& get(Entity entity)
		{
			return getContainer_<T>()->get(entity);
		};

		// Get component type
		template<typename T> ComponentType getType()
		{
			// Get type as string
			const auto type = typeid(T).name();

			// Check bounds
			assert(componentTypes_.find(type) != componentTypes_.end() && "Component not installed before use!");

			// Return type
			return componentTypes_[type];
		};

		// Install a new component
		template<typename T> void install()
		{
			// Get type as string
			const auto type = typeid(T).name();

			// Check bounds
			assert(componentTypes_.find(type) == componentTypes_.end() && "Installing component type more than once!");

			// Add type to types
			componentTypes_.insert(std::pair<const char*, ComponentType>(type, nextType_));

			// Add component to container
			componentContainer_.insert(std::pair<const char*, ComponentType>(type, std::make_shared<ComponentContainer<T>>()));

			// Increment next type
			++nextType_;
		}

		// Remove component from entity
		template<typename T> void remove(Entity entity)
		{
			getContainer_<T>()->remove(entity);
		};

	private:

		// Map of component types
		std::unordered_map<const char*, ComponentType> componentTypes_;

		// Map component container
		std::unordered_map<const char*, std::shared_ptr<ComponentBase>> componentContainer_;

		// Next type
		ComponentType nextType_;

		// Get container
		template<typename T> std::shared_ptr<ComponentContainer<T>> getContainer_()
		{
			// Get type as string
			const auto type = typeid(T).name();

			// Check bounds
			assert(componentTypes_.find(type) != componentTypes_.end() && "Component not installed before use!");

			// Return pointer from container with type
			return std::static_pointer_cast<ComponentContainer<T>>(componentContainer_[type]);
		};
	};

	#pragma endregion Component

	#pragma region System

	// System
	class System
	{
	public:

		// Entity set
		std::set<Entity> entities;
	};

	// System manager
	class SystemManager final
	{
	public:

		// Constructor
		SystemManager():
			signatures_(),
			systems_()
		{}

		// Destructor
		~SystemManager()
		{}

		// Entity destroyed
		void entityDestroyed(Entity entity)
		{
			for (const auto& pair : systems_)
			{
				const auto& system = pair.second;
				system->entities.erase(entity);
			}
		};

		// Install a new system
		template<typename T> std::shared_ptr<T> install()
		{
			// Get type as string
			const auto type = typeid(T).name();

			// Check bounds
			assert(systems_.find(type) == systems_.end() && "Installing new system more than once!");

			// Instantiate new system on heap
			const auto system = std::make_shared<T>();

			// Add system to map
			systems_.insert(std::pair<const char*, std::shared_ptr<System>>(type, system));

			// Done
			return system;
		};

		// Set signature
		template<typename T> void signature(Signature signature)
		{
			// Get type as string
			const auto type = typeid(T).name();

			// Check bounds
			assert(systems_.find(type) != systems_.end() && "System used before installed!");

			// Insert signature for type into map
			signatures_.insert(std::pair<const char*, Signature>(type, signature));
		};

		// Signature changed
		template<typename T> void signatureChanged(Entity entity, Signature signatureEntity)
		{
			// Iterate over systems
			for (const auto& pair : systems_)
			{
				// Get type
				const auto type = pair.first;

				// Get system
				const auto system = pair.second;

				// Get signature
				const auto signatureSystem = signatures_[type];

				// Signature matches, insert entity
				if ((signatureEntity && signatureSystem) == signatureSystem)
					system->entities.insert(entity);

				// Signature mismatches, erase entity
				else
					system->entities.erase(entity);
			}
		};

	private:

		// Signatures
		std::unordered_map<const char*, Signature> signatures_;

		// Systems
		std::unordered_map<const char*, std::shared_ptr<System>> systems_;
	};

	#pragma endregion System

	#pragma region Registry

	// Registry
	class Registry final
	{
	public:

		// Constructor
		Registry():
			componentManager_(std::make_unique<ComponentManager>()),
			entityManager_(std::make_unique<EntityManager>()),
			systemManager_(std::make_unique<SystemManager>())
		{};

		// Destructor
		~Registry()
		{};

		// Create entity
		Entity entityCreate()
		{
			return entityManager_->create();
		};

		// Destroy entity
		void entityDestroy(Entity entity)
		{
			entityManager_->destroy(entity);
			componentManager_->destroyed(entity);
			systemManager_->entityDestroyed(entity);
		};

		// Add component
		template<typename T> void componentAdd(Entity entity, T component)
		{
			// Install component
			componentManager_->install<T>();

			// Get signature from entity
			auto signature = entityManager_->signature(entity);

			// Add component to signature
			signature.set(componentManager_->getType<T>(), true);

			// Set signature for entity
			entityManager_->signature(entity, signature);

			// Notify signature changed for entity
			systemManager_->signatureChanged(entity, signature);
		};

		// Get component
		template<typename T> T& componentGet(Entity entity)
		{
			return componentManager_->get<T>(entity);
		};

		// Install new component
		template<typename T> void componentInstall()
		{
			componentManager_->install<T>();
		};

		// Remove component from entity
		template<typename T> void componentRemove(Entity entity)
		{
			// Remove component for entity
			componentManager_->remove<T>(entity);

			// Get signature from entity
			auto signature = entityManager_->signature(entity);

			// Remove component from signature
			signature.set(componentManager_->getType<T>(), false);

			// Set signature for entity
			entityManager_->signature(entity, signature);

			// Notify signature changed for entity
			systemManager_->signatureChanged(entity, signature);
		};

		// Get component type
		template<typename T> ComponentType componentType()
		{
			return componentManager_->getType<T>();
		};

		// Install a new system
		template<typename T> std::shared_ptr<T> systemInstall()
		{
			return systemManager_->install<T>();
		};

		// Set signature for system
		template<typename T> void systemSignature(Signature signature)
		{
			systemManager_->signature<T>(signature);
		};

	private:

		// Component manager
		std::unique_ptr<ComponentManager> componentManager_;

		// Entity manager
		std::unique_ptr<EntityManager> entityManager_;

		// System manager
		std::unique_ptr<SystemManager> systemManager_;
	};

	#pragma endregion Registry

}

#endif
