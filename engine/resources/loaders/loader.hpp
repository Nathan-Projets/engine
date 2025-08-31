#pragma once

#include <unordered_map>
#include <functional>
#include <memory>
#include <string>
#include <stdexcept>
#include <print>

#include "iloader.hpp"

class Loader
{
public:
    using LoaderFactory = std::function<std::unique_ptr<ILoader>()>;
    using Registry = std::unordered_map<std::string, LoaderFactory>;

    static std::vector<Mesh> Load(const std::string &filepath)
    {
        auto extension = GetExtension(filepath);
        auto loader = CreateLoader(extension);
        return loader->Load(filepath);
    }

    static void RegisterLoader(const std::string &extension, LoaderFactory factory, bool forceRegistering = false)
    {
        Registry &registry = GetRegistry();
        if (registry.find(extension) == registry.end() || forceRegistering)
        {
            registry[extension] = factory;
        }
    }

private:
    static std::unique_ptr<ILoader> CreateLoader(const std::string &extension)
    {
        Registry &registry = GetRegistry();
        auto loader = registry.find(extension);
        if (loader == registry.end())
        {
            throw std::runtime_error("No loader registered for extension: " + extension);
        }
        return loader->second();
    }

    static std::string GetExtension(const std::string &filepath)
    {
        size_t position = filepath.find_last_of('.');
        if (position == std::string::npos)
        {
            return "";
        }
        return filepath.substr(position + 1);
    }

    static Registry &GetRegistry()
    {
        static Registry registry;
        return registry;
    }
};