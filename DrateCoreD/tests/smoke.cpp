#ifdef _MSC_VER
#include <crtdbg.h>
#endif
#include <getrekt/GetRektUI.hpp>
#include <iostream>
#include <stdexcept>
void require(bool value,const char* text){if(!value)throw std::runtime_error(text);}
int main(){try{
#ifdef _MSC_VER
    _CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
#endif

    IMGUI_CHECKVERSION();ImGui::CreateContext();auto& io=ImGui::GetIO();io.DisplaySize=ImVec2(1100,760);io.DeltaTime=1.f/60;io.IniFilename=nullptr;io.LogFilename=nullptr;
    unsigned char* pixels;int w,h;io.Fonts->GetTexDataAsRGBA32(&pixels,&w,&h);
    getrekt::UI ui;ui.state.colors.config.text=ImVec4(.3f,.6f,.9f,1);
    const auto style=ImGui::GetStyle();
    for(int page=0;page<6;++page)for(int variant=0;variant<2;++variant){
        ImGui::NewFrame();ui.state.page=page;ui.state.legit=variant;ui.state.visuals=variant;ui.draw();
        require(ImGui::GetStyle().WindowPadding.x==style.WindowPadding.x,"host style changed");
        require(ui.state.colors.config.text.x==.3f&&ui.state.colors.config.text.y==.6f,"Normal overrides text color");ImGui::Render();
        require(ImGui::GetDrawData()!=nullptr,"no draw data");
    }
    ui.state.keybinds["test"]=0;ui.state.checks["test"]=false;ui.state.capturing="test";
    require(ui.keyEvent('C',true)&&ui.state.keybinds["test"]=='C',"assignment failed");ui.keyEvent('C',false);
    ui.setVisible(false);ui.keyEvent('C',true);require(ui.state.checks["test"],"hotkey did not toggle");ui.keyEvent('C',true);require(ui.state.checks["test"],"repeat toggled twice");ui.keyEvent('C',false);
    ui.setFocused(false);ui.keyEvent('C',true);require(ui.state.checks["test"],"unfocused hotkey fired");
    ui.setFocused(true);ui.setVisible(true);ui.state.capturing="test";ui.keyEvent(27,true);require(ui.state.keybinds["test"]==0,"Escape did not clear");
    for(int count=0;count<3;++count){ui.state.spectators=true;ui.state.clubWindow=true;ui.state.spectatorRows.clear();for(int i=0;i<count;++i)ui.state.spectatorRows.push_back({"Demo",i%2!=0,0});ImGui::NewFrame();ui.draw();ImGui::Render();}
    ui.state.clubWindow=false;ui.state.spectators=false;ui.state.page=1;ui.state.legit=0;
    auto frame=[&](){ImGui::NewFrame();ui.draw();ImGui::Render();};
    auto click=[&](float x,float y){io.AddMousePosEvent(x,y);frame();io.AddMouseButtonEvent(0,true);frame();io.AddMouseButtonEvent(0,false);frame();};
    frame();frame();click(180,270);
    require(ui.capturing(),"mouse click on badge did not start capture");
    auto bindingId=ui.state.capturing;const bool before=ui.state.checks[bindingId];
    require(ui.keyEvent(5,true)&&ui.state.keybinds[bindingId]==5,"Mouse 4 capture failed");
    require(ui.state.checks[bindingId]==before,"badge changed checkbox");ui.keyEvent(5,false);
    click(180,270);require(ui.capturing(),"assigned badge not clickable");ui.keyEvent(27,true);require(ui.state.keybinds[bindingId]==0,"clicked badge Escape clear failed");ui.keyEvent(27,false);
    // Exercise the preserved picker, including its integer alpha slider, on the modern API.
    ImGui::NewFrame();ImGui::Begin("picker-test");float rgb[]{.3f,.4f,.5f};getrekt::ReferenceColorPicker("test",rgb);int alpha=50;ui.state.colors.SliderInt("Alpha %",&alpha,0,100,"%d%%");ImGui::End();ImGui::Render();
    ImGui::DestroyContext();std::cout<<"PASS: isolated UI, pages, text color, style restoration, key capture, repeat/focus policy, auxiliary windows\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
