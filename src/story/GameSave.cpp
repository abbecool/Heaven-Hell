#include "story/GameSave.hpp"

#include <cstdlib>
#include <fstream>
#include <stdexcept>
#ifdef _WIN32
#include <windows.h>
#endif

using json = nlohmann::json;

GameSave::GameSave(std::filesystem::path path) : m_path(std::move(path)) {}

std::filesystem::path GameSave::defaultPath()
{
#ifdef _WIN32
    const char* root = std::getenv("LOCALAPPDATA");
    if (!root || !*root)
        root = std::getenv("APPDATA");
    if (root && *root)
        return std::filesystem::path(root) / "HeavenHell" / "save.json";
#else
    const char* root = std::getenv("XDG_DATA_HOME");
    if (root && *root)
        return std::filesystem::path(root) / "heavenhell" / "save.json";
    root = std::getenv("HOME");
    if (root && *root)
        return std::filesystem::path(root) / ".local/share/heavenhell/save.json";
#endif
    throw std::runtime_error("Cannot locate a user data directory for saves");
}

bool GameSave::exists() const
{
    return std::filesystem::exists(m_path);
}

json GameSave::load() const
{
    std::ifstream file(m_path);
    if (!file)
        throw std::runtime_error("Could not open saved game: " + m_path.string());
    json snapshot;
    file >> snapshot;
    if (!snapshot.is_object() || snapshot.at("version") != 1 ||
        !snapshot.at("player").is_object() ||
        !snapshot.at("story").is_object() ||
        !snapshot.at("world").is_object())
        throw std::runtime_error("Unsupported saved game format");
    return snapshot;
}

void GameSave::write(const json& snapshot) const
{
    if (!snapshot.is_object() || snapshot.at("version") != 1 ||
        !snapshot.at("player").is_object() ||
        !snapshot.at("story").is_object() ||
        !snapshot.at("world").is_object())
        throw std::runtime_error("Invalid saved game snapshot");
    std::filesystem::create_directories(m_path.parent_path());
    std::filesystem::path temporary = m_path;
    temporary += ".tmp";
    {
        std::ofstream file(temporary, std::ios::trunc);
        if (!file)
            throw std::runtime_error("Could not create temporary game save");
        file << snapshot.dump(4) << '\n';
        file.flush();
        if (!file)
            throw std::runtime_error("Could not write temporary game save");
    }
#ifdef _WIN32
    // ReplaceFile preserves the existing save if replacement fails.
    if (std::filesystem::exists(m_path))
    {
        if (!ReplaceFileW(m_path.wstring().c_str(),
                          temporary.wstring().c_str(), nullptr, 0, nullptr,
                          nullptr))
            throw std::runtime_error("Could not replace game save");
        return;
    }
#endif
    std::filesystem::rename(temporary, m_path);
}
