#pragma once

#include "../debug_ui.hpp"
#include "../../../resources/resource_manager.hpp"

class DebugLoadingPanel : public IDebugPanel
{
public:
    void SetResourceManager(resources::ResourceManager *resourceManager);
    void Draw() override;

private:
    resources::ResourceManager *m_resourceManager = nullptr;
};