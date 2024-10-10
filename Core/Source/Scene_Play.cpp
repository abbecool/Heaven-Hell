#include "Scene_Menu.h"
#include "Scene_Play.h"
#include "Sprite.h"
#include "Assets.h"
#include "Game.h"
#include "Components.h"
#include "Action.h"
#include "Level_Loader.h"
#include "Camera.h"
#include "ScriptableEntity.h"
#include "scripts/player.cpp"
#include "scripts/rooter.cpp"

#include "RandomArray.h"

#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>

#include <iostream>
#include <string>
#include <fstream>
#include <unordered_map>
#include <unordered_set>

// """
// std::map<std::string, void (Scene_Play::*)()> funcMap;
// 
// for loop dropings a map from string to spawnXXX function will be needed
// 
// """

Scene_Play::Scene_Play(Game* game, std::string levelPath, bool newGame)
    : Scene(game), m_levelPath(levelPath), m_newGame(newGame)
{
    init(m_levelPath);
    m_camera.calibrate(Vec2 {(float)width(), (float)height()}, m_levelSize, m_gridSize);
}

void Scene_Play::init(const std::string& levelPath) {
    registerAction(SDLK_w, "UP");
    registerAction(SDLK_s, "DOWN");
    registerAction(SDLK_a, "LEFT");
    registerAction(SDLK_d, "RIGHT");
    registerAction(SDL_BUTTON_LEFT , "ATTACK");
    registerAction(SDLK_SPACE , "ATTACK");
    registerAction(SDLK_LSHIFT, "SHIFT");
    registerAction(SDLK_LCTRL, "CTRL");
    registerAction(SDLK_ESCAPE, "QUIT");
    registerAction(SDLK_f, "CAMERA FOLLOW");
    registerAction(SDLK_z, "CAMERA PAN");
    registerAction(SDLK_PLUS, "ZOOM IN");
    registerAction(SDLK_MINUS, "ZOOM OUT");
    registerAction(SDLK_r, "RESET");
    registerAction(SDLK_p, "PAUSE");
    registerAction(SDLK_t, "TOGGLE_TEXTURE");
    registerAction(SDLK_c, "TOGGLE_COLLISION");
    registerAction(SDLK_0, "LEVEL0");
    registerAction(SDLK_1, "LEVEL1");
    registerAction(SDLK_2, "LEVEL2");
    registerAction(SDLK_u, "SAVE");

    loadConfig("config.txt");
    loadLevel(levelPath);
    
    spawnPlayer();
    spawnCoin(Vec2{64*46,64*10}, 4);
    // spawnDragon(Vec2{64*52 , 64*44}, false, "snoring_dragon");
    spawnWeapon(Vec2{64*50 , 64*13});
    spawnSmallEnemy(Vec2{64*15 , 64*9}, 3, "rooter");
    spawnSmallEnemy(Vec2{64*10 , 64*10}, 3, "rooter");
    spawnSmallEnemy(Vec2{64*20 , 64*12}, 3, "goblin");
    //spawnGoal(Vec2{64*23, 64*8}, false);
    //spawnGoal(Vec2{64*37, 64*47}, false);
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
    if (loadedSurface == nullptr) {
        std::cerr << "Unable to load image " << path << "! SDL_image Error: " << IMG_GetError() << std::endl;
    }

    // Create a texture from the surface
    SDL_Texture* texture = SDL_CreateTextureFromSurface(m_game->renderer() , loadedSurface);
    if (texture == nullptr) {
        std::cerr << "Unable to create texture from " << path << "! SDL Error: " << SDL_GetError() << std::endl;
        SDL_FreeSurface(loadedSurface);
    }

    // Lock the surface to access the pixels
    SDL_LockSurface(loadedSurface);
    Uint32* pixels = (Uint32*)loadedSurface->pixels;
    const int HEIGHT_PIX = loadedSurface->h;
    const int WIDTH_PIX = loadedSurface->w;
    m_levelSize = Vec2{ (float)WIDTH_PIX, (float)HEIGHT_PIX };
    auto format = loadedSurface->format;
    std::vector<std::vector<std::string>> pixelMatrix = m_levelLoader.createPixelMatrix(pixels, format, WIDTH_PIX, HEIGHT_PIX);

    // Unlock and free the surface
    SDL_UnlockSurface(loadedSurface);
    SDL_FreeSurface(loadedSurface);
    SDL_DestroyTexture(texture);


    // Process the pixels
    for (int y = 0; y < HEIGHT_PIX; ++y) {
        for (int x = 0; x < WIDTH_PIX; ++x) {
            const std::string& pixel = pixelMatrix[y][x];
            std::vector<bool> neighbors = m_levelLoader.neighborCheck(pixelMatrix, pixel, x, y, WIDTH_PIX, HEIGHT_PIX);
            std::vector<std::string> neighborsTags = m_levelLoader.neighborTag(pixelMatrix, pixel, x, y, WIDTH_PIX, HEIGHT_PIX);
            int textureIndex = m_levelLoader.getObstacleTextureIndex(neighbors);
            std::unordered_map<std::string, int> tileIndex = m_levelLoader.createDualGrid(pixelMatrix, x, y, HEIGHT_PIX, WIDTH_PIX);
            spawnDualTiles(Vec2 {64*(float)x - 32, 64*(float)y - 32},  tileIndex);

            if (pixel == "obstacle") {
                spawnObstacle(Vec2 {64*(float)x, 64*(float)y}, false, textureIndex);
            }
            else{
                if (pixel == "lava") {
                    spawnLava(Vec2 {64*(float)x,64*(float)y}, "Lava", textureIndex);
                } else if (pixel == "water") {
                    spawnWater(Vec2 {64*(float)x,64*(float)y}, "Water", textureIndex);
                } else if (pixel == "bridge") {
                    if ( std::find(neighborsTags.begin(), neighborsTags.end(), "water") != neighborsTags.end() ){
                        spawnWater(Vec2 {64*(float)x,64*(float)y}, "Background", m_levelLoader.getObstacleTextureIndex(m_levelLoader.neighborCheck(pixelMatrix, "water", x, y, WIDTH_PIX, HEIGHT_PIX)));
                    } else if ( std::find(neighborsTags.begin(), neighborsTags.end(), "lava") != neighborsTags.end() ){
                        spawnLava(Vec2 {64*(float)x,64*(float)y}, "Background", m_levelLoader.getObstacleTextureIndex(m_levelLoader.neighborCheck(pixelMatrix, "lava", x, y, WIDTH_PIX, HEIGHT_PIX)));
                    }
                    spawnBridge(Vec2 {64*(float)x,64*(float)y}, textureIndex);
                }
            }
        }
    }
}

