#include "Scene_Menu.h"
#include "Scene_Play.h"
#include "Sprite.h"
#include "Assets.h"
#include "Game.h"
#include "Components.h"
#include "Action.h"

#include "RandomArray.h"

#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#include <iostream>
#include <string>
#include <fstream>
#include <unordered_map>
#include <unordered_set>

Scene_Menu::Scene_Menu(Game* game)
    : Scene(game)
{
    init();
}

void Scene_Menu::init() {
    registerAction(SDLK_ESCAPE, "QUIT");
    registerAction(SDLK_t, "TOGGLE_TEXTURE");
    registerAction(SDLK_c, "TOGGLE_COLLISION");
    registerAction(SDLK_1, "LEVEL0");
    registerAction(SDLK_2, "LEVEL5");
    registerAction(SDL_BUTTON_LEFT , "MOUSE LEFT CLICK");
    registerAction(SDLK_v , "SHOW COORDINATES");
    loadMenu();

    TTF_Font* font = m_game->assets().getFont("minecraft");
    SDL_Color textColor = {255, 0, 0}; // White color
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, "HEAVEN & HELL", textColor);
    if (textSurface == nullptr) {
        std::cerr << "Unable to render text surface! TTF_Error: " << TTF_GetError() << std::endl;
    }

    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(m_game->renderer(), textSurface);
    if (textTexture == nullptr) {
        std::cerr << "Unable to create texture from rendered text! SDL_Error: " << SDL_GetError() << std::endl;
    }
    std::string name = "test";
    m_game->assets().addTexture(name, textTexture);


    SDL_FreeSurface(textSurface);

}

Vec2 Scene_Menu::gridToMidPixel(float gridX, float gridY, std::shared_ptr<Entity> entity) {
    Vec2 offset;
    Vec2 grid = Vec2{gridX, gridY};
    Vec2 eSize;
    if ( entity->hasComponent<CAnimation>() ){
        eSize = entity->getComponent<CAnimation>().animation.getSize();
    } else {
        eSize = m_gridSize/2;
    }
    
    Vec2 eScale;
    switch ((int)eSize.y) {
        case 270:
            eScale.x = 0.15;
            eScale.y = 0.18;
            break;
        case 225:
            eScale.x = 0.18;
            eScale.y = 0.18;
            break;
        case 192:
            eScale.x = 1;
            eScale.y = 1;
            eSize.x = 64;
            eSize.y = 64;
            break;
        case 128:
            eScale.x = 2;
            eScale.y = 2;
            eSize.x = 32;
            eSize.y = 32;
            break;
        case 64:
            eScale.x = 1.0;
            eScale.y = 1.0;
            break;
        case 32:
            eScale.x = 2.0;
            eScale.y = 2.0;
            break;
        case 16:
            eScale.x = 4.0;
            eScale.y = 4.0;
            break;
        case 24:
            eScale.x = 2.0;
            eScale.y = 2.0;
            break; 
        default:
            eScale.x = 1.0;
            eScale.y = 1.0;
    }
    
    offset = (m_gridSize - eSize * eScale) / 2.0;

    return grid + m_gridSize / 2 - offset;
}

void Scene_Menu::loadMenu(){
    // spawnTitleScreen
    // spawnLevel(Vec2 {float(width()/2),float(height()/2)}, "title_screen");
    size_t layer = 10;
    auto entity = m_entities.addEntity("Level", layer);
    entity->addComponent<CAnimation> (m_game->assets().getAnimation("level0_screenshot"), true);
    entity->addComponent<CTransform>(Vec2 {float(width()/2),float(height()/2)},Vec2 {0, 0}, false);
    entity->getComponent<CTransform>().scale = Vec2{1, 1};
    entity->addComponent<CName>("title_screen");

    layer = 9;
    entity = m_entities.addEntity("Level", layer);
    entity->addComponent<CAnimation> (m_game->assets().getAnimation("game_title"), true);
    entity->addComponent<CTransform>(Vec2 {1250, 180},Vec2 {0, 0}, false);
    entity->getComponent<CTransform>().scale = Vec2{1.2, 1.2};
    entity->addComponent<CName>("game_title");

    spawnButton(Vec2 {64*3.2,64*8.1}, "button_unpressed", "new");
    spawnButton(Vec2 {64*3.2,64*9.7}, "button_unpressed", "continue");

    m_entities.update();
    m_entities.sort();
}

