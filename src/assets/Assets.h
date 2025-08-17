#pragma once

#include <map>
#include <string>
#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include "Animation.h"

class Assets
{
	std::map<std::string, SDL_Texture*> m_textures;
	std::map<std::string, Animation> m_animations;
	std::map<std::string, TTF_Font*> m_fonts;
	std::map<std::string, Mix_Chunk*> m_audios;
	std::map<std::string, Mix_Music*> m_music;

public:

	Assets();

	void addTexture(const std::string & path, SDL_Renderer * ren);
	void addAnimation(const std::string& name, Animation animation);
	void addAudio(const std::string& name, const std::string& path);
	void addMusic(const std::string& name, const std::string& path);
	void addFont(const std::string& name, const std::string& path, const int font_size);

	SDL_Texture* getTexture(std::string name) const;
	TTF_Font* getFont(const std::string& name) const;
	const Animation& getAnimation(const std::string& name) const;
	Mix_Chunk* getAudio(const std::string& name) const;
	Mix_Music* getMusic(const std::string& name) const;

	void loadFromFile(
		const std::string & pathImages, 
		const std::string & pathText, 
		SDL_Renderer * ren
	);
};