void Scene_Play::sDoAction(const Action& action) {
    if (action.type() == "START") {
        if (action.name() == "TOGGLE_TEXTURE") {
            m_drawTextures = !m_drawTextures; 
        } else if (action.name() == "TOGGLE_COLLISION") { 
            m_drawCollision = !m_drawCollision; 
        } else if (action.name() == "TOGGLE_GRID") { 
            m_drawDrawGrid = !m_drawDrawGrid; 
        } else if (action.name() == "PAUSE") { 
            setPaused(!m_pause);
        } else if (action.name() == "QUIT") { 
            onEnd();
        } else if (action.name() == "ZOOM IN"){
            m_camera.setCameraZoom(Vec2 {2, 2});
        } else if (action.name() == "ZOOM OUT"){
            m_camera.setCameraZoom(Vec2{0.5, 0.5});
        } else if (action.name() == "CAMERA FOLLOW"){
            m_camera.toggleCameraFollow();
        } else if (action.name() == "CAMERA PAN"){
            m_pause = m_camera.startPan(2048, 1000, Vec2 {(float)(64*52+32 - width()/2), (float)(64*44+32 - height()/2)}, m_pause);
        } else if (action.name() == "SAVE"){
            saveGame("game_save.txt");
        } else if (action.name() == "LEVEL0") { 
            m_game->changeScene("PLAY", std::make_shared<Scene_Play>(m_game, "assets/images/levels/levelStartingArea.png", true));
        } else if (action.name() == "LEVEL1") { 
            m_game->changeScene("PLAY", std::make_shared<Scene_Play>(m_game, "assets/images/levels/level1.png", true));
        }else if (action.name() == "LEVEL2") {
            m_game->changeScene("PLAY", std::make_shared<Scene_Play>(m_game, "assets/images/levels/level2.png", true));
        } else if (action.name() == "RESET") { 
            m_game->changeScene("PLAY", std::make_shared<Scene_Play>(m_game, "assets/images/levels/levelStartingArea.png", true));
        }

        if (action.name() == "UP") {
            m_ECS.getComponent<CInputs>(m_player).up = true;
        }
        if (action.name() == "DOWN") {
            m_ECS.getComponent<CInputs>(m_player).down = true; 
        }
        if (action.name() == "LEFT") {
            m_ECS.getComponent<CInputs>(m_player).left = true;
        }
        if (action.name() == "RIGHT") {
            m_ECS.getComponent<CInputs>(m_player).right = true;
        }
        if (action.name() == "SHIFT") {
            m_ECS.getComponent<CInputs>(m_player).shift = true;
        }
        if (action.name() == "CTRL") {
            m_ECS.getComponent<CInputs>(m_player).ctrl = true;
        }
        if (action.name() == "ATTACK" && m_ECS.hasComponent<CWeaponChild>(m_player)){
            EntityID weaponID = m_ECS.getComponent<CWeaponChild>(m_player).weaponID;
            spawnProjectile(weaponID, getMousePosition()-m_ECS.getComponent<CTransform>(weaponID).pos+m_camera.position + Vec2{16,-16});
        }
    }
    else if (action.type() == "END") {
        if (action.name() == "DOWN") {
            m_ECS.getComponent<CInputs>(m_player).down = false;
        } if (action.name() == "UP") {
            m_ECS.getComponent<CInputs>(m_player).up = false;
        } if (action.name() == "LEFT") {
            m_ECS.getComponent<CInputs>(m_player).left = false;
        } if (action.name() == "RIGHT") {
            m_ECS.getComponent<CInputs>(m_player).right = false;
        } if (action.name() == "SHIFT") {
            m_ECS.getComponent<CInputs>(m_player).shift = false;
        } if (action.name() == "CTRL") {
            m_ECS.getComponent<CInputs>(m_player).ctrl = false;
        } if (action.name() == "ATTACK" && m_ECS.hasComponent<CWeaponChild>(m_player)) {
            EntityID weaponID = m_ECS.getComponent<CWeaponChild>(m_player).weaponID;
            if ( m_ECS.hasComponent<CProjectile>(weaponID) )
            {
                EntityID projectileID = m_ECS.getComponent<CProjectile>(weaponID).projectileID;
                m_ECS.queueRemoveComponent<CProjectile>(weaponID);
                m_ECS.queueRemoveComponent<CParent>(projectileID);
                if ( m_ECS.getComponent<CProjectileState>(projectileID).state == "Ready" )
                {
                    m_ECS.addComponent<CBoundingBox>(projectileID, Vec2{12, 12});
                    m_ECS.getComponent<CTransform>(projectileID).isMovable = true;
                    m_ECS.getComponent<CProjectileState>(projectileID).state = "Free";
                    m_ECS.getComponent<CTransform>(projectileID).vel = getMousePosition()-m_ECS.getComponent<CTransform>(weaponID).pos+m_camera.position;
                    m_ECS.getComponent<CTransform>(projectileID).angle = m_ECS.getComponent<CTransform>(projectileID).vel.angle();
                } else {
                    m_ECS.queueRemoveEntity(projectileID);
                }
            }
        }
    }
}

