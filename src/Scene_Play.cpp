#include "Scene_Menu.h"
#include "Scene_Play.h"
#include "Scene_GameOver.h"
#include "Scene_Inventory.h"
#include "Sprite.h"
#include "Assets.h"
#include "Game.h"
#include "Components.h"
#include "Action.h"
#include "Level_Loader.h"
#include "Camera.h"
#include "ScriptableEntity.h"
#include "scripts/player.cpp"
#include "scripts/npc.cpp"
#include "scripts/rooter.cpp"
#include "scripts/weapon.cpp"
#include "scripts/projectile.cpp"
#include "scripts/coin.cpp"

#include "RandomArray.h"

#include <SDL_image.h>
#include <SDL_mixer.h>

#include <iostream>
#include <string>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>

Scene_Play::Scene_Play(Game* game, std::string levelPath, bool newGame)
    : Scene(game), m_levelPath(levelPath), m_collisionManager(&m_ECS, this), m_newGame(newGame)
{
    registerAction(SDLK_w, "UP");
    registerAction(SDLK_UP, "UP");
    registerAction(SDLK_s, "DOWN");
    registerAction(SDLK_DOWN, "DOWN");
    registerAction(SDLK_a, "LEFT");
    registerAction(SDLK_LEFT, "LEFT");
    registerAction(SDLK_d, "RIGHT");
    registerAction(SDLK_RIGHT, "RIGHT");
    
    registerAction(SDLK_e, "INVENTORY");
    // registerAction(SDLK_XXX, "USE");
    registerAction(SDL_BUTTON_LEFT , "ATTACK");
    registerAction(SDL_MOUSEWHEEL , "ATTACK");
    registerAction(SDL_MOUSEWHEEL_NORMAL , "SCROLL");
    registerAction(SDLK_SPACE , "ATTACK");
    registerAction(SDLK_LSHIFT, "SHIFT");
    registerAction(SDLK_LCTRL, "CTRL");
    registerAction(SDLK_ESCAPE, "ESC");
    registerAction(SDLK_u, "SAVE");
    registerAction(SDLK_r, "RESET");

    registerAction(SDLK_f, "CAMERA FOLLOW");
    registerAction(SDLK_z, "CAMERA PAN");
    registerAction(SDLK_PLUS, "ZOOM IN");
    registerAction(SDLK_MINUS, "ZOOM OUT");
    registerAction(SDLK_q, "WRITE QUADTREE");
    registerAction(SDLK_p, "PAUSE");
    registerAction(SDLK_k, "KILL_PLAYER");
    registerAction(SDLK_t, "TOGGLE_TEXTURE");
    registerAction(SDLK_c, "TOGGLE_COLLISION");
    registerAction(SDLK_F3, "TOGGLE_COLLISION");
    registerAction(SDLK_i, "TOGGLE_INTERACTION");
    registerAction(SDLK_F4, "TOGGLE_INTERACTION");
    registerAction(SDLK_1, "TP1");
    registerAction(SDLK_2, "TP2");
    registerAction(SDLK_3, "TP3");

    loadConfig("config_files/config.txt");
    spawnPlayer();
    spawnNPC(Vec2{353, 63});
    loadLevel(levelPath); 
    loadMobsNItems("config_files/mobs.txt"); // mobs have to spawn after player, so they can target the player
    spawnWeapon(Vec2{364, 91}*m_gridSize, 7);
    spawnSword(Vec2{345, 60}*m_gridSize, 7);

    m_camera.calibrate(Vec2 {(float)width(), (float)height()}, m_levelSize, m_gridSize);
    m_inventory_scene =  std::make_shared<Scene_Inventory>(m_game);
}

void Scene_Play::loadMobsNItems(const std::string& path){
    std::ifstream file(path);
    if (!file) {
        std::cerr << "Could not load mobs.txt file!\n";
        exit(-1);
    }
    std::string head;
    Vec2 pos;
    int layer;
    while (file >> head) {
        file >>  pos.x >> pos.y >> layer;           
        if (head == "rooter") {
            spawnSmallEnemy(pos*m_gridSize, layer, "rooter-sheet");
        }
        else if (head == "goblin") {
            spawnSmallEnemy(pos*m_gridSize, layer, "goblin-sheet");
        }
        else if (head == "coin") {
            spawnCoin(pos*m_gridSize, layer);
        }
        else if (head == "tree") {
            spawnDecoration(pos*m_gridSize, Vec2 {24/4, 32/4}, layer, "tree");
        }
        else if (head == "House1") {
            spawnDecoration(pos*m_gridSize, Vec2 {56, 44}, layer, "House1");
        }
        else if (head == "campfire") {
            spawnCampfire(pos*m_gridSize, layer);
        }
        else {
            std::cerr << "The mobs file format is incorrect!\n";
            exit(-1);
        }
    }
}

void Scene_Play::loadConfig(const std::string& confPath){
    std::ifstream file(confPath);
    if (!file) {
        std::cerr << "Could not load config.txt file!\n";
        exit(-1);
    }
    std::string head;
    while (file >> head) {
        if (head == "Player") {
            file >> m_playerConfig.x >> m_playerConfig.y >> m_playerConfig.SPEED >> m_playerConfig.MAXSPEED >> m_playerConfig.HP >> m_playerConfig.DAMAGE;           
        }
        else if (head == "Rooter") {
            file >> m_rooterConfig.SPEED >> m_rooterConfig.ATTACK_SPEED >> m_rooterConfig.HP >> m_rooterConfig.DAMAGE;           
        }
        else if (head == "Goblin") {
            file >> m_goblinConfig.SPEED >> m_goblinConfig.ATTACK_SPEED >> m_goblinConfig.HP >> m_goblinConfig.DAMAGE;           
        }
        else if (head == "Camera") {
            file >> m_camera.config.SHAKE_DURATION_SMALL >> m_camera.config.SHAKE_INTENSITY_SMALL;           
        }
        else {
            std::cerr << "head to " << head << "\n";
            std::cerr << "The config file format is incorrect!\n";
            exit(-1);
        }
    }
}

// Function to save the game state to a file
void Scene_Play::saveGame(const std::string& filename) {
    std::ofstream saveFile(filename);

    if (saveFile.is_open()) {
        saveFile << "Player_pos " << (int)(m_ECS.getComponent<CTransform>(m_player).pos.x/m_gridSize.x) << " " << (int)(m_ECS.getComponent<CTransform>(m_player).pos.y/m_gridSize.y) << std::endl;
        saveFile << "Player_hp " << m_ECS.getComponent<CHealth>(m_player).HP << std::endl;
        saveFile.close();
    } else {
        std::cerr << "Unable to open file for saving!" << std::endl;
    }
}

void Scene_Play::loadLevel(const std::string& levelPath){

    const char* path = levelPath.c_str();
    SDL_Surface* loadedSurface = IMG_Load(path);
    if (loadedSurface == nullptr) 
    {
        std::cerr << "Unable to load image " << path << "! SDL_image Error: " << IMG_GetError() << std::endl;
    }

    // Lock the surface to access the pixels
    SDL_LockSurface(loadedSurface);
    Uint32* pixels = (Uint32*)loadedSurface->pixels;

    const int HEIGHT_PIX = loadedSurface->h;
    const int WIDTH_PIX = loadedSurface->w;
    m_levelSize = Vec2{ (float)WIDTH_PIX, (float)HEIGHT_PIX };
    m_pixelMatrix = m_levelLoader.createPixelMatrix(pixels, loadedSurface->format, WIDTH_PIX, HEIGHT_PIX);
    // Unlock and free the surface
    SDL_UnlockSurface(loadedSurface);
    SDL_FreeSurface(loadedSurface);
    m_levelLoader.init(this, WIDTH_PIX, HEIGHT_PIX);
    m_levelLoader.loadChunk(m_currentChunk);
}

