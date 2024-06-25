#pragma once

#include "Animation.h"
#include "SFML/Graphics/Font.hpp"
#include "SFML/Graphics/Texture.hpp"
#include <map>
#include <string>


typedef std::map<std::string, sf::Texture> TextureMap;
typedef std::map<std::string, Animation>   AnimationMap;
typedef std::map<std::string, sf::Font>    FontMap;


class Assets
{
	TextureMap   m_textures;
	AnimationMap m_animations;
	FontMap      m_fonts;

public:

	Assets();

	void addTexture(const std::string& name, const std::string& path);
	void addAnimation(const std::string& name, const Animation& animation);
	void addFont(const std::string& name, const std::string& path);

	const sf::Texture& getTexture(const std::string& name) const;
	const Animation& getAnimation(const std::string& name) const;
	const sf::Font& getfont(const std::string& name) const;

	void loadFromFile(const std::string& path);
};