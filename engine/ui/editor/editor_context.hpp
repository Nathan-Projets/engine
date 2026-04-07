#pragma once

#include "../../core/scene.hpp"

class EditorContext
{
public:
    void BindScene(Scene *scene)
    {
        if (m_scene != scene)
        {
            m_scene = scene;
            m_selectedEntity = Entity();
            m_dirty = false;
        }
    }

    Scene *GetScene() const
    {
        return m_scene;
    }

    void SelectEntity(Entity entity)
    {
        m_selectedEntity = entity;
    }

    void ClearSelection()
    {
        m_selectedEntity = Entity();
    }

    Entity GetSelectedEntity() const
    {
        return m_selectedEntity;
    }

    bool HasSelection() const
    {
        return m_selectedEntity.IsValid();
    }

    void MarkDirty()
    {
        m_dirty = true;
    }

    void ClearDirty()
    {
        m_dirty = false;
    }

    bool IsDirty() const
    {
        return m_dirty;
    }

    void SetViewportHovered(bool hovered)
    {
        m_viewportHovered = hovered;
    }

    void SetViewportFocused(bool focused)
    {
        m_viewportFocused = focused;
    }

    bool IsViewportHovered() const
    {
        return m_viewportHovered;
    }

    bool IsViewportFocused() const
    {
        return m_viewportFocused;
    }

private:
    Scene *m_scene = nullptr;
    Entity m_selectedEntity;
    bool m_dirty = false;
    bool m_viewportHovered = false;
    bool m_viewportFocused = false;
};