void Scene_Play::sDoAction(const Action& action) {
    if ( action.type() == "START") {
        if ( action.name() == "TOGGLE_TEXTURE") {
            m_drawTextures = !m_drawTextures; 
        } else if ( action.name() == "TOGGLE_COLLISION") { 
            m_drawCollision = !m_drawCollision; 
        } else if ( action.name() == "TOGGLE_INTERACTION") { 
            m_drawInteraction = !m_drawInteraction; 
        } else if ( action.name() == "TOGGLE_GRID") { 
            m_drawDrawGrid = !m_drawDrawGrid; 
        } else if ( action.name() == "PAUSE") { 
            setPaused( !m_pause || m_inventoryOpen );
        } else if ( action.name() == "INVENTORY") { 
            m_inventoryOpen = !m_inventoryOpen;
            m_inventory_scene->toggleInventory();
            setPaused( !m_pause || m_inventoryOpen );
        } else if ( action.name() == "SCROLL"){
            if ( m_inventoryOpen )
            {
                m_inventory_scene->Scroll(m_mouseState.scroll);
            }
        } else if ( action.name() == "ZOOM IN"){
            m_camera.stepCameraZoom(-1, m_game->getScale());
        } else if ( action.name() == "ZOOM OUT"){
            m_camera.stepCameraZoom(1, m_game->getScale());
        } else if ( action.name() == "CAMERA FOLLOW"){
            m_camera.toggleCameraFollow();
        } else if ( action.name() == "CAMERA PAN"){
            m_pause = m_camera.startPan(2048, 1000, Vec2{0,0}, m_pause);
        } else if ( action.name() == "SAVE"){
            saveGame("config_files/game_save.txt");
        } else if ( action.name() == "TP1") { 
            m_ECS.getComponent<CTransform>(m_player).pos = Vec2{460*64/4, 460*64/4};
        } else if ( action.name() == "TP2") { 
            m_ECS.getComponent<CTransform>(m_player).pos = Vec2{292*64/4, 236*64/4};
        }else if ( action.name() == "TP3") {
            m_ECS.getComponent<CTransform>(m_player).pos = Vec2{801*64/4, 181*64/4};
        } else if ( action.name() == "RESET") { 
            m_game->changeScene("PLAY", std::make_shared<Scene_Play>(m_game, "assets/images/levels/levelStartingArea.png", true), true);
        } else if ( action.name() == "KILL_PLAYER") { 
            m_ECS.getComponent<CHealth>(m_player).HP = 0;
        }

        if ( action.name() == "UP") { m_ECS.getComponent<CInputs>(m_player).up = true; }
        if ( action.name() == "DOWN") { m_ECS.getComponent<CInputs>(m_player).down = true;  }
        if ( action.name() == "LEFT") { m_ECS.getComponent<CInputs>(m_player).left = true; }
        if ( action.name() == "RIGHT") { m_ECS.getComponent<CInputs>(m_player).right = true; }
        if ( action.name() == "SHIFT") { m_ECS.getComponent<CInputs>(m_player).shift = true; }
        if ( action.name() == "CTRL") { m_ECS.getComponent<CInputs>(m_player).ctrl = true; }

        if ( action.name() == "ATTACK"){
            m_ECS.getComponent<CScript>(m_player).Instance->OnAttackFunction();
        }
    }
    else if ( action.type() == "END") {
        if ( action.name() == "DOWN") { m_ECS.getComponent<CInputs>(m_player).down = false; }
        if ( action.name() == "UP") { m_ECS.getComponent<CInputs>(m_player).up = false; }
        if ( action.name() == "LEFT") { m_ECS.getComponent<CInputs>(m_player).left = false; }
        if ( action.name() == "RIGHT") { m_ECS.getComponent<CInputs>(m_player).right = false; }
        if ( action.name() == "SHIFT") { m_ECS.getComponent<CInputs>(m_player).shift = false; }
        if ( action.name() == "CTRL") { m_ECS.getComponent<CInputs>(m_player).ctrl = false; }
        if ( action.name() == "ESC") {
            m_game->changeScene("SETTINGS", std::make_shared<Scene_Pause>(m_game), false);
            saveGame("config_files/game_save.txt");
            m_pause = true;
        }
        if ( action.name() == "WRITE QUADTREE")
        {
            std::cout << "Collisions:" << std::endl;
            m_physics.m_quadRoot->printTree("", "");
            std::cout << "Interactions:" << std::endl;
            m_physics.m_interactionQuadRoot->printTree("", "");
        }
    }
}

void Scene_Play::update() 
{
    m_pause = m_camera.update(m_ECS.getComponent<CTransform>(m_player).pos, m_pause);
    if (!m_pause) 
    {
        sLoader();
        sScripting();
        sMovement();
        sStatus();
        sCollision();
        sInteraction();
        sAnimation();
        sAudio();
        m_currentFrame++;
    }
    sRender();
    m_ECS.update();
    m_inventory_scene->update();
    if (m_restart) {
        // std::cerr << "Player entity is not initialized!" << std::endl;
        m_game->changeScene("GAMEOVER", std::make_shared<Scene_GameOver>(m_game), true);
        return;
    }
}

void Scene_Play::sLoader()
{
    m_currentChunk = ( ( (m_ECS.getComponent<CTransform>(m_player).pos / m_gridSize).toInt() ) / m_chunkSize ).toInt();
    for (int dx = -3; dx <= 3; ++dx) 
    {
        for (int dy = -2; dy <= 2; ++dy) 
        {
            Vec2 neighborChunk = {m_currentChunk.x + dx, m_currentChunk.y + dy};
            if ( neighborChunk.smaller(Vec2{0,0}) || neighborChunk.greater(m_levelSize/m_chunkSize) )
            {
                continue;
            }
            if (std::find(m_loadedChunks.begin(), m_loadedChunks.end(), neighborChunk) == m_loadedChunks.end())
            {
                EntityID chunkID = m_levelLoader.loadChunk(neighborChunk);
                m_loadedChunkIDs.push_back(chunkID);
                m_loadedChunks.push_back(neighborChunk);
            }
        }
    }
    m_levelLoader.clearChunks(35); // Will leave x number of chunks
}

void Scene_Play::sScripting() 
{
    auto view = m_ECS.signatureView<CScript>();
    auto& scriptPool = m_ECS.getComponentPool<CScript>();
    for ( auto e : view ) 
    {
        auto& sc = scriptPool.getComponent(e);
        sc.Instance->OnUpdateFunction();
        // memory leak, destroy
    }
}