void Scene_Play::update() {
    m_ECS.update();
    m_pause = m_camera.update(m_ECS.getComponent<CTransform>(m_player).pos, m_pause);
    if (!m_pause) {
        sMovement();
        m_currentFrame++;
        auto& view = m_ECS.view<CScript>();
        auto& scriptPool = m_ECS.getComponentPool<CScript>();
        for ( auto e : view ) {
            auto& sc = scriptPool.getComponent(e);
            if ( !sc.Instance )
            {
                sc.Instance = sc.InstantiateScript();
                sc.Instance->m_entity = {e, &m_ECS};
                sc.Instance->OnCreateFunction();
            }
            sc.Instance->OnUpdateFunction();
        //     // memory leak, destroy 
        }
        sCollision();
        sStatus();
        sAnimation();
        sAudio();
    }
    sRender();
}

void Scene_Play::sMovement() {
    auto& transformPool = m_ECS.getComponentPool<CTransform>();

    auto& pathfindPool = m_ECS.getComponentPool<CPathfind>();
    auto& view1 = m_ECS.view<CPathfind>();
    for (auto e : view1)
    {
        auto& transform = transformPool.getComponent(e);
        auto& pathfind = pathfindPool.getComponent(e);
        Vec2& target = pathfind.target;
        if ((target - transform.pos).length() < 64*2) {
            transform.vel = target - transform.pos;
        } else {
            transform.vel = Vec2 {0,0};
        }
        target = transformPool.getComponent(m_player).pos;
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
                transform.tempo = 3.0f;
            } else{
                transform.tempo = 1.0f;
            }
        }
    }
    auto& viewTransform = m_ECS.view<CTransform>();
    for (auto e : viewTransform){    
        auto &transform = transformPool.getComponent(e);
        
        // Update position
        transform.prevPos = transform.pos;
        if (!(transform.vel.isnull()) && transform.isMovable ){
            transform.pos += transform.vel.norm(transform.tempo*transform.speed/m_game->framerate());
        }
    }
    
    // if ( m_ECS.hasComponent<CProjectile>(m_player) )
    // {
    //     EntityID projectileID = m_ECS.getComponent<CProjectile>(m_player).projectileID;
    //     m_ECS.getComponent<CTransform>(projectileID).vel = getMousePosition()-m_ECS.getComponent<CTransform>(m_player).pos+m_camera.position;
    //     m_ECS.getComponent<CTransform>(projectileID).angle = m_ECS.getComponent<CTransform>(projectileID).vel.angle();
    // }

    auto& viewParent = m_ECS.view<CParent>();
    auto& parentPool = m_ECS.getOrCreateComponentPool<CParent>();
    for (auto e : viewParent)
    {
        auto& transform = transformPool.getComponent(e);
        auto& parent = parentPool.getComponent(e);
        transform.pos = transformPool.getComponent(parent.parent).pos + parent.relativePos;
    }
}

