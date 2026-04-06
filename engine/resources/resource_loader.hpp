#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <string_view>

#include "resource.hpp"

namespace resources
{

    class LoadDebugSink
    {
    public:
        virtual ~LoadDebugSink() = default;
        virtual void BeginStage(std::string_view stage, float progress) = 0;
        virtual void EndStage(std::string_view stage, double elapsedMs, float progress) = 0;
    };

    void SetActiveLoadDebugSink(LoadDebugSink *sink) noexcept;
    LoadDebugSink *GetActiveLoadDebugSink() noexcept;

    class LoadDebugStageScope
    {
    public:
        LoadDebugStageScope(std::string_view stage, float progress) : m_sink(GetActiveLoadDebugSink()), m_stage(stage), m_progress(progress), m_startedAt(std::chrono::steady_clock::now())
        {
            if (m_sink)
            {
                m_sink->BeginStage(m_stage, m_progress);
            }
        }

        ~LoadDebugStageScope()
        {
            if (!m_sink)
            {
                return;
            }

            const double elapsedMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - m_startedAt).count();
            m_sink->EndStage(m_stage, elapsedMs, m_progress);
        }

    private:
        LoadDebugSink *m_sink = nullptr;
        std::string_view m_stage;
        float m_progress = 0.0f;
        std::chrono::steady_clock::time_point m_startedAt = {};
    };

    /**
     * @template ResourceLoader
     * @brief Base template for type-specific resource loaders
     *
     * Specializations of this template provide the loading logic for specific resource types.
     *
     * @tparam T The resource type being loaded
     */
    template <typename T>
    struct ResourceLoader
    {
        /**
         * @brief Load a resource from disk/memory
         * @param path The path/identifier of the resource
         * @return A unique_ptr to the newly created resource
         */
        static std::unique_ptr<T> Load(const std::string &path);
    };
}
