#include <map>
#include <string>
#include <SDL2/SDL.h>
#include <C:\projects\privat\simple_game\src\include\SDL2\SDL_image.h>

class Assets
{
    std::map< std::string, SDL_Texture*> m_textures;
public:
    Assets(){}
    void addTexture(const std::string name, const char* path){}
    SDL_Texture& getTexture(const std::string name){}
    
};