void Scene_Play::sCollision() {
// ------------------------------- Player collisions -------------------------------------------------------------------------
    auto& transformPool = m_ECS.getComponentPool<CTransform>();
    auto& BboxPool = m_ECS.getComponentPool<CBoundingBox>();
    auto& transformPlayer = transformPool.getComponent(m_player);
    auto& BboxPlayer = BboxPool.getComponent(m_player);

    auto& viewLoot = m_ECS.getComponentPool<CLoot>();
    for ( auto e : viewLoot ){
        auto& transform = transformPool.getComponent(e);
        auto& Bbox = BboxPool.getComponent(e);
        if ( m_physics.isCollided(transformPlayer, BboxPlayer, transform, Bbox) )
        {
            m_ECS.queueRemoveEntity(e);
            Mix_PlayChannel(-1, m_game->assets().getAudio("test"), 0);
        }
    }

    auto& viewWeapon = m_ECS.getComponentPool<CWeapon>();
    for ( auto e : viewWeapon ){
        auto& transform = transformPool.getComponent(e);
        auto& Bbox = BboxPool.getComponent(e);
        if ( m_physics.isCollided(transformPlayer, BboxPlayer, transform, Bbox) )
        {
            m_ECS.queueRemoveComponent<CBoundingBox>(e);
            m_ECS.queueRemoveComponent<CWeapon>(e);
            m_ECS.addComponent<CWeaponChild>(m_player, e);
            m_ECS.addComponent<CParent>(e, m_player, Vec2{32, -16});
            Mix_PlayChannel(-1, m_game->assets().getAudio("test"), 0);

        }
    }

    auto &viewImmovable = m_ECS.view<CImmovable>();
    for ( auto e : viewImmovable ){
        auto& transform = transformPool.getComponent(e);
        auto& Bbox = BboxPool.getComponent(e);
        if (m_physics.isCollided(transformPlayer, BboxPlayer, transform, Bbox))
        {
            m_ECS.getComponent<CTransform>(m_player).pos += m_physics.overlap(transformPlayer, BboxPlayer, transform, Bbox);
        }
    }

// ------------------------------- Enemy collisions -------------------------------------------------------------------------

    auto& viewP = m_ECS.view<CPathfind>();
    for ( auto e1 : viewP )
    {
        auto& transform1 = transformPool.getComponent(e1);
        auto& Bbox1 = BboxPool.getComponent(e1);
        for (auto e2 : viewP)
        {
            auto& transform2 = transformPool.getComponent(e2);
            auto& Bbox2 = BboxPool.getComponent(e2);
            if (m_physics.isCollided(transform1, Bbox1, transform2, Bbox2))
            {
                transformPool.getComponent(e1).pos += m_physics.overlap(transform1, Bbox1, transform2, Bbox2);
            }
        }
        if (m_physics.isCollided(transform1, Bbox1, transformPlayer, BboxPlayer))
            {
                transformPool.getComponent(e1).pos += m_physics.overlap(transform1, Bbox1, transformPlayer, BboxPlayer);
            }
        auto &viewImmovable = m_ECS.view<CImmovable>();
        for ( auto e : viewImmovable ){
            auto& transform = transformPool.getComponent(e);
            auto& Bbox = BboxPool.getComponent(e);
            if (m_physics.isCollided(transform1, Bbox1, transform, Bbox))
            {
                m_ECS.getComponent<CTransform>(e1).pos += m_physics.overlap(transform1, Bbox1, transform, Bbox);
            }
        }
    }

// ------------------------------- Projectile collisions ---------------------------------------------------------------------
    // auto& viewProj = m_ECS.view<CProjectileState>();
    auto& viewHealth = m_ECS.view<CHealth>();
    std::vector<EntityID> viewSignatureBbox             = m_ECS.signatureView<CAnimation, CBoundingBox, CHealth>();
    std::vector<EntityID> viewSignatureImmovable        = m_ECS.signatureView<CAnimation, CImmovable, CTransform>();
    std::vector<EntityID> viewSignatureProjectileState  = m_ECS.signatureView<CAnimation, CBoundingBox, CProjectileState>();
    for ( auto projectileID : viewSignatureProjectileState )
    {
        auto& transformProjectile = transformPool.getComponent(projectileID);
        auto& BboxProjectile = BboxPool.getComponent(projectileID);
        for ( auto enemyID : viewSignatureBbox )
        {   
            if (enemyID != m_player ) 
            {
                auto& transformEnemy = transformPool.getComponent(enemyID);
                auto& BboxEnemy = BboxPool.getComponent(enemyID);
                if (m_physics.isCollided(transformProjectile, BboxProjectile, transformEnemy, BboxEnemy))
                {
                    auto& animation     = m_ECS.getComponent<CAnimation>(projectileID);
                    animation.animation = m_game->assets().getAnimation("fireball_explode");
                    animation.repeat    = false;
                    transformProjectile.isMovable = false;
                    m_ECS.queueRemoveComponent<CBoundingBox>(projectileID);
                    m_ECS.queueRemoveComponent<CDamage>(projectileID);
                    // m_ECS.addComponent<CParent>(projectileID, enemyID, Vec2{32, 0}); // Need to remove child if parent dies
                    auto& health = viewHealth.getComponent(enemyID);
                    health.HP--;
                    health.damage_frame = m_currentFrame;
                }
            }
        }
        for ( auto enemyID : viewSignatureImmovable )
        {   
            auto& transformObstacle = transformPool.getComponent(enemyID);
            auto& BboxObstacle = BboxPool.getComponent(enemyID);
            if (m_physics.isCollided(transformProjectile, BboxProjectile, transformObstacle, BboxObstacle))
            {
                auto& animation     = m_ECS.getComponent<CAnimation>(projectileID);
                animation.animation = m_game->assets().getAnimation("fireball_explode");
                animation.repeat    = false;
                transformProjectile.isMovable = false;
                m_ECS.queueRemoveComponent<CBoundingBox>(projectileID);
                m_ECS.queueRemoveComponent<CDamage>(projectileID);
                // m_ECS.addComponent<CParent>(projectileID, enemyID, Vec2{32, 0}); // Need to remove child if parent dies
            }
        }
    }
}