void Scene_Menu::spawnLevel(const Vec2 pos, std::string level)
{   
    size_t layer = 10;
    auto entity = m_entities.addEntity("Level", layer);
    entity->addComponent<CAnimation> (m_game->assets().getAnimation(level), true);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    entity->addComponent<CTransform>(midGrid,Vec2 {0, 0}, false);
    entity->getComponent<CTransform>().scale = Vec2{3,3};
    entity->addComponent<CBoundingBox>(entity->getComponent<CAnimation>().animation.getSize()*3);
    entity->addComponent<CName>(level);
}

void Scene_Menu::spawnButton(const Vec2 pos, const std::string& unpressed, const std::string& name)
{   
    size_t layer = 10;
    auto entity = m_entities.addEntity("Button", layer);
    entity->addComponent<CAnimation> (m_game->assets().getAnimation(unpressed), true);
    // Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    entity->addComponent<CTransform>(pos,Vec2 {0, 0}, false);
    entity->getComponent<CTransform>().scale = Vec2{2,2};
    entity->addComponent<CBoundingBox>(entity->getComponent<CAnimation>().animation.getSize()*2);
    entity->addComponent<CName>(name);
}

void Scene_Menu::spawnDualTile(const Vec2 pos, std::string tile, const int frame)
{   
    size_t layer = 10;
    if (tile == "water"){layer=layer-1;}
    if (tile == "lava"){layer=layer-1;}
    if (tile == "cloud"){layer=layer-2;}
    if (tile == "obstacle"){
        layer = layer-2;
        tile = std::string("mountain");}
    auto entity = m_entities.addEntity("DualTile", layer);
    entity->addComponent<CAnimation> (m_game->assets().getAnimation(tile+"_dual_sheet"), true);
    
    float size;
    float scale;
    if (entity->getComponent<CAnimation>().animation.getSize() == Vec2{64,64}){
        size = 16;
        scale = 1;
    } else{
        size = 32;
        scale = 0.5;
    }
    
    entity->addComponent<CTexture>(Vec2 {(float)(frame%4)*size, (float)(int)(frame/4)*size}, Vec2 {size, size}, m_game->assets().getTexture(tile+"_dual_sheet"));
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    entity->addComponent<CTransform>(midGrid,Vec2 {0, 0}, false);
    entity->getComponent<CTransform>().scale = Vec2{scale,scale};
}

void Scene_Menu::sDoAction(const Action& action) {
    if (action.type() == "START") {
        if (action.name() == "TOGGLE_TEXTURE") {
            m_drawTextures = !m_drawTextures; 
        } else if (action.name() == "TOGGLE_COLLISION") { 
            m_drawCollision = !m_drawCollision; 
        } else if (action.name() == "TOGGLE_GRID") { 
            m_drawDrawGrid = !m_drawDrawGrid; 
        } else if (action.name() == "SHOW COORDINATES") { 
            m_drawCoordinates = !m_drawCoordinates; 
        } else if (action.name() == "QUIT") { 
            onEnd();
        } else if (action.name() == "MOUSE LEFT CLICK") {
            for (auto e : m_entities.getEntities("Button")){
                auto &transform = e->getComponent<CTransform>();
                auto &Bbox = e->getComponent<CBoundingBox>();
                if ( m_mousePosition.x < transform.pos.x + Bbox.halfSize.x && m_mousePosition.x >= transform.pos.x -Bbox.halfSize.x ){
                    if ( m_mousePosition.y < transform.pos.y + Bbox.halfSize.y && m_mousePosition.y >= transform.pos.y -Bbox.halfSize.y ){
                        e->addComponent<CAnimation>(m_game->assets().getAnimation("button_pressed"), true);
                    }
                }
            }
        }   
    }

    else if (action.type() == "END") {
        if (action.name() == "MOUSE LEFT CLICK") {
            for (auto b : m_entities.getEntities("Button")){
                auto &transform = b->getComponent<CTransform>();
                auto &Bbox = b->getComponent<CBoundingBox>();
                if ( m_mousePosition.x < transform.pos.x + Bbox.halfSize.x && m_mousePosition.x >= transform.pos.x -Bbox.halfSize.x ){
                    if ( m_mousePosition.y < transform.pos.y + Bbox.halfSize.y && m_mousePosition.y >= transform.pos.y -Bbox.halfSize.y ){
                        if ( b->getComponent<CName>().name == "new" ){
                            m_game->changeScene("PLAY", std::make_shared<Scene_Play>(m_game, "assets/images/levels/level0.png", true));
                        } else if ( b->getComponent<CName>().name == "continue" ){
                            m_game->changeScene("PLAY", std::make_shared<Scene_Play>(m_game, "assets/images/levels/level0.png", false));
                        }
                    }
                }
            }
        }   
    }
}

