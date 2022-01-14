#include <iostream>
#include <tempo.h>

class MainApp : public Tempo::App {
private:
    Tempo::FontID m_font_regular;
    Tempo::FontID m_font_italic;
    Tempo::FontID m_font_bold;
    bool m_open = true;
public:
    virtual ~MainApp() {}

    void InitializationBeforeLoop() override {
        m_font_regular = Tempo::AddFontFromFileTTF("fonts/Roboto/Roboto-Regular.ttf", 16).value();
        m_font_italic = Tempo::AddFontFromFileTTF("fonts/Roboto/Roboto-Italic.ttf", 16).value();
        m_font_bold = Tempo::AddFontFromFileTTF("fonts/Roboto/Roboto-Bold.ttf", 16).value();
    }

    void FrameUpdate() override {
#ifdef IMGUI_HAS_VIEWPORT
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
#else 
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
#endif
        ImGui::Begin("My window", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize);

        if (ImGui::Button("Click me")) {
            std::cout << "Hello world" << std::endl;
        }
        ImGui::Text("Welcome to the multi-font application");
        Tempo::PushFont(m_font_bold);
        ImGui::Text("This is bold");
        Tempo::PopFont();
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

    MainApp* app = new MainApp();
    Tempo::Run(app, config);

    return 0;
}