void Scene_Play::sStatus() {
    // if ( m_ECS.getComponent<CHealth>(m_player).HP <= 0 ){
    auto& viewHealth = m_ECS.getComponentPool<CHealth>();
    for ( auto entityID : viewHealth)
    {
        if (viewHealth.getComponent(entityID).HP <= 0)
        {
            spawnCoin(m_ECS.getComponent<CTransform>(entityID).pos, 4);
            if ( m_player == entityID ){
                    m_game->changeScene("PLAY", std::make_shared<Scene_Play>(m_game, "assets/images/levels/levelStartingArea.png", true));
            }
            m_ECS.queueRemoveEntity(entityID);
        }
    }
}

void Scene_Play::sAnimation() {
    auto& view = m_ECS.view<CState>();
    // auto view = m_ECS.signatureView<CState, CAnimation>();
    auto& transformPool = m_ECS.getComponentPool<CTransform>();
    auto& statePool = m_ECS.getComponentPool<CState>();
    auto& animationPool = m_ECS.getComponentPool<CAnimation>();
    for ( auto e : view ){
        auto& transform = transformPool.getComponent(e);
        auto& state = statePool.getComponent(e);
        auto& animation = animationPool.getComponent(e).animation;
        if( transform.vel.isnull() ) {
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

    auto& viewProjectileState = m_ECS.view<CProjectileState>();
    for ( auto e : viewProjectileState ) {
        if ( viewProjectileState.getComponent(e).state == "Create" ) {
            if ( m_ECS.getComponent<CAnimation>(e).animation.hasEnded() ) {
                viewProjectileState.getComponent(e).state = "Ready";
                m_ECS.getComponent<CAnimation>(e).animation = m_game->assets().getAnimation("fireball");
                m_ECS.getComponent<CAnimation>(e).repeat = true;
                m_camera.startShake(4, 50);
            }
        }
    }

    auto& view2 = m_ECS.view<CAnimation>();
    for ( auto e : view2 ){
        auto& animation = animationPool.getComponent(e);
        if (animation.animation.hasEnded() && !animation.repeat) {
            m_ECS.queueRemoveEntity(e);
        } else{
            animation.animation.update();
        }
    }
}

void Scene_Play::sRender() {
    // Clear the screen with black
    SDL_SetRenderDrawColor(m_game->renderer(), 0, 0, 0, 255);
    SDL_RenderClear(m_game->renderer());

    if (m_drawTextures)
    {
        auto& viewBottom = m_ECS.view<CBottomLayer>();
        auto& transformPool = m_ECS.getComponentPool<CTransform>();
        auto& animationPool = m_ECS.getComponentPool<CAnimation>();
        for (auto e : viewBottom)
        {                
            auto& transform = transformPool.getComponent(e);
            auto& animation = animationPool.getComponent(e).animation;

            // Adjust the entity's position based on the camera position
            Vec2 adjustedPos = transform.pos - m_camera.position;
            // if (cameraZoom != 1) {

            // }
            animation.setScale(transform.scale*cameraZoom);
            animation.setAngle(transform.angle);
            animation.setDestRect(adjustedPos - animation.getDestSize()/2);
            
            spriteRender(animation);
        }

        auto& viewTop = m_ECS.view<CTopLayer>();
        auto& transformPool2 = m_ECS.getComponentPool<CTransform>();
        auto& animationPool2 = m_ECS.getComponentPool<CAnimation>();
        for (auto e : viewTop){
                
            auto& transform = transformPool2.getComponent(e);
            auto& animation = animationPool2.getComponent(e).animation;

            Vec2 adjustedPos = transform.pos - m_camera.position;

            animation.setScale(transform.scale*cameraZoom);
            animation.setAngle(transform.angle);
            animation.setDestRect(adjustedPos - animation.getDestSize()/2);
            
            spriteRender(animation);
        }    

        auto& viewHealth = m_ECS.getComponentPool<CHealth>();
        for ( auto entityID : viewHealth )
        {   
            if ( entityID == m_player ){continue;}
            auto& health = viewHealth.getComponent(entityID);
            if ( (int)(m_currentFrame - health.damage_frame) < health.heart_frames ) {

                auto& transform = transformPool.getComponent(entityID);
                Vec2 adjustedPos = transform.pos - m_camera.position;

                Animation animation;
                auto hearts = float(health.HP)/2;

                for (int i = 1; i <= health.HP_max/2; i++)
                {   
                    if ( hearts >= i ){
                        animation = health.animation_full;
                    } else if ( i-hearts == 0.5f ){
                        animation = health.animation_half;
                    } else{
                        animation = health.animation_empty;
                    }

                    animation.setScale(Vec2{2, 2}*cameraZoom);
                    animation.setDestRect(Vec2{
                        adjustedPos.x + (float)(i-1-(float)health.HP_max/4)*animation.getSize().x*animation.getScale().x, 
                        adjustedPos.y - m_ECS.getComponent<CAnimation>(entityID).animation.getSize().y * m_ECS.getComponent<CAnimation>(entityID).animation.getScale().y / 2
                    });
                    spriteRender(animation);
                }
            }
        }
        // Attemp to render coin balance in corner.

        // SDL_Rect texRect;
        // texRect.x = 0;
        // texRect.y = 96;
        // texRect.h = 128;
        // texRect.w = 128;

        // SDL_RenderCopyEx(
        //     m_game->renderer(), 
        //     m_game->assets().getTexture("coin_front"), 
        //     nullptr, 
        //     &texRect,
        //     0,
        //     NULL,
        //     SDL_FLIP_NONE
        // );

        Animation animation;
        auto hearts = float(m_ECS.getComponent<CHealth>(m_player).HP)/2;

        for (int i = 1; i <= m_ECS.getComponent<CHealth>(m_player).HP_max/2; i++)
        {   
            if ( hearts >= i ){
                animation = m_ECS.getComponent<CHealth>(m_player).animation_full;
            } else if ( i-hearts == 0.5f ){
                animation = m_ECS.getComponent<CHealth>(m_player).animation_half;
            } else{
                animation = m_ECS.getComponent<CHealth>(m_player).animation_empty;
            }

            animation.setScale(Vec2{4, 4});
            animation.setDestRect(Vec2{(float)(i-1)*animation.getSize().x*animation.getScale().x, 0});

            spriteRender(animation);
        }
    }
    
    if (m_drawCollision){
        auto& view = m_ECS.view<CBoundingBox>();
        auto& transformPool = m_ECS.getComponentPool<CTransform>();
        auto& BboxPool = m_ECS.getComponentPool<CBoundingBox>();
        for (auto e : view){      
            auto &transform = transformPool.getComponent(e);
            auto &box = BboxPool.getComponent(e);

            // Adjust the collision box position based on the camera position
            SDL_Rect collisionRect;
            collisionRect.x = (int)(transform.pos.x - box.halfSize.x - m_camera.position.x);
            collisionRect.y = (int)(transform.pos.y - box.halfSize.y - m_camera.position.y);
            collisionRect.w = (int)(box.size.x);
            collisionRect.h = (int)(box.size.y);

            SDL_SetRenderDrawColor(m_game->renderer(), 255, 255, 255, 255);
            SDL_RenderDrawRect(m_game->renderer(), &collisionRect);
        }
    }

    // SDL_Rect rect;
    // rect.x = 0;
    // rect.y = 0;
    // rect.w = width();
    // rect.h = height();

    // SDL_SetRenderDrawColor(m_game->renderer(), 20, 0, 50, 100);
    // SDL_RenderFillRect(m_game->renderer(), &rect);
}

void Scene_Play::spriteRender(Animation &animation){
    SDL_RenderCopyEx(
        m_game->renderer(), 
        animation.getTexture(), 
        animation.getSrcRect(), 
        animation.getDestRect(),
        animation.getAngle(),
        NULL,
        SDL_FLIP_NONE
    );
}

void Scene_Play::sAudio(){
    if( Mix_PlayingMusic() == 0 )
    {
        Mix_PlayMusic(m_game->assets().getMusic("music"), -1);
    }
}

void Scene_Play::spawnPlayer(){

    auto entityID = m_ECS.addEntity();
    m_player = entityID;

    int pos_x = m_playerConfig.x;
    int pos_y = m_playerConfig.y;
    int hp = m_playerConfig.HP;
    if (!m_newGame){
        std::ifstream file("game_save.txt");
        if (!file) {
            std::cerr << "Could not load game_save.txt file!\n";
            exit(-1);
        }
        std::string head;
        while (file >> head) {
            if (head == "Player_pos") {
                file >> pos_x >> pos_y;
            }
            if (head == "Player_hp") {
                file >> hp;
            }
        }
    }
    Vec2 pos = Vec2{64*(float)pos_x, 64*(float)pos_y};
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entityID);

    m_ECS.addComponent<CTransform>(entityID, midGrid, Vec2{0,0}, Vec2{4, 4}, 0.0f, m_playerConfig.SPEED, true);
    m_ECS.addComponent<CBoundingBox>(entityID, Vec2 {32, 32});

    m_ECS.addComponent<CName>(entityID, "wiz");
    m_ECS.addComponent<CAnimation>(entityID, m_game->assets().getAnimation("demon"), true, 3);
    m_ECS.addComponent<CTopLayer>(entityID);
    m_ECS.addComponent<CShadow>(entityID, m_game->assets().getAnimation("shadow"), false);

    m_ECS.addComponent<CInputs>(entityID);
    m_ECS.addComponent<CState>(entityID, PlayerState::STAND);

    m_ECS.addComponent<CDamage>(entityID, m_playerConfig.DAMAGE, 180);
    m_ECS.addComponent<CHealth>(entityID, hp, m_playerConfig.HP, 60, m_game->assets().getAnimation("heart_full"), m_game->assets().getAnimation("heart_half"), m_game->assets().getAnimation("heart_empty"));
    m_ECS.addComponent<CScript>(entityID).Bind<PlayerController>();
}

void Scene_Play::spawnWeapon(Vec2 pos){
    auto entity = m_ECS.addEntity();

    m_ECS.addComponent<CTransform>(entity, pos, Vec2{0,0}, Vec2{4, 4}, 0.0f, 0.0f, true);
    m_ECS.addComponent<CBoundingBox>(entity, Vec2 {24, 24});
    m_ECS.addComponent<CTopLayer>(entity);
    m_ECS.addComponent<CName>(entity, "staff");
    m_ECS.addComponent<CAnimation>(entity, m_game->assets().getAnimation("staff"), true, 2);
    m_ECS.addComponent<CDamage>(entity, 1, 180, std::unordered_set<std::string> {"Fire", "Explosive"});
    m_ECS.addComponent<CWeapon>(entity);
}

void Scene_Play::spawnObstacle(const Vec2 pos, bool movable, const int frame){
    auto entity = m_ECS.addEntity();
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid, Vec2 {0, 0}, Vec2 {0.5,0.5}, 0.0f, movable);
    m_ECS.addComponent<CBoundingBox>(entity, Vec2 {64, 64});
    m_ECS.addComponent<CImmovable>(entity);
}

