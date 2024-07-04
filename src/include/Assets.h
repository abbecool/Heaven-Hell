#pragma once

#include "Animation.h"
// #include "SFML/Graphics/Font.hpp"
// #include "SFML/Graphics/Texture.hpp"
#include <map>
#include <string>
// #include <SDL2/SDL.h>

class Assets
{
	std::map<std::string, SDL_Texture*> m_textures;
	std::map<std::string, Animation> m_animations;
	// std::map<std::string, sf::Font>      m_fonts;

public:

	Assets();

	void addTexture(std::string name, const std::string & path, SDL_Renderer * ren);
	void addAnimation(const std::string& name, Animation animation);
	// void addFont(const std::string& name, const std::string& path);

	SDL_Texture* getTexture(std::string name) const;
	const Animation& getAnimation(const std::string& name) const;
	// const sf::Font& getfont(const std::string& name) const;

	void loadFromFile(const std::string & path, SDL_Renderer * ren);
};