void Scene_Play::sMovement() {
    auto& transformPool = m_ECS.getComponentPool<CTransform>();

    auto& pathfindPool = m_ECS.getComponentPool<CPathfind>();
    auto viewPathfind = m_ECS.signatureView<CPathfind, CTransform>();
    for (auto e : viewPathfind)
    {
        auto& transform = transformPool.getComponent(e);
        auto& pathfind = pathfindPool.getComponent(e);
        if ((pathfind.target - transform.pos).length() < 16*2) {
            transform.vel = pathfind.target - transform.pos;
        } else {
            transform.vel = Vec2 {0,0};
        }
        pathfind.target = transformPool.getComponent(m_player).pos;
    }

    auto& inputPool = m_ECS.getComponentPool<CInputs>();
    auto& viewInputs = m_ECS.view<CInputs>();
    for (auto e : viewInputs){
        auto &transform = transformPool.getComponent(e);
        auto &inputs = inputPool.getComponent(e);

        if ( e == m_player ){
            transform.vel = { 0,0 };
            if (inputs.up){
                transform.vel.y--;
            } if (inputs.down){
                transform.vel.y++;
            } if (inputs.left){
                transform.vel.x--;
            } if (inputs.right){
                transform.vel.x++;
            } if (inputs.shift){
                transform.tempo = 0.5f;
            } else if (inputs.ctrl){
                transform.tempo = 2.0f;
            } else{
                transform.tempo = 1.0f;
            }

            if (m_ECS.hasComponent<CSwimming>(m_player)){
                transform.tempo *= m_ECS.getComponent<CSwimming>(m_player).swimSpeedMultiplier;
            }
        }
    }

    // auto viewKnockback = m_ECS.signatureView<CKnockback, CTransform>();
    // auto& knockbackPool = m_ECS.getComponentPool<CKnockback>();
    // for (auto entityKnockback : viewKnockback){    
    //     auto &transform = transformPool.getComponent(entityKnockback);
    //     auto& knockback = knockbackPool.getComponent(entityKnockback);
    //     transform.pos += m_physics.knockback(knockback);
    //     if (knockback.duration <= 0){
    //         m_ECS.queueRemoveComponent<CKnockback>(entityKnockback);
    //     }
    // }       

    auto& viewTransform = m_ECS.view<CTransform>();
    for (auto e : viewTransform){    
        auto &transform = transformPool.getComponent(e);
        
        // Update position
        transform.prevPos = transform.pos;
        if (!(transform.vel.isNull()) && transform.isMovable ){
            transform.pos += transform.vel.norm(transform.tempo*transform.speed/m_game->framerate());
        }
        
    }

    auto& viewParent = m_ECS.view<CParent>();
    auto& parentPool = m_ECS.getOrCreateComponentPool<CParent>();
    for (auto e : viewParent)
    {
        auto& transform = transformPool.getComponent(e);
        auto& parent = parentPool.getComponent(e);
        transform.pos = transformPool.getComponent(parent.parent).pos + parent.relativePos;
    }

}

void Scene_Play::playerCollisions()
{
    auto& transformPool = m_ECS.getComponentPool<CTransform>();
    auto& collisionPool = m_ECS.getComponentPool<CCollisionBox>();
    auto& transformPlayer = transformPool.getComponent(m_player);
    auto& collisionPlayer = collisionPool.getComponent(m_player);

    auto& viewLoot = m_ECS.getComponentPool<CLoot>();
    for ( auto e : viewLoot ){
        auto& transform = transformPool.getComponent(e);
        auto& collision = collisionPool.getComponent(e);
        if ( m_physics.isCollided(transformPlayer, collisionPlayer, transform, collision) )
        {
            m_ECS.queueRemoveEntity(e);
            Mix_PlayChannel(-1, m_game->assets().getAudio("loot_pickup"), 0);
        }
    }

    auto& viewWeapon = m_ECS.getComponentPool<CWeapon>();
    for ( auto e : viewWeapon ){
        auto& transform = transformPool.getComponent(e);
        auto& collision = collisionPool.getComponent(e);
        if ( m_physics.isCollided(transformPlayer, collisionPlayer, transform, collision) )
        {
            m_ECS.queueRemoveComponent<CCollisionBox>(e);
            m_ECS.queueRemoveComponent<CWeapon>(e);
            m_ECS.addComponent<CWeaponChild>(m_player, e);
            m_ECS.addComponent<CParent>(e, m_player, Vec2{8, -4});
            Mix_PlayChannel(-1, m_game->assets().getAudio("loot_pickup"), 0);
        }
    }

    auto& scriptPool = m_ECS.getComponentPool<CScript>();
    auto viewScript = m_ECS.signatureView<CScript, CCollisionBox>();
    for ( auto e : viewScript ){
        auto& transform = transformPool.getComponent(e);
        auto& collision = collisionPool.getComponent(e);
        if (m_physics.isCollided(transformPlayer, collisionPlayer, transform, collision))
        {
            auto& sc = scriptPool.getComponent(e);
            sc.Instance->OnInteractFunction();
        }
    }

    auto &viewImmovable = m_ECS.view<CImmovable>();
    for ( auto e : viewImmovable ){
        auto& transform = transformPool.getComponent(e);
        auto& collision = collisionPool.getComponent(e);
        if (m_physics.isCollided(transformPlayer, collisionPlayer, transform, collision))
        {
            Vec2 overlap = m_physics.overlap(transformPlayer, collisionPlayer, transform, collision);
            if ( m_ECS.hasComponent<CChild>(m_player) )
            {
                EntityID childID = m_ECS.getComponent<CChild>(m_player).childID;
                m_ECS.getComponent<CTransform>(childID).pos += overlap;
            }      
            m_ECS.getComponent<CTransform>(m_player).pos += overlap;
        }
    }
}

void Scene_Play::enemyCollisions()
{
    auto& transformPool = m_ECS.getComponentPool<CTransform>();
    auto& collisionPool = m_ECS.getComponentPool<CCollisionBox>();
    // auto& transformPlayer = transformPool.getComponent(m_player);
    // auto& collisionPlayer = collisionPool.getComponent(m_player);
    auto& viewP = m_ECS.view<CPathfind>();
    for ( auto enemy : viewP )
    {
        auto& transformEnemy = transformPool.getComponent(enemy);
        auto& collisionEnemy = collisionPool.getComponent(enemy);
        for (auto enemy2 : viewP)
        {
            // if (enemy == enemy2) {continue;}
            auto& transformEnemy2 = transformPool.getComponent(enemy2);
            auto& collisionEnemy2 = collisionPool.getComponent(enemy2);
            if (m_physics.isCollided(transformEnemy, collisionEnemy, transformEnemy2, collisionEnemy2))
            {
                transformPool.getComponent(enemy).pos += m_physics.overlap(transformEnemy, collisionEnemy, transformEnemy2, collisionEnemy2);
            }
        }
        // if (m_physics.isCollided(transformEnemy, collisionEnemy, transformPlayer, collisionPlayer))
        //     {
        //         transformPool.getComponent(enemy).pos += m_physics.overlap(transformEnemy, collisionEnemy, transformPlayer, collisionPlayer);
        //         m_ECS.addComponent<CKnockback>(m_player, 120, 10, transformEnemy.vel);
        //     }
        auto &viewImmovable = m_ECS.view<CImmovable>();
        for ( auto e : viewImmovable ){
            auto& transform = transformPool.getComponent(e);
            auto& collision = collisionPool.getComponent(e);
            if (m_physics.isCollided(transformEnemy, collisionEnemy, transform, collision))
            {
                m_ECS.getComponent<CTransform>(enemy).pos += m_physics.overlap(transformEnemy, collisionEnemy, transform, collision);
            }
        }
    }
}