void Scene_Play::spawnDragon(const Vec2 pos, bool movable, const std::string &ani) {
    auto entity = m_ECS.addEntity();
    m_ECS.addComponent<CAnimation>(entity, m_game->assets().getAnimation(ani), true, 3);
    m_ECS.addComponent<CTopLayer>(entity);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid,Vec2 {0, 0}, Vec2 {2, 2}, 0.0f, movable);
    m_ECS.addComponent<CHealth>(entity, (int)10, (int)10, 30, m_game->assets().getAnimation("heart_full"), m_game->assets().getAnimation("heart_half"), m_game->assets().getAnimation("heart_empty"));
    m_ECS.getComponent<CHealth>(entity).HPType = {""};
    m_ECS.addComponent<CBoundingBox>(entity, Vec2{96, 96});
    m_ECS.addComponent<CShadow>(entity, m_game->assets().getAnimation("shadow"), false);
    m_ECS.addComponent<CDamage>(entity, 2, 30);
}

void Scene_Play::spawnGrass(const Vec2 pos, const int frame)
{
    auto entity = m_ECS.addEntity();
    // std::vector<int> ranArray = generateRandomArray(1, m_ECS.getNumEntities(), 0, 15);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid, Vec2 {0, 0}, Vec2{0.5, 0.5}, 0.0f, false);
}

