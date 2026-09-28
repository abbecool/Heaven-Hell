#pragma once

#include "external/json.hpp"

#include <filesystem>

class GameSave
{
public:
    explicit GameSave(std::filesystem::path path = defaultPath());

    static std::filesystem::path defaultPath();
    bool exists() const;
    nlohmann::json load() const;
    void write(const nlohmann::json& snapshot) const;

private:
    std::filesystem::path m_path;
};
