#pragma once

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../units/shader.hpp"
#include "../resource_loader.hpp"

namespace resources
{
    namespace detail
    {
        inline std::string ReadTextFile(const std::string &path)
        {
            std::ifstream file(path);
            if (!file.is_open())
            {
                throw std::runtime_error("Unable to open shader file: " + path);
            }

            std::stringstream buffer;
            buffer << file.rdbuf();
            return buffer.str();
        }

        inline std::string Trim(const std::string &value)
        {
            const std::string::size_type begin = value.find_first_not_of(" \t\r\n");
            if (begin == std::string::npos)
            {
                return "";
            }

            const std::string::size_type end = value.find_last_not_of(" \t\r\n");
            return value.substr(begin, end - begin + 1);
        }

        inline ShaderStage StageFromString(const std::string &stageName)
        {
            std::string normalized = stageName;
            std::transform(
                normalized.begin(),
                normalized.end(),
                normalized.begin(),
                [](unsigned char character)
                {
                    return static_cast<char>(std::tolower(character));
                });

            if (normalized == "vertex" || normalized == "vert" || normalized == "vs")
            {
                return ShaderStage::Vertex;
            }

            if (normalized == "fragment" || normalized == "frag" || normalized == "fs" || normalized == "pixel")
            {
                return ShaderStage::Fragment;
            }

            if (normalized == "geometry" || normalized == "geom" || normalized == "gs")
            {
                return ShaderStage::Geometry;
            }

            if (normalized == "compute" || normalized == "comp" || normalized == "cs")
            {
                return ShaderStage::Compute;
            }

            if (normalized == "tess_control" || normalized == "tesc" || normalized == "hull")
            {
                return ShaderStage::TessellationControl;
            }

            if (normalized == "tess_evaluation" || normalized == "tese" || normalized == "domain")
            {
                return ShaderStage::TessellationEvaluation;
            }

            throw std::runtime_error("Unknown shader stage name: " + stageName);
        }

        inline bool TryStageFromExtension(const std::filesystem::path &path, ShaderStage &outStage)
        {
            std::string extension = path.extension().string();
            std::transform(
                extension.begin(),
                extension.end(),
                extension.begin(),
                [](unsigned char character)
                {
                    return static_cast<char>(std::tolower(character));
                });

            if (extension == ".vert" || extension == ".vs")
            {
                outStage = ShaderStage::Vertex;
                return true;
            }

            if (extension == ".frag" || extension == ".fs")
            {
                outStage = ShaderStage::Fragment;
                return true;
            }

            if (extension == ".geom" || extension == ".gs")
            {
                outStage = ShaderStage::Geometry;
                return true;
            }

            if (extension == ".comp" || extension == ".cs")
            {
                outStage = ShaderStage::Compute;
                return true;
            }

            if (extension == ".tesc")
            {
                outStage = ShaderStage::TessellationControl;
                return true;
            }

            if (extension == ".tese")
            {
                outStage = ShaderStage::TessellationEvaluation;
                return true;
            }

            return false;
        }

        inline bool FileExists(const std::filesystem::path &path)
        {
            std::error_code errorCode;
            return std::filesystem::exists(path, errorCode) && !errorCode;
        }

        inline std::vector<ShaderSource> ParseCombinedShaderFile(const std::string &content, const std::string &path)
        {
            std::vector<ShaderSource> sources;

            const std::string token = "#type";
            std::string::size_type cursor = 0;
            while (true)
            {
                const std::string::size_type tokenPos = content.find(token, cursor);
                if (tokenPos == std::string::npos)
                {
                    break;
                }

                const std::string::size_type lineEnd = content.find_first_of("\r\n", tokenPos);
                if (lineEnd == std::string::npos)
                {
                    throw std::runtime_error("Malformed #type declaration in shader file: " + path);
                }

                const std::string stageName = Trim(content.substr(tokenPos + token.size(), lineEnd - (tokenPos + token.size())));
                const ShaderStage stage = StageFromString(stageName);

                const std::string::size_type bodyStart = content.find_first_not_of("\r\n", lineEnd);
                if (bodyStart == std::string::npos)
                {
                    throw std::runtime_error("Shader stage block is empty in file: " + path);
                }

                const std::string::size_type nextTokenPos = content.find(token, bodyStart);
                const std::string::size_type bodyEnd = (nextTokenPos == std::string::npos) ? content.size() : nextTokenPos;

                sources.push_back({stage, path, content.substr(bodyStart, bodyEnd - bodyStart)});

                cursor = bodyEnd;
            }

            return sources;
        }

