#include <iostream>

#include <tempo.h>
#include "imgui_stdlib.h"

class MainApp : public Tempo::App {
private:
    Tempo::FontID m_font_regular;
    Tempo::FontID m_font_italic;
    Tempo::FontID m_font_bold;
    Tempo::FontID m_font_emoji;
    //bool m_open = true;

    std::string m_input;
public:
    virtual ~MainApp() {}

    void InitializationBeforeLoop() override {
        m_font_regular = Tempo::AddFontFromFileTTF("fonts/Roboto/Roboto-Regular.ttf", 16).value();
        m_font_italic = Tempo::AddFontFromFileTTF("fonts/Roboto/Roboto-Italic.ttf", 16).value();
        m_font_bold = Tempo::AddFontFromFileTTF("fonts/Roboto/Roboto-Bold.ttf", 16).value();

        // Emoji font
        static ImVector<ImWchar> ranges;
        static ImFontConfig cfg;
        cfg.OversampleH = cfg.OversampleV = 1;
        cfg.MergeMode = true;
#ifdef ADVANCED_TEXT
        std::cout << "ADVANCED_TEXT" << std::endl;
        ranges.push_back(0x1); ranges.push_back(0x1FFFF); ranges.push_back(0);
        cfg.FontBuilderFlags |= ImGuiFreeTypeBuilderFlags_LoadColor;
#endif
        m_font_emoji = Tempo::AddFontFromFileTTF("fonts/noto-untouchedsvg.ttf", 32.0f, cfg, ranges).value();
        Tempo::Shortcut shortcut;
        shortcut.keys = { CMD_KEY, GLFW_KEY_Q };
        shortcut.name = "Quit";
        shortcut.description = "Quit the application";
        shortcut.callback = [this]() {
            std::cout << "Quit" << std::endl;
            Tempo::EventQueue::getInstance().post(Tempo::Event_ptr(new Tempo::Event("Tempo/quit")));
            };
        Tempo::KeyboardShortCut::addShortcut(shortcut);
    }

    void FrameUpdate() override {
        ImGui::Begin("My window");

        if (ImGui::Button("Click me")) {
            Tempo::EventQueue::getInstance().post(Tempo::Event_ptr(new Tempo::Event("Tempo/redraw")));
        }
        ImGui::Text("Welcome to the multi-font application");
        Tempo::PushFont(m_font_bold);
        ImGui::Text("This is bold");
        Tempo::PopFont();
        Tempo::PushFont(m_font_emoji);
        ImGui::Text("🤚🏻😳😅😂👽");
        Tempo::PopFont();
        ImGui::InputTextMultiline("Input text", &m_input);
        ImGui::End();
        ImGui::ShowDemoWindow();
    }
    void BeforeFrameUpdate() override {}
};

int main() {
    Tempo::Config config{
        .app_name = "TestApp",
        .app_title = "Hello world",
        // .viewports_focus_all = true,
    };
    // config.maximized = true;

    MainApp* app = new MainApp();
    Tempo::Run(app, config);

    return 0;
}