void Scene_Play::projectileCollisions()
{auto& transformPool = m_ECS.getComponentPool<CTransform>();
    auto& collisionPool = m_ECS.getComponentPool<CCollisionBox>();
    auto& healthPool = m_ECS.getComponentPool<CHealth>();
    std::vector<EntityID> viewSignaturecollision             = m_ECS.signatureView<CCollisionBox, CHealth>();
    std::vector<EntityID> viewSignatureImmovable        = m_ECS.signatureView<CCollisionBox, CImmovable>();
    std::vector<EntityID> viewSignatureProjectileState  = m_ECS.signatureView<CCollisionBox, CProjectileState>();
    for ( auto projectileID : viewSignatureProjectileState )
    {
        auto& transformProjectile = transformPool.getComponent(projectileID);
        auto& collisionProjectile = collisionPool.getComponent(projectileID);
        for ( auto enemyID : viewSignaturecollision )
        {   
            if (enemyID == m_player ){continue;}
            auto& transformEnemy = transformPool.getComponent(enemyID);
            auto& collisionEnemy = collisionPool.getComponent(enemyID);
            if (!m_physics.isCollided(transformProjectile, collisionProjectile, transformEnemy, collisionEnemy))
            {
                continue;
            }
            auto& animation     = m_ECS.getComponent<CAnimation>(projectileID);
            animation.animation = m_game->assets().getAnimation("fireball_explode");
            animation.repeat    = false;
            transformProjectile.isMovable = false;
            m_ECS.queueRemoveComponent<CCollisionBox>(projectileID);
            m_ECS.queueRemoveComponent<CDamage>(projectileID);
            auto& health = healthPool.getComponent(enemyID);
            health.HP--;
            health.damage_frame = m_currentFrame;
        }
        for ( auto enemyID : viewSignatureImmovable )
        {   
            auto& transformObstacle = transformPool.getComponent(enemyID);
            auto& collisionObstacle = collisionPool.getComponent(enemyID);
            if (!m_physics.isCollided(transformProjectile, collisionProjectile, transformObstacle, collisionObstacle))
            {
                continue;
            }
            auto& animation     = m_ECS.getComponent<CAnimation>(projectileID);
            animation.animation = m_game->assets().getAnimation("fireball_explode");
            animation.repeat    = false;
            transformProjectile.isMovable = false;
            m_ECS.queueRemoveComponent<CCollisionBox>(projectileID);
            m_ECS.queueRemoveComponent<CDamage>(projectileID);
            // m_ECS.addComponent<CParent>(projectileID, enemyID, Vec2{32, 0}); // Need to remove child if parent dies
        }
    }
}

void Scene_Play::sInteraction()
{
    auto screenSize = Vec2{(float)width(), (float)height()};
    Vec2 treePos = m_camera.position + screenSize/2 - Vec2{32, 32};
    Vec2 treeSize = Vec2{1048, 1048};
    m_physics.createInteractionQuadtree(treePos, treeSize);
    auto viewInteraction = m_ECS.signatureView<CInteractionBox, CTransform, CScript>();
    for ( auto e : viewInteraction ){
        Entity entity = {e, &m_ECS};
        m_physics.insertInteractionQuadtree(entity);
    }

    auto& interactionPool = m_ECS.getComponentPool<CInteractionBox>();
    auto& transformPool = m_ECS.getComponentPool<CTransform>();
    auto& scriptPool = m_ECS.getComponentPool<CScript>();
    auto quadVector = m_physics.createInteractionQuadtreeVector();
    for (auto quadleaf : quadVector){
        std::vector<Entity> entityVector = quadleaf->getObjects();

        for (size_t a = 0; a < entityVector.size(); ++a) {
            EntityID entityIDA = entityVector[a].getID();
            auto& interactionA = interactionPool.getComponent(entityIDA);

            
            for (size_t b = a + 1; b < entityVector.size(); ++b) {
                EntityID entityIDB = entityVector[b].getID();
                if ( entityIDA == entityIDB ) {
                    continue; // Skip self-collision
                }
                auto& interactionB = interactionPool.getComponent(entityIDB);
                auto& interactionLayerA = interactionA.layer;
                auto& interactionMaskA = interactionA.mask;
                auto& interactionLayerB = interactionB.layer;
                auto& interactionMaskB = interactionB.mask;
                if ( ((interactionLayerB & interactionMaskA) != interactionLayerB) | ((interactionLayerA & interactionMaskB) != interactionLayerA) ){
                    continue; // No interaction layer match
                }
                auto& transformA = transformPool.getComponent(entityIDA);
                auto& transformB = transformPool.getComponent(entityIDB);
                if ( !m_physics.isCollided(transformA, interactionA, transformB, interactionB) ) 
                {
                    continue; // No interaction detected
                }
                // std::cout << "interaction detected between entity " << entityIDA << " and entity " << entityIDB << std::endl;
                // auto overlap = m_physics.overlap(transformA, interactionA, transformB, interactionB);

                scriptPool.getComponent(entityIDA).Instance->OnInteractionCollisionFunction(entityIDB, interactionLayerB);
                scriptPool.getComponent(entityIDB).Instance->OnInteractionCollisionFunction(entityIDA, interactionLayerA);
                
            }
        }
    }
}

 
void Scene_Play::sCollision() {

    auto screenSize = Vec2{(float)width(), (float)height()};
    Vec2 treePos = m_camera.position + screenSize/2 - Vec2{32, 32};
    Vec2 treeSize = Vec2{1048, 1048};

    // m_physics.createQuadtree(treePos, treeSize);
    // auto viewCollision = m_ECS.signatureView<CCollisionBox, CTransform>();
    // for ( auto e : viewCollision ){
    //     Entity entity = {e, &m_ECS};
    //     m_physics.insertQuadtree(entity);
    // }

    m_collisionManager.newQuadtree(treePos, treeSize);
    m_collisionManager.doCollisions();

    // auto& collisionPool = m_ECS.getComponentPool<CCollisionBox>();
    // auto& transformPool = m_ECS.getComponentPool<CTransform>();
    // auto& scriptPool = m_ECS.getComponentPool<CScript>();
    // auto quadVector = m_physics.createQuadtreeVector();
    // for (auto quadleaf : quadVector){
    //     std::vector<Entity> entityVector = quadleaf->getObjects();

    //     for (size_t a = 0; a < entityVector.size(); ++a) {
    //         EntityID entityIDA = entityVector[a].getID();
    //         auto& collisionA = collisionPool.getComponent(entityIDA);

            
    //         for (size_t b = a + 1; b < entityVector.size(); ++b) {
    //             EntityID entityIDB = entityVector[b].getID();
    //             if ( entityIDA == entityIDB ) {
    //                 continue; // Skip self-collision
    //             }
    //             auto& collisionB = collisionPool.getComponent(entityIDB);
    //             auto& collisionLayerA = collisionA.layer;
    //             auto& collisionMaskA = collisionA.mask;
    //             auto& collisionLayerB = collisionB.layer;
    //             auto& collisionMaskB = collisionB.mask;
    //             if ( ((collisionLayerB & collisionMaskA) != collisionLayerB) | ((collisionLayerA & collisionMaskB) != collisionLayerA) ){
    //                 continue; // No collision layer match
    //             }
    //             auto& transformA = transformPool.getComponent(entityIDA);
    //             auto& transformB = transformPool.getComponent(entityIDB);
    //             auto overlap = m_physics.overlap(transformA, collisionA, transformB, collisionB);
    //             if ( !m_physics.isCollided(transformA, collisionA, transformB, collisionB) ) 
    //             {
    //                 continue; // No collision detected
    //             }
    //             // std::cout << "Collision detected between entity " << entityIDA << " and entity " << entityIDB << std::endl;
    //             // Only entities with a script can handle collisions
    //             bool aHasScript = scriptPool.hasComponent(entityIDA);
    //             bool bHasScript = scriptPool.hasComponent(entityIDB);

    //             if ( aHasScript && bHasScript ) {
    //                 scriptPool.getComponent(entityIDA).Instance->OnCollisionFunction(entityIDB, collisionLayerB, overlap/2);
    //                 scriptPool.getComponent(entityIDB).Instance->OnCollisionFunction(entityIDA, collisionLayerA, overlap/2*-1);
    //             }
    //             else if ( scriptPool.hasComponent(entityIDA) ) {
    //                 scriptPool.getComponent(entityIDA).Instance->OnCollisionFunction(entityIDB, collisionLayerB, overlap);
    //             }
    //             else if ( scriptPool.hasComponent(entityIDB) ) {
    //                 scriptPool.getComponent(entityIDB).Instance->OnCollisionFunction(entityIDA, collisionLayerA, overlap*-1);
    //             }

    //         }
    //     }
    // }

    // playerCollisions();
    // enemyCollisions();
    // projectileCollisions();

    // auto viewCollision = m_ECS.signatureView<CCollisionBox, CTransform>();
    // // std::cout << "Number of entities in viewCollision: " << viewCollision.size() << std::endl;
    // // Implementation using collisionManager:
    // auto& collisionPool = m_ECS.getComponentPool<CCollisionBox>();
    // for ( auto entityA : m_ECS.signatureView<CCollisionBox, CTransform>() )
    // {
    //      for ( auto entityB : collisionPool )
    //      {
    //         if ( entityA == entityB ) { continue; } // Skip self-collision
    //         auto& collisionA = collisionPool.getComponent(entityA);
    //         auto& collisionB = collisionPool.getComponent(entityB);
    //         auto& collisionLayerA = collisionA.layer;
    //         auto& collisionMaskA = collisionA.mask;
    //         auto& collisionLayerB = collisionB.layer;
    //         auto& collisionMaskB = collisionB.mask;
    //         if ( (collisionLayerB & collisionMaskA) != collisionLayerB ) { continue; } // No collision mask match
    //         if ( (collisionLayerA & collisionMaskB) != collisionLayerA ) { continue; } // No collision layer match
    //          auto& transformPool = m_ECS.getComponentPool<CTransform>();
    //          auto& transformA = transformPool.getComponent(entityA);
    //          auto& transformB = transformPool.getComponent(entityB);
    //          if ( m_physics.isCollided(transformA, collisionA, transformB, collisionB) )
    //          {
    //              // Handle collision between entityA and entityB
    //              std::cout << "Collision detected between entity " << entityA << " and entity " << entityB << std::endl;
    //          }
    //      }
    // }
}

