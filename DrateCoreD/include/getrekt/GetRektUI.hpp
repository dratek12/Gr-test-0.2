#pragma once
#include "detail/VisualReplica.hpp"
#if IMGUI_VERSION_NUM < 19300
#error "GetRekt Modern Kit requires Dear ImGui 1.93.0 or newer; validated with 1.93.0."
#endif
// Renderer-independent UI. Call only on the thread owning the current ImGui context.
namespace getrekt {
class UI {
    bool focused_=true, visible_=true, panic_=false;
public:
    VisualReplica state;
    bool visible() const { return visible_; }
    bool capturing() const { return !state.capturing.empty(); }
    void setVisible(bool value) { visible_=value;sync(); }
    void setPanic(bool value) { panic_=value;sync(); }
    void setFocused(bool value) { focused_=value;if(!value)state.CancelBinding();sync(); }
    bool keyEvent(int key,bool down) { sync();return focused_&&state.HandleBindingKey(key,down); }
    void setPreviewTextures(ImTextureID player,ImTextureID knife,ImTextureID glove) {
        state.playerTexture=player;state.knifeTexture=knife;state.gloveTexture=glove;
    }
    void draw() {
        sync();const ImGuiStyle previous=ImGui::GetStyle();
        if(visible_)state.Draw(panic_);
        else if(state.spectators&&!panic_){state.colors.Apply();state.Spectators();}
        ImGui::GetStyle()=previous;
    }
private:
    void sync() {
        state.allowHotkeys=focused_&&!visible_&&!panic_;
        if(!focused_||!visible_||panic_)state.capturing.clear();
    }
};
}
