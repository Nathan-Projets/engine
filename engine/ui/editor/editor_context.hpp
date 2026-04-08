#pragma once

#include <functional>
#include <string>
#include <vector>

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
        }

        ResetHistory();
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
        if (m_scene)
        {
            m_savedSnapshot = m_scene->ExportEditorSceneSnapshot();
        }
        else
        {
            m_savedSnapshot.clear();
        }
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

    bool CanEdit() const
    {
        return m_scene && !IsPlaying();
    }

    bool IsPlaying() const
    {
        return m_scene && m_scene->IsEditorPlayMode();
    }

    void ResetHistory()
    {
        m_undoStack.clear();
        m_redoStack.clear();
        m_pendingTransaction = {};
        ClearDirty();
        ValidateSelection();
    }

    bool BeginTransaction()
    {
        if (!CanEdit() || m_pendingTransaction.active)
        {
            return false;
        }

        m_pendingTransaction.active = true;
        m_pendingTransaction.snapshot = m_scene->ExportEditorSceneSnapshot();
        m_pendingTransaction.selection = m_selectedEntity;
        return true;
    }

    bool HasPendingTransaction() const
    {
        return m_pendingTransaction.active;
    }

    void CancelTransaction()
    {
        m_pendingTransaction = {};
    }

    bool CommitTransaction(bool changed = true)
    {
        if (!m_pendingTransaction.active)
        {
            return false;
        }

        if (changed)
        {
            PushUndoSnapshot(m_pendingTransaction.snapshot, m_pendingTransaction.selection);
            m_redoStack.clear();
            UpdateDirtyFromScene();
        }

        m_pendingTransaction = {};
        return changed;
    }

    bool ExecuteMutation(const std::function<bool(Scene *)> &mutation)
    {
        if (!CanEdit() || !mutation)
        {
            return false;
        }

        const std::string snapshot = m_scene->ExportEditorSceneSnapshot();
        const Entity selection = m_selectedEntity;
        if (!mutation(m_scene))
        {
            return false;
        }

        PushUndoSnapshot(snapshot, selection);
        m_redoStack.clear();
        UpdateDirtyFromScene();
        ValidateSelection();
        return true;
    }

    bool Undo()
    {
        if (!CanEdit() || m_undoStack.empty())
        {
            return false;
        }

        const SnapshotState current = CaptureCurrentSnapshot();
        const SnapshotState target = m_undoStack.back();
        m_undoStack.pop_back();
        if (!ApplySnapshot(target))
        {
            m_undoStack.push_back(target);
            return false;
        }

        m_redoStack.push_back(current);
        UpdateDirtyFromScene();
        return true;
    }

    bool Redo()
    {
        if (!CanEdit() || m_redoStack.empty())
        {
            return false;
        }

        const SnapshotState current = CaptureCurrentSnapshot();
        const SnapshotState target = m_redoStack.back();
        m_redoStack.pop_back();
        if (!ApplySnapshot(target))
        {
            m_redoStack.push_back(target);
            return false;
        }

        m_undoStack.push_back(current);
        UpdateDirtyFromScene();
        return true;
    }

    bool CanUndo() const
    {
        return CanEdit() && !m_undoStack.empty();
    }

    bool CanRedo() const
    {
        return CanEdit() && !m_redoStack.empty();
    }

    bool StartPlayMode()
    {
        if (!m_scene || IsPlaying())
        {
            return false;
        }

        return m_scene->StartEditorPlayMode();
    }

    bool StopPlayMode()
    {
        if (!IsPlaying())
        {
            return false;
        }

        const bool stopped = m_scene->StopEditorPlayMode();
        if (stopped)
        {
            CancelTransaction();
            ValidateSelection();
            UpdateDirtyFromScene();
        }
        return stopped;
    }

    bool TogglePlayMode()
    {
        return IsPlaying() ? StopPlayMode() : StartPlayMode();
    }

    void ValidateSelection()
    {
        if (!m_scene || !m_selectedEntity.IsValid())
        {
            return;
        }

        if (!m_scene->GetEditorEntityInspectorState(m_selectedEntity).valid)
        {
            m_selectedEntity = Entity();
        }
    }

private:
    struct SnapshotState
    {
        bool active = false;
        std::string snapshot;
        Entity selection;
    };

    SnapshotState CaptureCurrentSnapshot() const
    {
        if (!m_scene)
        {
            return {};
        }

        return SnapshotState{true, m_scene->ExportEditorSceneSnapshot(), m_selectedEntity};
    }

    bool ApplySnapshot(const SnapshotState &snapshot)
    {
        if (!m_scene || !snapshot.active)
        {
            return false;
        }

        if (!m_scene->RestoreEditorSceneSnapshot(snapshot.snapshot))
        {
            return false;
        }

        m_selectedEntity = snapshot.selection;
        ValidateSelection();
        return true;
    }

    void PushUndoSnapshot(const std::string &snapshot, Entity selection)
    {
        m_undoStack.push_back({true, snapshot, selection});
        constexpr size_t maxHistory = 128;
        if (m_undoStack.size() > maxHistory)
        {
            m_undoStack.erase(m_undoStack.begin());
        }
    }

    void UpdateDirtyFromScene()
    {
        if (!m_scene)
        {
            m_dirty = false;
            return;
        }

        m_dirty = m_scene->ExportEditorSceneSnapshot() != m_savedSnapshot;
    }

private:
    Scene *m_scene = nullptr;
    Entity m_selectedEntity;
    bool m_dirty = false;
    bool m_viewportHovered = false;
    bool m_viewportFocused = false;
    std::string m_savedSnapshot;
    SnapshotState m_pendingTransaction;
    std::vector<SnapshotState> m_undoStack;
    std::vector<SnapshotState> m_redoStack;
};