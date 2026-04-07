#pragma once

#include <array>

#include "../../debug/debug_ui.hpp"
#include "../editor_context.hpp"

class EditorInspectorPanel : public IDebugPanel
{
public:
    void SetContext(EditorContext *context);
    void Draw() override;

private:
    void SyncNameBuffer(const EditorEntityInspectorState &state);

private:
    EditorContext *m_context = nullptr;
    Entity::ID m_nameBufferEntityId = Entity::INVALID_ID;
    std::array<char, 256> m_nameBuffer{};
};