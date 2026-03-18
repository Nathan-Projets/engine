#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "../resource.hpp"

namespace resources
{
    enum class ShaderStage
    {
        Vertex,                     ///< Vertex processing stage
        Fragment,                   ///< Fragment/pixel processing stage
        Geometry,                   ///< Geometry processing stage
        Compute,                    ///< Compute shader
        TessellationControl,        ///< Tessellation control (hull) shader
        TessellationEvaluation      ///< Tessellation evaluation (domain) shader
    };

    /// @struct ShaderSource
    /// @brief Source code for a single shader stage
    struct ShaderSource
    {
        ShaderStage stage = ShaderStage::Vertex;
        std::string sourcePath;     ///< File path where this source came from
        std::string code;           ///< The actual shader source code
    };

    /**
     * @class Shader
     * @brief Represents a compiled shader program resource
     * @details
     * A Shader contains source code for one or more shader stages (vertex, fragment, etc).
     * Sources are compiled separately per stage and linked into a program.
     *
     * Shaders are loaded from:
     * - Individual files (auto-discovery: "path/to/shader" finds shader.vert + shader.frag)
     * - Combined files with #type directives
     * - Descriptor files with metadata (entry points, defines)
     *
     * @see ResourceLoader<Shader>::Load for loading strategies
     */
    class Shader : public Resource
    {
    public:
        Shader(uint32_t id, const std::string &path = "") : Resource(id, path) {}

        /// @brief Add source code for a shader stage
        void AddSource(ShaderStage stage, const std::string &sourcePath, std::string code)
        {
            m_sources.push_back({stage, sourcePath, std::move(code)});
        }

        /// @brief Set all sources at once (used after loading)
        void SetSources(std::vector<ShaderSource> sources)
        {
            m_sources = std::move(sources);
        }

        /// @brief Get all shader sources
        const std::vector<ShaderSource> &GetSources() const noexcept { return m_sources; }

        /// @brief Check if a shader stage is present in this shader program
        bool HasStage(ShaderStage stage) const noexcept
        {
            for (const ShaderSource &source : m_sources)
            {
                if (source.stage == stage)
                {
                    return true;
                }
            }

            return false;
        }

        /// @brief Set the entry point function name (default: "main")
        void SetEntryPoint(std::string entryPoint)
        {
            m_entryPoint = std::move(entryPoint);
        }

        /// @brief Get the entry point function name
        const std::string &GetEntryPoint() const noexcept
        {
            return m_entryPoint;
        }

        /// @brief Define a preprocessor symbol
        void SetDefine(const std::string &name, const std::string &value)
        {
            m_defines[name] = value;
        }

        /// @brief Get all preprocessor defines
        const std::unordered_map<std::string, std::string> &GetDefines() const noexcept
        {
            return m_defines;
        }

        bool IsComputeOnly() const noexcept
        {
            return HasStage(ShaderStage::Compute) && m_sources.size() == 1;
        }

        void SetProgramId(uint32_t programId) noexcept
        {
            m_programId = programId;
        }

        uint32_t GetProgramId() const noexcept
        {
            return m_programId;
        }

        bool IsGpuReady() const noexcept
        {
            return m_programId != 0;
        }

    private:
        std::vector<ShaderSource> m_sources;
        std::unordered_map<std::string, std::string> m_defines;
        std::string m_entryPoint = "main";
        uint32_t m_programId = 0;
    };
}