void Scene_Menu::update() {
    m_entities.update();
    sAnimation();
    sRender();
}

void Scene_Menu::sMovement() {
    // for (auto e : m_entities.getEntities()){    
    //     auto &transform = e->getComponent<CTransform>(); 
        
    // }
}

void Scene_Menu::sCollision() {
    // for ( auto p : m_entities.getEntities() )
    // {
        
    // }
}

void Scene_Menu::sAnimation() {
    for ( auto e : m_entities.getEntities() ){
        if (e->hasComponent<CAnimation>())
        {
            e->getComponent<CAnimation>().animation.update();
        }
    }
}

void Scene_Menu::sRender() {
    
    // Clear the screen with black
    SDL_SetRenderDrawColor(m_game->renderer(), 0, 0, 0, 255);
    SDL_RenderClear(m_game->renderer());

    if (m_drawTextures){
        for (auto e : m_entities.getEntities()){        
            if ( e->hasComponent<CTransform>() && e->hasComponent<CAnimation>()){

                auto& transform = e->getComponent<CTransform>();
                auto& animation = e->getComponent<CAnimation>().animation;

                SDL_Rect texRect;
                texRect.x = e->getComponent<CTexture>().pos.x;
                texRect.y = e->getComponent<CTexture>().pos.y;
                texRect.w = e->getComponent<CTexture>().size.x;
                texRect.h = e->getComponent<CTexture>().size.y;

                // Adjust the entity's position based on the camera position
                Vec2 adjustedPos = transform.pos;

                // Set the destination rectangle for rendering
                animation.setScale(transform.scale*cameraZoom);
                animation.setDestRect(adjustedPos - animation.getDestSize()/2);
                animation.setAngle(transform.angle);
                
                if (animation.frames() == 111){
                    SDL_RenderCopyEx(
                        m_game->renderer(), 
                        animation.getTexture(), 
                        &texRect, 
                        animation.getDestRect(),
                        animation.getAngle(),
                        NULL,
                        SDL_FLIP_NONE
                    );
                } 
                else {
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
                // if (e->tag() == "Button"){
                //     SDL_RenderCopy(m_game->renderer(), m_game->assets().getTexture("test"), nullptr, animation.getDestRect());
                // }
            }
        }
    }

    if (m_drawCollision){
        for (auto e : m_entities.getEntities()){      
            if ( e->hasComponent<CTransform>() && e->hasComponent<CBoundingBox>() ){
                auto& transform = e->getComponent<CTransform>();
                auto& box = e->getComponent<CBoundingBox>();

                // Adjust the collision box position based on the camera position
                SDL_Rect collisionRect;
                collisionRect.x = static_cast<int>(transform.pos.x - box.halfSize.x);
                collisionRect.y = static_cast<int>(transform.pos.y - box.halfSize.y);
                collisionRect.w = static_cast<int>(box.size.x);
                collisionRect.h = static_cast<int>(box.size.y);

                SDL_SetRenderDrawColor(m_game->renderer(), 255, 255, 255, 255);
                SDL_RenderDrawRect(m_game->renderer(), &collisionRect);
            }
        }
    }

    if (m_drawCoordinates) {
        std::cout << m_mousePosition.x << " " << m_mousePosition.y << std::endl;
    } 
}

void Scene_Menu::onEnd() {
    m_game->quit();
}

void Scene_Menu::setPaused(bool pause) {
    m_pause = pause;
}

void Scene_Menu::changePlayerStateTo(PlayerState s) {
    auto& prev = m_player->getComponent<CState>().preState; 
    if (prev != s) {
        prev = m_player->getComponent<CState>().state;
        m_player->getComponent<CState>().state = s; 
        m_player->getComponent<CState>().changeAnimate = true;
    }
    else { 
        m_player->getComponent<CState>().changeAnimate = false;
    }
}

std::vector<bool> Scene_Menu::neighborCheck(const std::vector<std::vector<std::string>>& pixelMatrix, const std::string &pixel, int x, int y, int width, int height) {
    std::vector<std::string> friendlyPixels(1, "");
    std::vector<bool> neighbors(4, false); // {top, bottom, left, right}
    if ( pixel == "grass" || pixel == "dirt"){
        friendlyPixels = {"grass", "dirt", "key", "goal", "player_God", "player_Devil", "dragon", "water"};
    } else if ( pixel == "water" ){
        friendlyPixels = {"water", "bridge"};
    } else if (pixel == "lava"){
        friendlyPixels = {"lava", "bridge"};
    }
    else{
        friendlyPixels = {pixel};
    }
    for ( auto pix : friendlyPixels){
        if(!neighbors[0]){neighbors[0] = (y > 0 && pixelMatrix[y - 1][x] == pix);}           // top
        if(!neighbors[1]){neighbors[1] = (x < width - 1 && pixelMatrix[y][x + 1] == pix);}   // right
        if(!neighbors[2]){neighbors[2] = (y < height - 1 && pixelMatrix[y + 1][x] == pix);}  // bottom
        if(!neighbors[3]){neighbors[3] = (x > 0 && pixelMatrix[y][x - 1] == pix);}           // left
    }
    return neighbors;

}

std::vector<std::string> Scene_Menu::neighborTag(const std::vector<std::vector<std::string>>& pixelMatrix, const std::string &pixel, int x, int y, int width, int height) {
    
    std::vector<std::string> neighborsTags(4, "nan"); // {top, bottom, left, right}
    if(y > 0){neighborsTags[0] = pixelMatrix[y - 1][x];}            // top
    if(x < width - 1){neighborsTags[1] = pixelMatrix[y][x + 1];}    // right
    if(y < height - 1){neighborsTags[2] = pixelMatrix[y + 1][x];}   // bottom
    if(x > 0 ){neighborsTags[3] = pixelMatrix[y][x - 1];}           // left

    return neighborsTags;
}

int Scene_Menu::getObstacleTextureIndex(const std::vector<bool>& neighbors) {
    int numObstacles = std::count(neighbors.begin(), neighbors.end(), true);
    if (numObstacles == 1) {
        if (neighbors[0]) return 12;    // Top
        if (neighbors[1]) return 1;     // Right
        if (neighbors[2]) return 4;     // Bottom
        if (neighbors[3]) return 3;     // Left
    } else if (numObstacles == 2) {
        if (!neighbors[0] && !neighbors[1]) return 7;  // Top & Right
        if (!neighbors[1] && !neighbors[2]) return 15; // Right & Bottom
        if (!neighbors[2] && !neighbors[3]) return 13; // Bottom & Left
        if (!neighbors[3] && !neighbors[0]) return 5;  // Left & Top
        if (!neighbors[0] && !neighbors[2]) return 2;  // Top & Bottom
        if (!neighbors[1] && !neighbors[3]) return 8; // Right & Left
    } else if (numObstacles == 3) {
        if (!neighbors[0]) return 6;    // Top is not an obstacle
        if (!neighbors[1]) return 11;   // Right is not an obstacle
        if (!neighbors[2]) return 14;   // Bottom is not an obstacle
        if (!neighbors[3]) return 9;    // Left is not an obstacle
    } else if (numObstacles == 4) {
        return 10; // All neighbors are obstacles
    }
    return 0; // No neighbors are obstacles
}

std::vector<std::vector<std::string>> Scene_Menu::createPixelMatrix(Uint32* pixels, SDL_PixelFormat* format, int width, int height) {
    std::vector<std::vector<std::string>> pixelMatrix(height, std::vector<std::string>(width, ""));
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            Uint32 pixel = pixels[y * width + x];

            Uint8 r, g, b, a;
            SDL_GetRGBA(pixel, format, &r, &g, &b, &a);

            if ((int)r == 192 && (int)g == 192 && (int)b == 192) {
                pixelMatrix[y][x] = "obstacle";
            } else if ((int)r == 200 && (int)g == 240 && (int)b == 255) {
                pixelMatrix[y][x] = "cloud";
            } else if ((int)r == 203 && (int)g == 129 && (int)b == 56) {
                pixelMatrix[y][x] = "dirt";
            } else if ((int)r == 0 && (int)g == 255 && (int)b == 0) {
                pixelMatrix[y][x] = "grass";
            } else if ((int)r == 255 && (int)g == 255 && (int)b == 255) {
                pixelMatrix[y][x] = "player_God";
            } else if ((int)r == 0 && (int)g == 0 && (int)b == 0) {
                pixelMatrix[y][x] = "player_Devil";
            } else if ((int)r == 255 && (int)g == 0 && (int)b == 255) {
                pixelMatrix[y][x] = "key";
            } else if ((int)r == 255 && (int)g == 255 && (int)b == 0) {
                pixelMatrix[y][x] = "goal";
            } else if ((int)r == 9 && (int)g == 88 && (int)b == 9) {
                pixelMatrix[y][x] = "dragon";
            } else if ((int)r == 255 && (int)g == 0 && (int)b == 0) {
                pixelMatrix[y][x] = "lava";
            } else if ((int)r == 0 && (int)g == 0 && (int)b == 255) {
                pixelMatrix[y][x] = "water";
            } else if ((int)r == 179 && (int)g == 0 && (int)b == 255) {
                pixelMatrix[y][x] = "bridge";
            } else {
                pixelMatrix[y][x] = "unknown";
            }
        }
    }
    return pixelMatrix;
}

