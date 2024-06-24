#include "EntityManager.h"

struct PlayerConfig { int SR, CR;};
struct EnemyConfig { int SR, CR;};

class Game
{    
    SDL_Renderer *m_renderer;
    // SDL_Window *window;
    EntityManager m_entities;
    PlayerConfig m_playerConfig;
    EnemyConfig m_enemyConfig;
    std::shared_ptr<Entity> m_player;
    int m_score;
    int m_speed = 400;
    int m_currentFrame;
    bool m_paused;
    bool m_running = true;
    int m_height;
    int m_width;
    int m_framerate = 60;
    SDL_Texture *m_texture;
    SDL_Texture *m_texture_field;
    SDL_Texture *m_texture_releif;
    SDL_Texture *m_texture_angel;
    SDL_Texture *m_texture_devil;
    SDL_Texture *m_texture_key;
    SDL_Texture *m_texture_boulder_small;
    SDL_Texture *m_texture_outofbound;
    
    void init(const std::string & config);
    void setPaused(bool paused);
    void getHeight();
    int getWidth();

    void sMovement();
    void sUserInput();
    void sRender();
    void sCollisions();

    bool isCollided(std::shared_ptr<Entity>, std::shared_ptr<Entity>);
    Vec2 Overlap(std::shared_ptr<Entity>, std::shared_ptr<Entity>);
    void spawnPlayer(const Vec2 pos, const std::string name, bool movable);
    void spawnObstacle(const Vec2 pos, const Vec2 size, bool movable );
    void spawnBackground(const Vec2 pos, const Vec2 size, bool movable);
    void spawnWorldBorder(const Vec2 pos, const Vec2 size, bool movable);
    void spawnOutofboundBorder(const Vec2 pos, const Vec2 size, bool movable);
    void spawnGoal(const Vec2 pos, const Vec2 size, bool movable);
    void spawnKey(const Vec2 pos, const Vec2 size, const std::string, bool movable);
public:
    Game(const std::string & config, SDL_Renderer * renderer, const int H, const int W);
    void run();
};