void Scene_Play::spawnDirt(const Vec2 pos, const int frame)
{
    auto entity = m_ECS.addEntity();
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid, Vec2 {0, 0}, Vec2{0.5, 0.5}, 0.0f, false);
}

void Scene_Play::spawnGoal(const Vec2 pos, bool movable)
{
    auto entity = m_ECS.addEntity();
    m_ECS.addComponent<CAnimation>(entity,m_game->assets().getAnimation("checkpoint_idle"), true, 3);
    m_ECS.addComponent<CTopLayer>(entity);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid,Vec2 {0, 0}, Vec2{4,4}, 0.0f, movable);
    m_ECS.addComponent<CBoundingBox>(entity, Vec2{32, 32});
}

void Scene_Play::spawnKey(const Vec2 pos, const std::string playerToUnlock, bool movable)
{
    auto entity = m_ECS.addEntity();
    m_ECS.addComponent<CAnimation>(entity, m_game->assets().getAnimation("m_texture_key"), true, 3);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid,Vec2 {0, 0}, Vec2 {1, 1}, 0.0f, movable);
    m_ECS.addComponent<CBoundingBox>(entity, Vec2{32, 32});
}

void Scene_Play::spawnLava(const Vec2 pos, const std::string tag, const int frame)
{
    auto entity = m_ECS.addEntity();
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid,Vec2 {0, 0}, false);
    m_ECS.addComponent<CImmovable>(entity);
    m_ECS.addComponent<CBoundingBox>(entity, Vec2{64, 64});
}

void Scene_Play::spawnWater(const Vec2 pos, const std::string tag, const int frame)
{
    auto entity = m_ECS.addEntity();
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid,Vec2 {0, 0}, false);
    m_ECS.addComponent<CImmovable>(entity);
    m_ECS.addComponent<CBoundingBox>(entity, Vec2{64, 64});
}

void Scene_Play::spawnBridge(const Vec2 pos, const int frame)
{
    auto entity = m_ECS.addEntity();
    m_ECS.addComponent<CAnimation>(entity, m_game->assets().getAnimation("bridge"), true, 3);
    m_ECS.getComponent<CAnimation>(entity).animation.setTile(Vec2{(float)(frame % 4), (float)(int)(frame / 4)});
    m_ECS.addComponent<CTopLayer>(entity);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid,Vec2 {0, 0}, Vec2{2,2}, 0.0f, false);
    m_ECS.addComponent<CBoundingBox>(entity, Vec2{64, 64});
}

void Scene_Play::spawnProjectile(EntityID creator, Vec2 vel)
{
    auto entity = m_ECS.addEntity();
    m_ECS.addComponent<CAnimation>(entity, m_game->assets().getAnimation("fireball_create"), false, 3);
    m_ECS.addComponent<CTopLayer>(entity);
    m_ECS.addComponent<CTransform>(entity, m_ECS.getComponent<CTransform>(creator).pos, vel, Vec2{2, 2}, vel.angle(), 400.0f, false);
    // m_ECS.addComponent<CBoundingBox>(entity, Vec2{12, 12});
    m_ECS.addComponent<CDamage>(entity, m_ECS.getComponent<CDamage>(creator).damage, m_ECS.getComponent<CDamage>(creator).speed); // damage speed 6 = frames between attacking
    m_ECS.getComponent<CDamage>(entity).damageType = {"Fire", "Explosive"};
    m_ECS.addComponent<CProjectileState>(entity, "Create");
    m_ECS.addComponent<CParent>(entity, creator);
    m_ECS.addComponent<CProjectile>(creator, entity);
}

