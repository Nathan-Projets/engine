#pragma once

#include "../../debug/debug_ui.hpp"
#include "../editor_context.hpp"

class EditorFilesPanel : public IDebugPanel
{
public:
    void SetContext(EditorContext *context);
    void Draw() override;

private:
    EditorContext *m_context = nullptr;
};