void Scene_Play::sStatus() {
    auto& transformPool = m_ECS.getComponentPool<CTransform>();
  
    auto& lifespanPool = m_ECS.getComponentPool<CLifespan>();
    auto viewLifespan = m_ECS.signatureView<CLifespan>();
    for ( auto entityID : viewLifespan)
    {   
        auto& lifespan = lifespanPool.getComponent(entityID).lifespan;
        lifespan--;
        m_ECS.queueRemoveEntity(entityID, lifespan <= 0);
    }

    auto viewHealth = m_ECS.signatureView<CHealth>();
    auto& healthPool = m_ECS.getComponentPool<CHealth>();
    for ( auto entityID : viewHealth)
    {
        auto& health = healthPool.getComponent(entityID);
        if (health.HP > 0)
        {
            continue;
        }
        auto& transform = transformPool.getComponent(entityID);
        if ( m_player == entityID ){
            std::cout << "Player has died!" << std::endl;
            m_restart = true;
        } else {
            spawnCoin(transform.pos, 6);
            m_ECS.queueRemoveEntity(entityID);
            Mix_PlayChannel(-1, m_game->assets().getAudio("enemy_death"), 0);
        }
    }
    // ------------------- SHLOULD BE IN sCollision() -------------------
    // auto viewDamage = m_ECS.signatureView<CDamage, CCollisionBox, CTransform>();
    // auto& collisionPool = m_ECS.getComponentPool<CCollisionBox>();
    // auto& damagePool = m_ECS.getComponentPool<CDamage>();
    // for ( auto entityDamage : viewDamage)
    // {
    //     auto& transformDamage = transformPool.getComponent(entityDamage);
    //     auto& bboxDamage = collisionPool.getComponent(entityDamage);
    //     auto& damage = damagePool.getComponent(entityDamage);
    //     Signature damageSignature = m_ECS.getSignature(entityDamage);
    //     for ( auto entityHealth : viewHealth )
    //     {
    //         if ( entityDamage == entityHealth ){continue;}
    //         if ( (entityHealth == m_player) && ((damageSignature & CProjectileStateMask) == CProjectileStateMask) ){continue;}
            
    //         auto& transforHealth = transformPool.getComponent(entityHealth);
    //         auto& bboxHealth = collisionPool.getComponent(entityHealth);
    //         auto& health = healthPool.getComponent(entityHealth);
    //         if ( m_physics.isCollided(transformDamage, bboxDamage, transforHealth, bboxHealth) )
    //         {
    //             if ( (int)(m_currentFrame-health.damage_frame) > health.i_frames ) {
    //                 int damageMultiplier = 1;
    //                 // m_ECS.addComponent<CKnockback>(entityHealth, 50/4, 64/4, transformDamage.pos-transforHealth.pos);
    //                 health.HP = health.HP-(int)(damage.damage*damageMultiplier);
    //                 health.damage_frame = m_currentFrame;
    //             }
    //         }
    //     }
    // }
}

void Scene_Play::sAnimation() {
    auto view = m_ECS.signatureView<CState, CAnimation, CTransform>();
    auto& transformPool = m_ECS.getComponentPool<CTransform>();
    auto& statePool = m_ECS.getComponentPool<CState>();
    auto& animationPool = m_ECS.getComponentPool<CAnimation>();
    for ( auto e : view ){
        auto& transform = transformPool.getComponent(e);
        auto& state = statePool.getComponent(e);
        auto& animation = animationPool.getComponent(e).animation;
        if( transform.vel.isNull() ) {
            changePlayerStateTo(e, PlayerState::STAND);
        } else if( transform.vel.mainDir().x > 0 ) {
            changePlayerStateTo(e, PlayerState::RUN_RIGHT);
        } else if(transform.vel.mainDir().x < 0) {
            changePlayerStateTo(e, PlayerState::RUN_LEFT);
        } else if(transform.vel.mainDir().y > 0) {
            changePlayerStateTo(e, PlayerState::RUN_DOWN);
        } else if(transform.vel.mainDir().y < 0) {
            changePlayerStateTo(e, PlayerState::RUN_UP);
        }
        // // change player animation
        if (state.changeAnimate) {
            animation.setRow((int)state.state);
        }
    }

    auto viewProjectileState = m_ECS.signatureView<CProjectileState, CAnimation>();
    auto& projectileStatePool = m_ECS.getComponentPool<CProjectileState>();
    for ( auto e : viewProjectileState ) {
        auto& animation = animationPool.getComponent(e);
        auto& projectileState = projectileStatePool.getComponent(e);
        if ( projectileState.state == "Create" ) {
            if ( animation.animation.hasEnded() ) {
                projectileState.state = "Ready";
                animation.animation = m_game->assets().getAnimation("fireball");
                animation.repeat = true;
                std::cout << "Projectile created!\n";
                m_camera.startShake(50, 100);
            }
        }
    }

    auto& view2 = m_ECS.view<CAnimation>();
    for ( auto e : view2 ){
        auto& animation = animationPool.getComponent(e);
        if (animation.animation.hasEnded() && !animation.repeat) {
            m_ECS.queueRemoveEntity(e);
        } else{
            animation.animation.update(m_currentFrame);
        }
    }

    auto view3 = m_ECS.signatureView<CSwimming, CAnimation>();
    for ( auto e : view3 ){
        auto& animation = animationPool.getComponent(e);
        SDL_Rect* rect = animation.animation.getSrcRect();
        animation.animation.setSrcRect(rect->x, rect->y-4, rect->w, rect->h);
    }
}