void Scene_Play::spawnCoin(Vec2 pos, const size_t layer)
{
    auto entity = m_ECS.addEntity();
    m_ECS.addComponent<CAnimation>(entity, m_game->assets().getAnimation("coin"), true, 3);
    m_ECS.addComponent<CTopLayer>(entity);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid, Vec2{0,0}, Vec2{4,4}, 0.0f, false);
    m_ECS.addComponent<CBoundingBox>(entity, Vec2{32, 32});
    m_ECS.addComponent<CShadow>(entity, m_game->assets().getAnimation("shadow"), false);
    m_ECS.addComponent<CLoot>(entity);
}

void Scene_Play::spawnSmallEnemy(Vec2 pos, const size_t layer, std::string type)
{
    auto entity = m_ECS.addEntity();
    m_ECS.addComponent<CName>(entity, type);
    m_ECS.addComponent<CAnimation>(entity, m_game->assets().getAnimation(type), true, 3);
    m_ECS.addComponent<CTopLayer>(entity);
    m_ECS.addComponent<CState>(entity, PlayerState::STAND);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid, Vec2{0,0}, Vec2{4,4}, 0.0f, 150.0f, true);
    m_ECS.addComponent<CBoundingBox>(entity, Vec2{32, 48});
    m_ECS.addComponent<CPathfind>(entity, m_ECS.getComponent<CTransform>(m_player).pos);
    m_ECS.addComponent<CShadow>(entity, m_game->assets().getAnimation("shadow"), false);
    m_ECS.addComponent<CHealth>(entity, 4, 4, 30, m_game->assets().getAnimation("heart_full"), m_game->assets().getAnimation("heart_half"), m_game->assets().getAnimation("heart_empty"));
    m_ECS.getComponent<CHealth>(entity).HPType = {"Grass", "Organic"};
    m_ECS.addComponent<CDamage>(entity, 1, 60);
    m_ECS.addComponent<CScript>(entity).Bind<RooterController>();

}

void Scene_Play::spawnDualTiles(const Vec2 pos, std::unordered_map<std::string, int> tileTextureMap)
{   
    for (const auto& [tileKey, textureIndex] : tileTextureMap) {
        std::string tile = tileKey;
        int layer = 10;
        
        if (tile == "water") {
            layer = layer - 1;
        } else if (tile == "lava") {
            layer = layer - 1;
        } else if (tile == "cloud") {
            layer = layer - 2;
        } else if (tile == "obstacle") {
            layer = layer - 2;
            tile = "mountain";  // Change the tile name for "obstacle"
        }

        EntityID entity = m_ECS.addEntity();
        m_ECS.addComponent<CAnimation>(entity, m_game->assets().getAnimation(tile + "_dual_sheet"), true, 10);
        m_ECS.getComponent<CAnimation>(entity).animation.setTile(Vec2{(float)(textureIndex % 4), (float)(int)(textureIndex / 4)});                           
        m_ECS.addComponent<CBottomLayer>(entity);
        Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
        m_ECS.addComponent<CTransform>(entity, midGrid, Vec2{0, 0}, Vec2{4, 4}, 0.0f, false);
        // m_ECS.addComponent<CName>(entity, tile);
    }
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

void Scene_Play::onEnd() {
    m_game->changeScene("Menu", std::make_shared<Scene_Menu>(m_game));
}

void Scene_Play::setPaused(bool pause) {
    m_pause = pause;
}

Vec2 Scene_Play::gridSize(){
    return m_gridSize;
}

Vec2 Scene_Play::levelSize(){
    return m_levelSize;
}

Vec2 Scene_Play::gridToMidPixel(float gridX, float gridY, EntityID entity) {
    Vec2 offset;
    Vec2 grid = Vec2{gridX, gridY};
    Vec2 eSize;
    if ( m_ECS.hasComponent<CAnimation>(entity) ){
        eSize = m_ECS.getComponent<CAnimation>(entity).animation.getSize();
    } else {
        eSize = m_gridSize/2;
    }
    
    Vec2 eScale;
    switch ((int)eSize.y) {
        case 270:
            eScale.x = 0.15f;
            eScale.y = 0.18f;
            break;
        case 225:
            eScale.x = 0.18f;
            eScale.y = 0.18f;
            break;
        case 192:
            eScale.x = 1.0f;
            eScale.y = 1.0f;
            eSize.x = 64.0f;
            eSize.y = 64.0f;
            break;
        case 128:
            eScale.x = 2.0f;
            eScale.y = 2.0f;
            eSize.x = 32.0f;
            eSize.y = 32.0f;
            break;
        case 64:
            eScale.x = 1.0f;
            eScale.y = 1.0f;
            break;
        case 32:
            eScale.x = 2.0f;
            eScale.y = 2.0f;
            break;
        case 16:
            eScale.x = 4.0f;
            eScale.y = 4.0f;
            break;
        case 24:
            eScale.x = 2.0f;
            eScale.y = 2.0f;
            break; 
        default:
            eScale.x = 1.0f;
            eScale.y = 1.0f;
    }
    
    offset = (m_gridSize - eSize * eScale) / 2.0;

    return grid + m_gridSize / 2 - offset;
}