#include "editor_outliner_panel.hpp"

#include <algorithm>
#include <string>
#include <vector>

#include <imgui.h>

void EditorOutlinerPanel::SetContext(EditorContext *context)
{
    m_context = context;
}

void EditorOutlinerPanel::Draw()
{
    ImGui::SetNextWindowSize(ImVec2(320.0f, 420.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Outliner");

    if (!m_context || !m_context->GetScene())
    {
        ImGui::TextUnformatted("No scene bound.");
        ImGui::End();
        return;
    }

    std::vector<EditorEntitySummary> entities = m_context->GetScene()->GetEditorEntitySummaries();
    std::sort(entities.begin(), entities.end(), [](const EditorEntitySummary &left, const EditorEntitySummary &right)
              {
                  if (left.name == right.name)
                  {
                      return left.entity.GetID() < right.entity.GetID();
                  }

                  return left.name < right.name;
              });

    if (entities.empty())
    {
        ImGui::TextUnformatted("Scene has no entities.");
        ImGui::End();
        return;
    }

    ImGui::Text("Entities: %d", static_cast<int>(entities.size()));
    ImGui::Separator();
    ImGui::BeginChild("entity_list", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);

    for (const EditorEntitySummary &entry : entities)
    {
        const bool isSelected = m_context->GetSelectedEntity() == entry.entity;
        const std::string label = entry.name + "##entity_" + std::to_string(entry.entity.GetID());
        if (ImGui::Selectable(label.c_str(), isSelected))
        {
            m_context->SelectEntity(entry.entity);
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Entity %u", entry.entity.GetID());
        }
    }

    ImGui::EndChild();
    ImGui::End();
}