#pragma once

#include <map>
#include <string>
#include <SDL3_mixer/SDL_mixer.h>
#include "SpriteDefinition.hpp"

class RenderBackend;

class Assets
{
	std::map<std::string, SpriteDefinition> m_sprites;
	std::map<std::string, MIX_Audio*> m_audios;
	std::map<std::string, MIX_Audio*> m_music;
	MIX_Mixer* m_mixer = nullptr;

	bool ensureMixer();

public:

	Assets();
	~Assets();

	void addSprite(const std::string& name, SpriteDefinition sprite);
	void addAudio(const std::string& name, const std::string& path);
	void addMusic(const std::string& name, const std::string& path);

	const SpriteDefinition& getSprite(const std::string& name) const;
	MIX_Audio* getAudio(const std::string& name) const;
	MIX_Audio* getMusic(const std::string& name) const;
	void playAudio(const std::string& name);
	void shutdown();

	void loadFromFile(
		const std::string & pathImages, 
		const std::string & pathText, 
		RenderBackend& renderBackend
	);
};
