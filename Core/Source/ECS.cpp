// #include "ECS.h"
// #include "Entity.h"



// template<typename... T>
// std::vector<EntityID> ECS::view(){

// }



// void ECS::update()
// {
//     for (auto e : m_toAdd)
//     {
//             m_entities.push_back(e);
//             m_entityMap[e->tag()].push_back(e);
//     }
//     m_toAdd.clear();

//     removeDeadEntities(m_entities);
//     for ( auto& [tag, entityVec] : m_entityMap)
//     {
//         removeDeadEntities(entityVec);
//     }
// }

// void ECS::sort()
// {
//     std::sort(m_entities.begin(), m_entities.end(), [](const std::shared_ptr<Entity> &a, const std::shared_ptr<Entity> &b) {
//         return a->layer() > b->layer();
//     });

// }