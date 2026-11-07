#pragma once
#include "imgui.h"
#include "ReferenceColorPicker.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>
#include <sstream>

// UI only. No platform API, game state, process access or renderer hooks.
namespace getrekt {
struct Config {
    ImVec4 accent{1.f,.48f,.045f,1.f};
    ImVec4 windowBg{.085f,.085f,.085f,1.f};
    ImVec4 panelBg{.17f,.17f,.17f,1.f};
    ImVec4 text{.84f,.84f,.84f,1.f};
    std::string caption="GetRekt Club",title="Your Project";
    static constexpr bool cutoutHeader=true;
    float windowOpacity=.90f; // Host compositing; 1 = fully opaque.
    float hoverSeconds=.06f;
    static constexpr float tabFadeSeconds=.12f;
};
enum class ChangeKind { Added, Fixed, Updated, Changed };
struct Change {ChangeKind kind;std::string text;};
struct Release {std::string version;std::vector<Change> changes;};
inline const char* ChangeLabel(ChangeKind kind){switch(kind){case ChangeKind::Added:return "ADD";case ChangeKind::Fixed:return "FIX";case ChangeKind::Updated:return "UPDATE";default:return "CHANGED";}}
inline ImVec4 ChangeColor(ChangeKind kind){switch(kind){case ChangeKind::Added:return ImVec4(.065f,.48f,.075f,1);case ChangeKind::Fixed:return ImVec4(0,.40f,.96f,1);case ChangeKind::Updated:return ImVec4(.62f,.16f,.85f,1);default:return ImVec4(1,.46f,.19f,1);}}
class Theme {
    std::unordered_map<ImGuiID,float> hover;
    int lastTab=-1;
    bool savePending=false,changelogRequested=false;
    std::chrono::steady_clock::time_point saveAfter{};
    std::string changelogProject="Your Project",clubTitle="GETREKT MEMBERS' CLUB";
    std::vector<Release> changelog;
    float content=1.f;
    static ImU32 Color(ImVec4 c){c.w*=ImGui::GetStyle().Alpha;return ImGui::ColorConvertFloat4ToU32(c);}
    float Hover(bool on){
        float& value=hover[ImGui::GetID("##hover")];
        if(config.hoverSeconds<=0)value=on?1.f:0.f;
        else value+=(float(on)-value)*(1.f-std::exp(-ImGui::GetIO().DeltaTime*4.f/config.hoverSeconds));
        return value;
    }
public:
    Config config;
    void Apply(){
        auto& s=ImGui::GetStyle();s.WindowRounding=s.ChildRounding=s.FrameRounding=s.ScrollbarRounding=s.GrabRounding=0;
        s.WindowPadding=ImVec2(12,10);s.FramePadding=ImVec2(4,2);s.ItemSpacing=ImVec2(6,5);s.ScrollbarSize=6;s.ScrollbarPadding=0;s.GrabMinSize=8;s.WindowBorderSize=0;
        bool light=config.windowBg.x+config.windowBg.y+config.windowBg.z>1.5f;
        auto c=s.Colors;c[ImGuiCol_WindowBg]=config.windowBg;c[ImGuiCol_ChildBg]=config.panelBg;
        c[ImGuiCol_Text]=config.text;c[ImGuiCol_Border]=light?ImVec4(.55f,.55f,.55f,1):ImVec4(.07f,.07f,.07f,1);
        c[ImGuiCol_FrameBg]=light?ImVec4(.76f,.76f,.76f,1):ImVec4(.045f,.045f,.045f,1);c[ImGuiCol_FrameBgHovered]=light?ImVec4(.68f,.68f,.68f,1):ImVec4(.12f,.12f,.12f,1);c[ImGuiCol_FrameBgActive]=light?ImVec4(.62f,.62f,.62f,1):ImVec4(.15f,.15f,.15f,1);
        c[ImGuiCol_CheckMark]=c[ImGuiCol_SliderGrab]=c[ImGuiCol_SliderGrabActive]=config.accent;
        c[ImGuiCol_Button]=light?ImVec4(.82f,.82f,.82f,1):ImVec4(.13f,.13f,.13f,1);c[ImGuiCol_ButtonHovered]=light?ImVec4(.70f,.70f,.70f,1):ImVec4(.23f,.23f,.23f,1);c[ImGuiCol_ButtonActive]=config.accent;
        c[ImGuiCol_TitleBg]=config.windowBg;c[ImGuiCol_TitleBgActive]=config.accent;c[ImGuiCol_TitleBgCollapsed]=config.windowBg;
        c[ImGuiCol_PopupBg]=config.panelBg;c[ImGuiCol_Header]=config.accent;c[ImGuiCol_ScrollbarGrab]=ImVec4(.42f,.42f,.42f,1);
    }
    void Dark(){config.windowBg=ImVec4(.085f,.085f,.085f,1);config.panelBg=ImVec4(.17f,.17f,.17f,1);config.text=ImVec4(.84f,.84f,.84f,1);Apply();}
    void Light(){config.windowBg=ImVec4(.75f,.75f,.75f,1);config.panelBg=ImVec4(.94f,.94f,.94f,1);config.text=ImVec4(.08f,.08f,.08f,1);Apply();}
    int CaptionWidth()const{return int(std::ceil((std::min)(300.f,ImGui::CalcTextSize(config.caption.c_str()).x+10)));}
    bool ColorRow(const char* label,ImVec4& color){
        bool changed=false;ImGui::PushID(label);
        float width=ImGui::GetContentRegionAvail().x;ImGui::TextUnformatted(label);ImGui::SameLine(width-28);
        auto p=ImGui::GetCursorScreenPos();if(ImGui::InvisibleButton("swatch",ImVec2(28,14)))ImGui::OpenPopup("picker");
        auto draw=ImGui::GetWindowDrawList();draw->AddRectFilled(p,ImVec2(p.x+28,p.y+10),Color(ImVec4(color.x,color.y,color.z,1)));
        draw->AddRect(p,ImVec2(p.x+28,p.y+10),Color(ImVec4(.42f,.42f,.42f,1)));
        ImGui::SetNextWindowSize(ImVec2(260,326),ImGuiCond_Always);
        if(ImGui::BeginPopup("picker")){
            ImGui::TextUnformatted(label);changed|=ReferenceColorPicker("SV",&color.x);
            int rgb[3]={int(color.x*255+.5f),int(color.y*255+.5f),int(color.z*255+.5f)};
            ImGui::PushItemWidth(56);bool typed=false;
            typed|=ImGui::InputInt("R",&rgb[0],0);ImGui::SameLine();typed|=ImGui::InputInt("G",&rgb[1],0);ImGui::SameLine();typed|=ImGui::InputInt("B",&rgb[2],0);
            ImGui::PopItemWidth();if(typed){color.x=std::clamp(rgb[0],0,255)/255.f;color.y=std::clamp(rgb[1],0,255)/255.f;color.z=std::clamp(rgb[2],0,255)/255.f;changed=true;}
            int alpha=int(color.w*100+.5f);if(SliderInt("Alpha %",&alpha,0,100,"%d%%")){color.w=alpha/100.f;changed=true;}
            ImGui::EndPopup();
        }
        ImGui::PopID();return changed;
    }
    void SetChangelog(std::string project,std::vector<Release> releases,std::string heading="GETREKT MEMBERS' CLUB"){
        changelogProject=std::move(project);changelog=std::move(releases);clubTitle=std::move(heading);
    }
    // Legacy text callers keep working; projects can migrate to explicitly typed rows.
    void SetChangelog(std::string project,std::string entries){
        std::vector<Release> releases;std::istringstream input(entries);std::string line;
        while(std::getline(input,line)){
            if(line.empty())continue;
            if(line[0]!='-'){releases.push_back({line,{}});continue;}
            if(releases.empty())releases.push_back({"",{}});
            releases.back().changes.push_back({ChangeKind::Changed,line.size()>1?line.substr(2):line});
        }
        SetChangelog(std::move(project),std::move(releases));
    }
    void RequestAutoSave(){savePending=true;saveAfter=std::chrono::steady_clock::now()+std::chrono::milliseconds(300);}
    bool FlushAutoSave(const std::filesystem::path& path){if(!savePending)return true;if(!SaveTheme(path))return false;savePending=false;return true;}
    bool TickAutoSave(const std::filesystem::path& path){
        if(!savePending||std::chrono::steady_clock::now()<saveAfter)return true;
        if(FlushAutoSave(path))return true;
        saveAfter=std::chrono::steady_clock::now()+std::chrono::seconds(2);return false;
    }
    bool DrawColors(){
        bool changed=false;Section("Appearance");
        if(ImGui::Button("Set light mode",ImVec2(152,23))){Light();changed=true;}ImGui::SameLine();
        if(ImGui::Button("Set dark mode",ImVec2(152,23))){Dark();changed=true;}
        changed|=ColorRow("Theme color",config.accent);changed|=ColorRow("BG color",config.windowBg);
        changed|=ColorRow("BG child color",config.panelBg);changed|=ColorRow("Text color",config.text);
        if(changed){Apply();RequestAutoSave();}return changed;
    }
    bool DrawOpacity(){
        Section("Opacity");int opacity=int(config.windowOpacity*100+.5f);
        if(SliderInt("Opacity %",&opacity,65,100,"%d%%")){config.windowOpacity=opacity/100.f;RequestAutoSave();return true;}return false;
    }
    bool DrawSettings(){bool a=DrawColors();bool b=DrawOpacity();return a||b;}
    void DrawChangelog(){
        if(changelogRequested){ImGui::OpenPopup("Changelog");changelogRequested=false;}
        auto display=ImGui::GetIO().DisplaySize;
        ImVec2 size((std::min)(680.f,(std::max)(280.f,display.x-24)),(std::min)(600.f,(std::max)(260.f,display.y*.75f)));
        ImGui::SetNextWindowSize(size,ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2((std::max)(12.f,display.x-size.x-20),(std::max)(20.f,display.y-size.y-20)),ImGuiCond_Appearing);
        ImGui::PushStyleColor(ImGuiCol_PopupBg,ImVec4(0,0,0,0));
        if(ImGui::BeginPopupModal("Changelog",nullptr,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoScrollbar)){
            auto origin=ImGui::GetWindowPos();auto draw=ImGui::GetWindowDrawList();
            draw->PushClipRect(origin,ImVec2(origin.x+size.x,origin.y+size.y),false);
            auto banner=config.accent;banner.w=.62f;
            draw->AddRectFilled(origin,ImVec2(origin.x+size.x,origin.y+52),Color(banner));
            auto heading=ImGui::CalcTextSize(clubTitle.c_str());draw->AddText(ImVec2(origin.x+(size.x-heading.x)/2,origin.y+20),Color(ImVec4(1,1,1,1)),clubTitle.c_str());
            draw->AddRectFilled(ImVec2(origin.x,origin.y+56),ImVec2(origin.x+size.x,origin.y+size.y),Color(config.windowBg));
            draw->AddRect(ImVec2(origin.x+2,origin.y+57),ImVec2(origin.x+size.x-2,origin.y+size.y-2),Color(ImVec4(.24f,.24f,.24f,1)));
            draw->PopClipRect();
            ImGui::SetCursorPos(ImVec2(28,78));
            ImGui::BeginChild("club-panel",ImVec2(size.x-56,size.y-102),false,ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
            auto panelPos=ImGui::GetWindowPos();auto panelSize=ImGui::GetWindowSize();
            ImGui::TextUnformatted(changelogProject.c_str());
            Section("Change log:");
            ImGui::PushStyleColor(ImGuiCol_ChildBg,config.windowBg);
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,ImVec2(0,1));
            ImGui::BeginChild("release-notes",ImVec2(0,(std::max)(70.f,panelSize.y-100)),true,ImGuiWindowFlags_AlwaysVerticalScrollbar);
            const float available=ImGui::GetContentRegionAvail().x;
            const float versionWidth=available>450?94.f:78.f,badgeWidth=76.f,gap=6.f;
            const float textWidth=(std::max)(35.f,available-versionWidth-badgeWidth-gap-4);
            for(const auto& release:changelog){bool first=true;
                for(const auto& change:release.changes){
                    auto pos=ImGui::GetCursorScreenPos();auto row=ImGui::GetWindowDrawList();
                    auto bounds=ImGui::CalcTextSize(change.text.c_str(),nullptr,false,textWidth);
                    float height=(std::max)(20.f,bounds.y+4);
                    if(first){std::string version="Version "+release.version;auto versionSize=ImGui::CalcTextSize(version.c_str(),nullptr,false,versionWidth-6);height=(std::max)(height,versionSize.y+4);row->AddText(ImGui::GetFont(),ImGui::GetFontSize(),pos,Color(config.text),version.c_str(),nullptr,versionWidth-6);first=false;}
                    ImVec2 badge(pos.x+versionWidth,pos.y);auto color=ChangeColor(change.kind);
                    row->AddRectFilled(badge,ImVec2(badge.x+badgeWidth,badge.y+20),Color(color));
                    row->AddRect(badge,ImVec2(badge.x+badgeWidth,badge.y+20),Color(ImVec4(0,0,0,.45f)));
                    const char* label=ChangeLabel(change.kind);auto labelSize=ImGui::CalcTextSize(label);
                    row->AddText(ImVec2(badge.x+(badgeWidth-labelSize.x)/2,badge.y+3),Color(ImVec4(1,1,1,1)),label);
                    row->AddText(ImGui::GetFont(),ImGui::GetFontSize(),ImVec2(badge.x+badgeWidth+gap,pos.y+2),Color(config.text),change.text.c_str(),nullptr,textWidth);
                    ImGui::Dummy(ImVec2(available,height));
                }
            }
            if(changelog.empty())ImGui::TextUnformatted("No release notes supplied.");
            ImGui::EndChild();ImGui::PopStyleVar();ImGui::PopStyleColor();
            if(ImGui::Button("Close",ImVec2(-1,24)))ImGui::CloseCurrentPopup();
            ImGui::EndChild();ImGui::EndPopup();
        }
        ImGui::PopStyleColor();
    }
    bool SaveTheme(const std::filesystem::path& path)const{
        std::ofstream out(path,std::ios::binary|std::ios::trunc);if(!out)return false;
        out<<std::setprecision(9)<<"GetRektTheme 1\n";
        auto color=[&](const char* name,const ImVec4& c){out<<name<<' '<<c.x<<' '<<c.y<<' '<<c.z<<' '<<c.w<<'\n';};
        color("accent",config.accent);color("window",config.windowBg);color("panel",config.panelBg);color("text",config.text);
        out<<"opacity "<<config.windowOpacity<<"\nhover "<<config.hoverSeconds<<'\n';
        out.flush();return bool(out);
    }
    bool LoadTheme(const std::filesystem::path& path){
        std::ifstream in(path,std::ios::binary);std::string magic;int version=0;
        if(!(in>>magic>>version)||magic!="GetRektTheme"||version!=1)return false;
        Config next=config;std::string key;
        while(in>>key){
            ImVec4* c=key=="accent"?&next.accent:key=="window"?&next.windowBg:key=="panel"?&next.panelBg:key=="text"?&next.text:nullptr;
            if(c){if(!(in>>c->x>>c->y>>c->z>>c->w))return false;}
            else if(key=="opacity"){if(!(in>>next.windowOpacity))return false;}
            else if(key=="hover"){if(!(in>>next.hoverSeconds))return false;}
            else if(key=="fade"){float ignored;if(!(in>>ignored))return false;}
            else if(key=="cutout"){int value;if(!(in>>value)||(value!=0&&value!=1))return false;/* Legacy key accepted, fixed header remains enabled. */}
            else if(key=="caption"){std::string ignored;if(!(in>>std::quoted(ignored)))return false;}
            else if(key=="title"){std::string ignored;if(!(in>>std::quoted(ignored)))return false;}
            else return false;
        }
        auto valid=[](float x,float lo,float hi){return std::isfinite(x)&&x>=lo&&x<=hi;};
        for(auto c:{next.accent,next.windowBg,next.panelBg,next.text})if(!valid(c.x,0,1)||!valid(c.y,0,1)||!valid(c.z,0,1)||!valid(c.w,0,1))return false;
        if(!valid(next.windowOpacity,.65f,1)||!valid(next.hoverSeconds,0,1)||!valid(next.tabFadeSeconds,0,1)||next.caption.size()>127||next.title.size()>127)return false;
        config=next;savePending=false;Apply();return true;
    }
    bool BeginMenu(const char* id,ImGuiWindowFlags flags){
        // Keep the original transparent cutout header.
        ImGui::SetNextWindowBgAlpha(0.f);return ImGui::Begin(id,nullptr,flags);
    }
    void Header(float width=720){Header(config.caption.c_str(),config.title.c_str(),width);}
    void Header(const char* caption,const char* title,float width){
        auto origin=ImGui::GetWindowPos();auto draw=ImGui::GetWindowDrawList();
        width=(std::min)(width,ImGui::GetWindowWidth());
        const ImVec2 end(origin.x+width,origin.y+ImGui::GetWindowHeight());
        // Chrome belongs to the whole window, not the padded content clip rectangle.
        draw->PushClipRect(origin,end,false);
        draw->AddRectFilled(ImVec2(origin.x,origin.y+(config.cutoutHeader?16.f:0.f)),end,Color(config.windowBg));
        auto a=config.accent;a.w=.82f;auto b=a;b.w=.46f;
        float capWidth=std::ceil((std::min)(300.f,ImGui::CalcTextSize(caption).x+10));
        draw->AddRectFilled(origin,ImVec2(origin.x+capWidth,origin.y+16),Color(config.accent));
        draw->AddText(ImVec2(origin.x+5,origin.y+1),IM_COL32(255,255,255,255),caption);
        draw->AddRectFilledMultiColor(ImVec2(origin.x,origin.y+16),ImVec2(origin.x+width,origin.y+58),Color(a),Color(b),Color(b),Color(a));
        auto size=ImGui::CalcTextSize(title);draw->AddText(ImVec2(origin.x+(width-size.x)/2,origin.y+30),IM_COL32(255,255,255,255),title);
        draw->AddRectFilled(ImVec2(origin.x,origin.y+58),ImVec2(origin.x+width,origin.y+92),Color(ImVec4(config.windowBg.x+.06f,config.windowBg.y+.06f,config.windowBg.z+.06f,1)));
        ImGui::PushClipRect(origin,ImVec2(origin.x+capWidth,origin.y+16),false);
        ImGui::SetCursorScreenPos(origin);
        if(ImGui::InvisibleButton("##caption-changelog",ImVec2(capWidth,16)))changelogRequested=true;
        ImGui::PopClipRect();draw->PopClipRect();
        ImGui::SetCursorPos(ImVec2(12,60));
    }
    bool Tab(const char* label,bool selected,float width){
        ImGui::PushID(label);auto pos=ImGui::GetCursorScreenPos();bool clicked=ImGui::InvisibleButton("tab",ImVec2(width,30));
        float h=Hover(ImGui::IsItemHovered());auto draw=ImGui::GetWindowDrawList();
        if(h>.001f)draw->AddRectFilled(pos,ImVec2(pos.x+width,pos.y+30),IM_COL32(64,64,64,int(h*180)));
        auto size=ImGui::CalcTextSize(label);auto c=selected?config.accent:config.text;
        draw->AddText(ImVec2(pos.x+(width-size.x)/2,pos.y+8),Color(c),label);
        if(selected)draw->AddLine(ImVec2(pos.x,pos.y+30),ImVec2(pos.x+width,pos.y+30),Color(config.accent),1);
        ImGui::PopID();return clicked;
    }
    void Section(const char* label){
        auto p=ImGui::GetCursorScreenPos();auto draw=ImGui::GetWindowDrawList();auto size=ImGui::CalcTextSize(label);
        auto color=config.accent;color.w=.55f;
        draw->AddLine(ImVec2(p.x,p.y+7),ImVec2(p.x+18,p.y+7),Color(color));
        draw->AddText(ImVec2(p.x+24,p.y),Color(config.accent),label);
        float end=ImGui::GetWindowPos().x+ImGui::GetWindowWidth()-12;
        if(p.x+size.x+32<end)draw->AddLine(ImVec2(p.x+size.x+32,p.y+7),ImVec2(end,p.y+7),Color(color));
        ImGui::Dummy(ImVec2(0,19));
    }
    bool Checkbox(const char* label,bool* value){
        ImGui::PushID(label);auto p=ImGui::GetCursorScreenPos();bool pressed=ImGui::InvisibleButton("check",ImVec2(ImGui::GetContentRegionAvail().x,18));
        if(pressed)*value=!*value;float h=Hover(ImGui::IsItemHovered());auto draw=ImGui::GetWindowDrawList();
        draw->AddRectFilled(ImVec2(p.x+1,p.y+2),ImVec2(p.x+13,p.y+14),ImGui::GetColorU32(ImGuiCol_FrameBg));
        if(*value){auto c=config.accent;draw->AddRectFilled(ImVec2(p.x+3,p.y+4),ImVec2(p.x+11,p.y+12),Color(c));}
        draw->AddRect(ImVec2(p.x+1,p.y+2),ImVec2(p.x+13,p.y+14),Color(ImVec4(.333f,.333f,.333f,(100+155*h)/255.f)));
        draw->AddText(ImVec2(p.x+21,p.y+1),Color(config.text),label);
        ImGui::PopID();return pressed;
    }
    bool SliderInt(const char* label,int* value,int min,int max,const char* format="%dx"){
        ImGui::PushID(label);float width=ImGui::GetContentRegionAvail().x;ImGui::PushItemWidth(width*.64f);
        bool changed=ImGui::SliderInt("##value",value,min,max,format);ImGui::PopItemWidth();
        ImGui::SameLine();ImGui::TextUnformatted(label);ImGui::PopID();return changed;
    }
    void BeginContent(int selected){
        if(lastTab!=selected){lastTab=selected;content=config.tabFadeSeconds>0?.15f:1.f;}
        if(config.tabFadeSeconds<=0)content=1;
        else content=std::min(1.f,content+ImGui::GetIO().DeltaTime/config.tabFadeSeconds);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha,ImGui::GetStyle().Alpha*content);
    }
    void EndContent(){ImGui::PopStyleVar();}
};
}


