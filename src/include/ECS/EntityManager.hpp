#pragma once

#include <array>
#include <cassert>
#include <queue>
#include "Types.hpp"
#include "ComponentManager.hpp"
class EntityManager
{
public:
	EntityManager()
	{
		for (EntityID entity = 0; entity < MAX_ENTITIES; ++entity)
		{
			mAvailableEntities.push(entity);
		}
	}

	EntityID CreateEntity()
	{
		assert(mLivingEntityCount < MAX_ENTITIES && "Too many entities in existence.");

		EntityID id = mAvailableEntities.front();
		mAvailableEntities.pop();
		++mLivingEntityCount;

		return id;
	}

	void DestroyEntity(EntityID entity)
	{
		assert(entity < MAX_ENTITIES && "EntityID out of range.");

		mSignatures[entity].reset();
		mAvailableEntities.push(entity);
		--mLivingEntityCount;

        m_ComponentManager->EntityDestroyed(entity);
	}

	void SetSignature(EntityID entity, Signature signature)
	{
		assert(entity < MAX_ENTITIES && "EntityID out of range.");

		mSignatures[entity] = signature;
	}

	Signature GetSignature(EntityID entity)
	{
		assert(entity < MAX_ENTITIES && "EntityID out of range.");

		return mSignatures[entity];
	}

    	// Component methods
	template<typename T>
	void RegisterComponent()
	{
		m_ComponentManager->RegisterComponent<T>();
	}

	template<typename T>
	void AddComponent(EntityID entity, T component)
	{
		m_ComponentManager->AddComponent<T>(entity, component);

		auto signature = GetSignature(entity);
		signature.set(m_ComponentManager->GetComponentType<T>(), true);
		SetSignature(entity, signature);
	}

	template<typename T>
	void RemoveComponent(EntityID entity)
	{
		m_ComponentManager->RemoveComponent<T>(entity);

		auto signature = GetSignature(entity);
		signature.set(m_ComponentManager->GetComponentType<T>(), false);
		SetSignature(entity, signature);
	}

	template<typename T>
	T& GetComponent(EntityID entity)
	{
		return m_ComponentManager->GetComponent<T>(entity);
	}

	template<typename T>
	ComponentType GetComponentType()
	{
		return m_ComponentManager->GetComponentType<T>();
	}

private:
	std::queue<EntityID> mAvailableEntities{};
	std::array<Signature, MAX_ENTITIES> mSignatures{};
	uint32_t mLivingEntityCount{};

    std::unique_ptr<ComponentManager> m_ComponentManager;
};
