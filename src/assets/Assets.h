#pragma once

#include <map>
#include <string>
#include <SDL3_mixer/SDL_mixer.h>
#include "Animation.h"

class RenderBackend;

class Assets
{
	std::map<std::string, Animation> m_animations;
	std::map<std::string, MIX_Audio*> m_audios;
	std::map<std::string, MIX_Audio*> m_music;
	MIX_Mixer* m_mixer = nullptr;

	bool ensureMixer();

public:

	Assets();
	~Assets();

	void addAnimation(const std::string& name, Animation animation);
	void addAudio(const std::string& name, const std::string& path);
	void addMusic(const std::string& name, const std::string& path);

	const Animation& getAnimation(const std::string& name) const;
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