void Scene_Menu::spawnDualGrid(std::vector<std::vector<std::string>> pixelMatrix, int x, int y) {
    std::vector<std::string> tileQ = std::vector<std::string>(4, "");
    // std::vector<bool> neighbors(4, false); // {top, bottom, left, right}
    int textureIndex;
    // std::cout << tileQ[2] << std::endl;
    // std::cout << pixelMatrix[y][x] << std::endl;
    tileQ[1] = pixelMatrix[y][x];   //Q4
    if (x>0)        {tileQ[0] = pixelMatrix[y][x-1];}    else {tileQ[0] = pixelMatrix[y][x];}  // Q3
    if (y>0)        {tileQ[2] = pixelMatrix[y-1][x];}    else {tileQ[2] = pixelMatrix[y][x];}  // Q1
    if (x>0 && y>0) {tileQ[3] = pixelMatrix[y-1][x-1];}  else {tileQ[3] = pixelMatrix[y][x];}  // Q2

    std::unordered_map<std::string, std::unordered_set<std::string>> friendlyNeighbors = {
        {"grass", {"key", "goal", "player_God", "dragon"}},
        {"dirt", {"key", "goal", "player_Devil", "dragon"}}
    };
            
    for (std::string tile : {"grass", "dirt", "water", "lava", "cloud", "obstacle", "bridge"})
    {
        if ( std::find(tileQ.begin(), tileQ.end(), "bridge") != tileQ.end() ){

            if ( std::find(tileQ.begin(), tileQ.end(), "water") != tileQ.end() ){
                std::transform(tileQ.begin(), tileQ.end(), tileQ.begin(), [](const std::string& str) {
                    return (str == "bridge" ) ? "water" : str;
                });
            } else if ( std::find(tileQ.begin(), tileQ.end(), "lava") != tileQ.end() ){
                    std::transform(tileQ.begin(), tileQ.end(), tileQ.begin(), [](const std::string& str) {
                    return (str == "bridge" ) ? "lava" : str;
                });
            } else {
                std::transform(tileQ.begin(), tileQ.end(), tileQ.begin(), [](const std::string& str) {
                    return (str == "bridge" ) ? "" : str;
                });
            }
        }
        
         // Transform the vector based on friendly neighbors for the current tile type
        std::transform(tileQ.begin(), tileQ.end(), tileQ.begin(), [&](const std::string& str) {
            // Check if the current string is a friendly neighbor for the current tile
            if (friendlyNeighbors[tile].count(str)) {
                return tile; // Replace it with the current tile
            }
            return str; // Keep the original if it's not a friendly neighbor
        });

        int numTiles = std::count(tileQ.begin(), tileQ.end(), tile);
        if (numTiles > 0){
            if (numTiles == 4) {
                textureIndex = 6; // All quadrants are tiles
            } else if (numTiles == 3) {
                if (tileQ[0] != tile) textureIndex = 10;
                if (tileQ[1] != tile) textureIndex = 7;
                if (tileQ[2] != tile) textureIndex = 2;
                if (tileQ[3] != tile) textureIndex = 5;
            } else if (numTiles == 2) {
                if (tileQ[0] != tile && tileQ[1] != tile) textureIndex = 9;
                if (tileQ[1] != tile && tileQ[2] != tile) textureIndex = 11;
                if (tileQ[2] != tile && tileQ[3] != tile) textureIndex = 3;
                if (tileQ[3] != tile && tileQ[0] != tile) textureIndex = 1;
                if (tileQ[0] != tile && tileQ[2] != tile) textureIndex = 4;
                if (tileQ[1] != tile && tileQ[3] != tile) textureIndex = 14; 
            } if (numTiles == 1) {
                if (tileQ[0] == tile) textureIndex = 0;
                if (tileQ[1] == tile) textureIndex = 13;
                if (tileQ[2] == tile) textureIndex = 8;
                if (tileQ[3] == tile) textureIndex = 15;
            }
            if (tile != ""){
                spawnDualTile(Vec2 {64*(float)x-32, 64*(float)y-32}, tile, textureIndex);
            }
        }
    }
}