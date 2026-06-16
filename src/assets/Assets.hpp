#pragma once

#include <map>
#include <string>
#include "SpriteDefinition.hpp"

class RenderBackend;
class SDLPlatform;

class Assets
{
	std::map<std::string, SpriteDefinition> m_sprites;

public:

	Assets();
	~Assets();

	void addSprite(const std::string& name, SpriteDefinition sprite);

	const SpriteDefinition& getSprite(const std::string& name) const;
	void shutdown();

	void loadFromFile(
		const std::string & pathImages, 
		const std::string & pathText, 
		RenderBackend& renderBackend,
		SDLPlatform& platform
	);
};
