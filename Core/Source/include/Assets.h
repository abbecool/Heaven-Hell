#pragma once

#include <map>
#include <string>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "Animation.h"

class Assets
{
	std::map<std::string, SDL_Texture*> m_textures;
	std::map<std::string, Animation> m_animations;
	std::map<std::string, TTF_Font*> m_fonts;

public:

	Assets();

	void addTexture(std::string name, SDL_Texture* texture);
	void addTexture(std::string name, const std::string & path, SDL_Renderer * ren);
	void addAnimation(const std::string& name, Animation animation);
	void addFont(const std::string& name, const std::string& path);

	SDL_Texture* getTexture(std::string name) const;
	TTF_Font* getFont(const std::string& name) const;
	const Animation& getAnimation(const std::string& name) const;

	void loadFromFile(const std::string & pathImages, const std::string & pathText, SDL_Renderer * ren);
};