void Scene_Play::sRender() {
    // Clear the screen with black
    SDL_SetRenderDrawColor(m_game->renderer(), 0, 0, 0, 255);
    SDL_RenderClear(m_game->renderer());
    sRenderBasic();
    int windowScale = m_game->getScale();

    Animation animation;
    auto hearts = float(m_ECS.getComponent<CHealth>(m_player).HP) / 2;
    
    for (int i = 1; i <= m_ECS.getComponent<CHealth>(m_player).HP_max / 2; i++)
    {   
        if (hearts >= i)
        {
            animation = m_ECS.getComponent<CHealth>(m_player).animation_full;
        }
        else if (i - hearts == 0.5f)
        {
            animation = m_ECS.getComponent<CHealth>(m_player).animation_half;
        }
        else
        {
            animation = m_ECS.getComponent<CHealth>(m_player).animation_empty;
        }
        animation.setScale(Vec2{1, 1}*windowScale);
        animation.setDestRect(Vec2{(float)(i - 1) * animation.getSize().x * animation.getScale().x, 0}*windowScale);
        spriteRender(animation);
    }

    Vec2 screenCenter = Vec2{(float)width(), (float)height()}/2;
    auto& transformPool = m_ECS.getComponentPool<CTransform>();
    auto& viewHealth = m_ECS.getComponentPool<CHealth>();
    for (auto entityID : viewHealth)
    {   
        if (entityID == m_player) { continue; }
        auto& health = viewHealth.getComponent(entityID);
        if ((int)(m_currentFrame - health.damage_frame) >= health.i_frames)
        {
            continue;
        }
        auto& transform = transformPool.getComponent(entityID);

        Vec2 adjustedPosition = (transform.pos - m_camera.position) * (windowScale - m_camera.getCameraZoom());
        adjustedPosition += screenCenter*m_camera.getCameraZoom();

        Animation animation;
        auto hearts = float(health.HP) / 2;

        for (int i = 1; i <= health.HP_max / 2; i++)
        {   
            if (hearts >= i)
            {
                animation = health.animation_full;
            }
            else if (i - hearts == 0.5f)
            {
                animation = health.animation_half;
            }
            else
            {
                animation = health.animation_empty;
            }

            animation.setScale(transform.scale * windowScale);
            animation.setDestRect(Vec2{
                adjustedPosition.x + (float)(i - 1 - (float)health.HP_max / 4) * animation.getSize().x * animation.getScale().x, 
                adjustedPosition.y - m_ECS.getComponent<CAnimation>(entityID).animation.getSize().y * m_ECS.getComponent<CAnimation>(entityID).animation.getScale().y / 2
            });
            spriteRender(animation);
        }
    }
    auto totalZoom = windowScale - m_camera.getCameraZoom();
    auto screenCenterZoomed = screenCenter * m_camera.getCameraZoom();
    auto camPos = m_camera.position;
    if (m_drawCollision)
    {
        // m_physics.renderQuadtree(m_game->renderer(), totalZoom, screenCenterZoomed, camPos);
        m_collisionManager.renderQuadtree(m_game->renderer(), totalZoom, screenCenterZoomed, camPos);
    }
    if (m_drawInteraction)
    {
        // m_physics.renderInteractionQuadtree(m_game->renderer(), totalZoom, screenCenterZoomed, camPos);
        m_collisionManager.renderInteractionQuadtree(m_game->renderer(), totalZoom, screenCenterZoomed, camPos);
    }
}

void Scene_Play::sAudio()
{
    if( Mix_PlayingMusic() == 0 )
    {
        Mix_PlayMusic(m_game->assets().getMusic("AbbeGameTrack1ogg"), -1);
    }
}

EntityID Scene_Play::spawnPlayer()
{
    uint8_t layer = 10;
    auto entityID = m_ECS.addEntity();
    m_player = entityID;

    int pos_x = m_playerConfig.x;
    int pos_y = m_playerConfig.y;
    int hp = m_playerConfig.HP;
    
    if (!m_newGame){
        std::ifstream file("config_files/game_save.txt");
        if (!file) {
            std::cerr << "Could not load game_save.txt file!\n";
            exit(-1);
        }
        std::string head;
        
        while (file >> head) {
            if (head == "Player_pos") {
                file >> pos_x >> pos_y;
            } else if (head == "Player_hp") {
                file >> hp;
            } else {
                std::cerr << "The game save file format is incorrect!\n";
                exit(-1);
            }
        }
    }
    Vec2 pos = Vec2{16*(float)pos_x, 16*(float)pos_y};
    Vec2 midGrid = gridToMidPixel(pos, entityID);

    m_ECS.addComponent<CTransform>(entityID, midGrid, Vec2{0,0}, Vec2{1, 1}, 0.0f, m_playerConfig.SPEED, true);
    CollisionMask collisionMask = ENEMY_LAYER | OBSTACLE_LAYER | FRIENDLY_LAYER;
    m_ECS.addComponent<CCollisionBox>(entityID, Vec2 {8, 8}, PLAYER_LAYER, collisionMask);
    InterationMask interactionMask = ENEMY_LAYER | FRIENDLY_LAYER | LOOT_LAYER;
    m_ECS.addComponent<CInteractionBox>(entityID, Vec2 {16, 16}, PLAYER_LAYER1, interactionMask);
    m_ECS.addComponent<CName>(entityID, "demon");
    m_ECS.addComponent<CAnimation>(entityID, m_game->assets().getAnimation("demon-sheet"), true, layer);
    m_rendererManager.addEntityToLayer(entityID, layer);
    spawnShadow(entityID, Vec2{0,0}, 1, layer-1);
    m_ECS.addComponent<CInputs>(entityID);
    m_ECS.addComponent<CState>(entityID, PlayerState::STAND);
    m_ECS.addComponent<CHealth>(entityID, hp, m_playerConfig.HP, 60, m_game->assets().getAnimation("heart_full"), m_game->assets().getAnimation("heart_half"), m_game->assets().getAnimation("heart_empty"));
    
    auto& sc= m_ECS.addComponent<CScript>(entityID);
    InitiateScript<PlayerController>(sc, entityID);
    return entityID;
}

EntityID Scene_Play::spawnNPC(Vec2 pos)
{
    uint8_t layer = 10;
    auto entityID = m_ECS.addEntity();

    Vec2 midGrid = gridToMidPixel(pos*16, entityID);

    m_ECS.addComponent<CTransform>(entityID, midGrid, Vec2{0,0}, Vec2{1, 1}, 0.0f, m_playerConfig.SPEED, true);
    
    CollisionMask collisionMask = ENEMY_LAYER | OBSTACLE_LAYER | FRIENDLY_LAYER | PLAYER_LAYER;
    m_ECS.addComponent<CCollisionBox>(entityID, Vec2 {8, 8}, FRIENDLY_LAYER, collisionMask);

    InterationMask interactionMask = PLAYER_LAYER1;
    m_ECS.addComponent<CInteractionBox>(entityID, Vec2 {48, 32}, FRIENDLY_LAYER, interactionMask);

    m_ECS.addComponent<CName>(entityID, "NPC1");
    m_ECS.addComponent<CAnimation>(entityID, m_game->assets().getAnimation("wiz-sheet"), true, layer);
    m_rendererManager.addEntityToLayer(entityID, layer);
    spawnShadow(entityID, Vec2{0,0}, 1, layer-1);
    m_ECS.addComponent<CState>(entityID, PlayerState::STAND);
    
    auto& sc= m_ECS.addComponent<CScript>(entityID);
    InitiateScript<NPCController>(sc, entityID);
    return entityID;
}

