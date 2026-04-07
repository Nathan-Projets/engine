#pragma once

#include "../../debug/debug_ui.hpp"
#include "../editor_context.hpp"

class EditorOutlinerPanel : public IDebugPanel
{
public:
    void SetContext(EditorContext *context);
    void Draw() override;

private:
    EditorContext *m_context = nullptr;
};