        inline void ParseDescriptor(
            const std::string &descriptorPath,
            const std::string &content,
            std::vector<ShaderSource> &outSources,
            std::unordered_map<std::string, std::string> &outDefines,
            std::string &outEntryPoint)
        {
            const std::filesystem::path descriptorDirectory = std::filesystem::path(descriptorPath).parent_path();

            std::stringstream stream(content);
            std::string line;
            while (std::getline(stream, line))
            {
                line = Trim(line);
                if (line.empty() || line.rfind("//", 0) == 0 || line.rfind("#", 0) == 0)
                {
                    continue;
                }

                const std::string::size_type separator = line.find_first_of(":=");
                if (separator == std::string::npos)
                {
                    continue;
                }

                const std::string key = Trim(line.substr(0, separator));
                const std::string value = Trim(line.substr(separator + 1));
                if (key.empty() || value.empty())
                {
                    continue;
                }

                if (key == "entry" || key == "entryPoint")
                {
                    outEntryPoint = value;
                    continue;
                }

                if (key.rfind("define", 0) == 0)
                {
                    const std::string defineKey = Trim(key.substr(6));
                    if (!defineKey.empty())
                    {
                        outDefines[defineKey] = value;
                    }
                    continue;
                }

                ShaderStage stage = ShaderStage::Vertex;
                try
                {
                    stage = StageFromString(key);
                }
                catch (const std::exception &)
                {
                    continue;
                }

                const std::filesystem::path sourcePath = descriptorDirectory / value;
                outSources.push_back({stage, sourcePath.string(), ReadTextFile(sourcePath.string())});
            }
        }
    }

    template <>
    inline std::unique_ptr<Shader> ResourceLoader<Shader>::Load(const std::string &path)
    {
        auto shader = std::make_unique<Shader>(0, path);
        shader->SetState(ResourceState::Loading);

        const std::filesystem::path shaderPath(path);

        // Auto-discovery: If the path has no extension (e.g., "shaders/lit_ubo"),
        // probe for individual shader files (.vert, .frag, .geom, .comp) in the same directory.
        // This simplifies the scene format and asset organization.
        // Example: path="shaders/proto/lit_ubo" will find and load:
        //   - shaders/proto/lit_ubo.vert (if exists) → Vertex stage
        //   - shaders/proto/lit_ubo.frag (if exists) → Fragment stage
        //   - shaders/proto/lit_ubo.geom (if exists) → Geometry stage
        //   - shaders/proto/lit_ubo.comp (if exists) → Compute stage
        if (!shaderPath.has_extension())
        {
            std::vector<ShaderSource> discoveredSources;

            const std::filesystem::path vertexPath = shaderPath.string() + ".vert";
            if (detail::FileExists(vertexPath))
            {
                discoveredSources.push_back({ShaderStage::Vertex, vertexPath.string(), detail::ReadTextFile(vertexPath.string())});
            }

            const std::filesystem::path fragmentPath = shaderPath.string() + ".frag";
            if (detail::FileExists(fragmentPath))
            {
                discoveredSources.push_back({ShaderStage::Fragment, fragmentPath.string(), detail::ReadTextFile(fragmentPath.string())});
            }

            const std::filesystem::path geometryPath = shaderPath.string() + ".geom";
            if (detail::FileExists(geometryPath))
            {
                discoveredSources.push_back({ShaderStage::Geometry, geometryPath.string(), detail::ReadTextFile(geometryPath.string())});
            }

            const std::filesystem::path computePath = shaderPath.string() + ".comp";
            if (detail::FileExists(computePath))
            {
                discoveredSources.push_back({ShaderStage::Compute, computePath.string(), detail::ReadTextFile(computePath.string())});
            }

            if (!discoveredSources.empty())
            {
                shader->SetSources(std::move(discoveredSources));
                return shader;
            }
        }

        ShaderStage extensionStage = ShaderStage::Vertex;
        if (detail::TryStageFromExtension(shaderPath, extensionStage))
        {
            shader->AddSource(extensionStage, path, detail::ReadTextFile(path));
            return shader;
        }

        const std::string fileContent = detail::ReadTextFile(path);

        std::vector<ShaderSource> sources = detail::ParseCombinedShaderFile(fileContent, path);
        if (!sources.empty())
        {
            shader->SetSources(std::move(sources));
            return shader;
        }

        std::unordered_map<std::string, std::string> defines;
        std::string entryPoint = "main";
        detail::ParseDescriptor(path, fileContent, sources, defines, entryPoint);

        if (sources.empty())
        {
            throw std::runtime_error("Shader descriptor does not define any stage source: " + path);
        }

        shader->SetSources(std::move(sources));
        shader->SetEntryPoint(std::move(entryPoint));
        for (const auto &[name, value] : defines)
        {
            shader->SetDefine(name, value);
        }

        return shader;
    }
}