EntityID Scene_Play::spawnShadow(EntityID parentID, Vec2 relPos, int size, int layer){
    auto shadowID = m_ECS.addEntity();
    // m_ECS.addComponent<CTransform>(shadowID);
    // m_ECS.getComponent<CTransform>(shadowID).scale *= size;
    // m_ECS.addComponent<CParent>(shadowID, parentID, relPos);
    // m_ECS.addComponent<CAnimation>(shadowID, m_game->assets().getAnimation("shadow"), true, layer);
    // m_rendererManager.addEntityToLayer(shadowID, layer);
    // m_ECS.addComponent<CChild>(parentID, shadowID);
    return shadowID;
}

EntityID Scene_Play::spawnWeapon(Vec2 pos, int layer){
    auto entity = m_ECS.addEntity();

    Vec2 midGrid = gridToMidPixel(pos, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid, Vec2{0,0}, Vec2{1, 1}, 0.0f, 0.0f, true);
    
    InterationMask interactionMask = PLAYER_LAYER1;
    m_ECS.addComponent<CInteractionBox>(entity, Vec2 {6, 6}, LOOT_LAYER, interactionMask);

    m_ECS.addComponent<CName>(entity, "staff");
    m_ECS.addComponent<CAnimation>(entity, m_game->assets().getAnimation("staff"), true, 2);
    m_rendererManager.addEntityToLayer(entity, 5);
    // m_ECS.addComponent<CDamage>(entity, 1, 180, std::unordered_set<std::string> {"Fire", "Explosive"});
    m_ECS.addComponent<CWeapon>(entity);
    spawnShadow(entity, Vec2{0,0}, 1, layer-1);
    auto& sc = m_ECS.addComponent<CScript>(entity);
    InitiateScript<WeaponController>(sc, entity);
    return entity;
}

EntityID Scene_Play::spawnSword(Vec2 pos, int layer){
    auto entity = m_ECS.addEntity();

    Vec2 midGrid = gridToMidPixel(pos, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid, Vec2{0,0}, Vec2{1, 1}, 0.0f, 0.0f, true);
    
    InterationMask interactionMask = PLAYER_LAYER1;
    m_ECS.addComponent<CInteractionBox>(entity, Vec2 {6, 6}, LOOT_LAYER, interactionMask);
    
    m_ECS.addComponent<CName>(entity, "sword");
    m_ECS.addComponent<CAnimation>(entity, m_game->assets().getAnimation("sword"), true, 2);
    m_rendererManager.addEntityToLayer(entity, 5);
    // m_ECS.addComponent<CDamage>(entity, 1, 180, std::unordered_set<std::string> {"Fire", "Explosive"});
    m_ECS.addComponent<CWeapon>(entity);
    spawnShadow(entity, Vec2{0,0}, 1, layer-1);
    auto& sc = m_ECS.addComponent<CScript>(entity);
    InitiateScript<WeaponController>(sc, entity);
    return entity;
}

EntityID Scene_Play::spawnDecoration(Vec2 pos, Vec2 collisionBox, const size_t layer, std::string animation){
    auto entity = m_ECS.addEntity();

    Vec2 midGrid = gridToMidPixel(pos, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid, Vec2{0,0}, Vec2{1, 1}, 0.0f, 0.0f, true);
    CollisionMask collisionMask = ENEMY_LAYER | FRIENDLY_LAYER | PLAYER_LAYER;
    m_ECS.addComponent<CCollisionBox>(entity, collisionBox, OBSTACLE_LAYER, collisionMask);
    m_ECS.addComponent<CAnimation>(entity, m_game->assets().getAnimation(animation), true, layer);
    m_rendererManager.addEntityToLayer(entity, layer);
    m_ECS.addComponent<CImmovable>(entity);
    spawnShadow(entity, Vec2{0,-16/4}, 3, layer-1);
    return entity;
}

EntityID Scene_Play::spawnObstacle(const Vec2 pos, bool movable, const int frame){
    auto entity = m_ECS.addEntity();
    Vec2 midGrid = gridToMidPixel(pos, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid, Vec2 {0, 0}, Vec2 {0.5,0.5}, 0.0f, movable);
    CollisionMask collisionMask = ENEMY_LAYER | FRIENDLY_LAYER | PLAYER_LAYER | PROJECTILE_LAYER;
    m_ECS.addComponent<CCollisionBox>(entity, Vec2 {16, 16}, OBSTACLE_LAYER, collisionMask);

    m_ECS.addComponent<CImmovable>(entity); // remove when new collision system is implemented
    return entity;
}

EntityID Scene_Play::spawnGrass(const Vec2 pos, const int frame)
{
    auto entity = m_ECS.addEntity();
    // std::vector<int> ranArray = generateRandomArray(1, m_ECS.getNumEntities(), 0, 15);
    Vec2 midGrid = gridToMidPixel(pos, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid, Vec2 {0, 0}, Vec2{1, 1}, 0.0f, false);
    return entity;
}

EntityID Scene_Play::spawnDirt(const Vec2 pos, const int frame)
{
    auto entity = m_ECS.addEntity();
    Vec2 midGrid = gridToMidPixel(pos, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid, Vec2 {0, 0}, Vec2{1, 1}, 0.0f, false);
    return entity;
}

EntityID Scene_Play::spawnCampfire(const Vec2 pos, int layer)
{
    auto entity = m_ECS.addEntity();
    m_ECS.addComponent<CAnimation>(entity,m_game->assets().getAnimation("campfire"), true, layer);
    m_rendererManager.addEntityToLayer(entity, layer);
    Vec2 midGrid = gridToMidPixel(pos, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid, Vec2 {0, 0}, Vec2{1, 1}, 0.0f, false);
    // m_ECS.addComponent<CCollisionBox>(entity, Vec2{8, 8});
    return entity;
}

EntityID Scene_Play::spawnLava(const Vec2 pos, const std::string tag, const int frame)
{
    auto entity = m_ECS.addEntity();
    Vec2 midGrid = gridToMidPixel(pos, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid, Vec2 {0, 0}, Vec2{1, 1}, 0.0f, false);
    m_ECS.addComponent<CCollisionBox>(entity, Vec2{64/4, 64/4});
    return entity;
}

EntityID Scene_Play::spawnWater(const Vec2 pos, const std::string tag, const int frame)
{
    auto entity = m_ECS.addEntity();
    Vec2 midGrid = gridToMidPixel(pos, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid, Vec2 {0, 0}, Vec2{1, 1}, 0.0f, false);
    m_ECS.addComponent<CWater>(entity, CWater{false});
    m_ECS.addComponent<CCollisionBox>(entity, Vec2{64/4, 64/4});

    return entity;
}

EntityID Scene_Play::spawnProjectile(EntityID creator, Vec2 vel, int layer)
{
    auto entity = m_ECS.addEntity();
    m_ECS.addComponent<CAnimation>(entity, m_game->assets().getAnimation("fireball"), true, layer);
    m_rendererManager.addEntityToLayer(entity, layer);
    m_ECS.addComponent<CTransform>(entity, m_ECS.getComponent<CTransform>(creator).pos, vel, Vec2{1, 1}, vel.angle(), 200.0f, true);
    m_ECS.addComponent<CDamage>(entity, 1);
    m_ECS.getComponent<CDamage>(entity).damageType = {"Fire", "Explosive"};
    // m_ECS.addComponent<CProjectileState>(entity, "Create");
    CollisionMask collisionMask = ENEMY_LAYER | OBSTACLE_LAYER;
    m_ECS.addComponent<CCollisionBox>(entity, Vec2{12, 12}, PROJECTILE_LAYER, collisionMask);
    // m_ECS.getComponent<CTransform>(entity).isMovable = true;
    m_ECS.addComponent<CProjectileState>(entity, "Free");
    // m_ECS.getComponent<CTransform>(entity).vel = (m_game->currentScene()->getMousePosition()-m_ECS.getComponent<CTransform>(entity).pos+m_game->currentScene()->getCameraPosition());
    // m_ECS.getComponent<CTransform>(entity).angle = m_ECS.getComponent<CTransform>(entity).vel.angle();

    spawnShadow(entity, Vec2{0,0}, 1, layer-1);
    // auto& script = m_ECS.addComponent<CScript>(entity);
    // InitiateScript<ProjectileController>(script, entity);
    return entity;
}

EntityID Scene_Play::spawnCoin(Vec2 pos, const size_t layer)
{
    auto entity = m_ECS.addEntity();
    m_ECS.addComponent<CAnimation>(entity, m_game->assets().getAnimation("coin"), true, layer);
    m_rendererManager.addEntityToLayer(entity, layer);
    Vec2 midGrid = gridToMidPixel(pos, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid, Vec2 {0, 0}, Vec2{1, 1}, 0.0f, false);

    InterationMask interactionMask = PLAYER_LAYER1;
    m_ECS.addComponent<CInteractionBox>(entity, Vec2 {8 ,8}, LOOT_LAYER, interactionMask);

    m_ECS.addComponent<CLoot>(entity);
    spawnShadow(entity, Vec2{0,0}, 1, layer-1);

    auto& sc= m_ECS.addComponent<CScript>(entity);
    InitiateScript<CoinController>(sc, entity);
    
    return entity;
}

EntityID Scene_Play::spawnSmallEnemy(Vec2 pos, const size_t layer, std::string type)
{
    auto entity = m_ECS.addEntity();
    m_ECS.addComponent<CName>(entity, type);
    m_ECS.addComponent<CAnimation>(entity, m_game->assets().getAnimation(type), true, 3);
    m_rendererManager.addEntityToLayer(entity, layer);
    m_ECS.addComponent<CState>(entity, PlayerState::STAND);
    Vec2 midGrid = gridToMidPixel(pos, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid, Vec2{0,0}, Vec2{1,1}, 0.0f, 50.0f, true);
    m_ECS.addComponent<CTransform>(entity, midGrid, Vec2{0, 0}, Vec2{1, 1}, 0.0f, m_goblinConfig.SPEED, false);
    CollisionMask collisionMask = ENEMY_LAYER | OBSTACLE_LAYER | FRIENDLY_LAYER | PLAYER_LAYER | PROJECTILE_LAYER;
    m_ECS.addComponent<CCollisionBox>(entity, Vec2{8, 12}, ENEMY_LAYER, collisionMask);
    m_ECS.addComponent<CPathfind>(entity, m_ECS.getComponent<CTransform>(m_player).pos);

    m_ECS.addComponent<CHealth>(entity, 4, 4, 30, 
        m_game->assets().getAnimation("heart_full"), 
        m_game->assets().getAnimation("heart_half"), 
        m_game->assets().getAnimation("heart_empty"));
    m_ECS.getComponent<CHealth>(entity).HPType = {"Grass", "Organic"};
    m_ECS.addComponent<CAttack>(entity, 1, 120, 30, 3*64/4, Vec2{64/4,64/4});

    spawnShadow(entity, Vec2{0, 16/4}, 1, layer-1);

    auto& sc= m_ECS.addComponent<CScript>(entity);
    InitiateScript<RooterController>(sc, entity);
    return entity;
}

std::vector<EntityID> Scene_Play::spawnDualTiles(const Vec2 pos, std::unordered_map<std::string, int> tileTextureMap)
{   
    std::vector<EntityID> entityIDs;
    for (const auto& [tileKey, textureIndex] : tileTextureMap) {
        std::string tile = tileKey;
        uint8_t layer = 3;
        
        if (tile == "water") {
            layer = layer + 1;
        } else if (tile == "dirt") {
            layer = layer + 1;
        } else if (tile == "cloud") {
            layer = layer + 1;
        } else if (tile == "obstacle") {
            layer = layer + 1;
            tile = "mountain";  // Change the tile name for "obstacle"
        }
        EntityID entity = m_ECS.addEntity();
        entityIDs.push_back(entity);
        m_ECS.addComponent<CAnimation>(entity, m_game->assets().getAnimation(tile + "_dual_sheet"), true, 10);
        Vec2 tilePosition = Vec2{   (float)(textureIndex % 4), 
                                    (float)(int)(textureIndex / 4)};
        m_ECS.getComponent<CAnimation>(entity).animation.setTile(tilePosition);   
        m_rendererManager.addEntityToLayer(entity, layer);
        Vec2 midGrid = gridToMidPixel(pos, entity);
        m_ECS.addComponent<CTransform>(entity, midGrid, Vec2{0, 0}, Vec2{1, 1}, 0.0f, false);
    }
    return entityIDs;
}

void Scene_Play::changePlayerStateTo(EntityID entity, PlayerState s) {
    auto& prev = m_ECS.getComponent<CState>(entity).preState; 
    if (prev != s) {
        prev = m_ECS.getComponent<CState>(entity).state;
        m_ECS.getComponent<CState>(entity).state = s; 
        m_ECS.getComponent<CState>(entity).changeAnimate = true;
    }
    else { 
        m_ECS.getComponent<CState>(entity).changeAnimate = false;
    }
}

template<typename T>
void Scene_Play::InitiateScript(CScript& sc, EntityID entityID){
    sc.Bind<T>();
    sc.Instance = sc.InstantiateScript();
    sc.Instance->m_entity = {entityID, &m_ECS};
    sc.Instance->m_ECS = &m_ECS;
    sc.Instance->m_physics = &m_physics;
    sc.Instance->m_game = m_game;
    sc.Instance->m_scene = this;
    sc.Instance->OnCreateFunction();
}

void Scene_Play::InitiateProjectileScript(CScript& sc, EntityID entityID){
    sc.Bind<ProjectileController>();
    sc.Instance = sc.InstantiateScript();
    sc.Instance->m_entity = {entityID, &m_ECS};
    sc.Instance->m_ECS = &m_ECS;
    sc.Instance->m_physics = &m_physics;
    sc.Instance->m_game = m_game;
    sc.Instance->m_scene = this;
    sc.Instance->OnCreateFunction();
}


void Scene_Play::onEnd() {
    m_game->changeScene("MAIN_MENU", std::make_shared<Scene_Menu>(m_game));
}

void Scene_Play::setPaused(bool pause) {
    m_pause = pause;
}

void Scene_Play::togglePause() {
    m_pause = !m_pause;
}

Vec2 Scene_Play::gridSize(){
    return m_gridSize;
}

Vec2 Scene_Play::levelSize(){
    return m_levelSize;
}

Vec2 Scene_Play::getCameraPosition() {
    return m_camera.position;
}
