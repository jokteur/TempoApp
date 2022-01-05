#include <iostream>
#include <tempo.h>

class MainApp : public Tempo::App {
public:
    virtual ~MainApp() {}

    void FrameUpdate() override {
        ImGui::Begin("My window");

        if (ImGui::Button("Click me")) {
            std::cout << "Hello world" << std::endl;
        }

        ImGui::End();

        ImGui::Begin("2nd window");

        if (ImGui::CollapsingHeader("Header")) {
            ImGui::Text("Hello");
            // ImGui::End();
        }

        ImGui::End();
    }
    void BeforeFrameUpdate() override {}
};

int main() {
    Tempo::Config config{
        .app_name = "TestApp",
        .app_title = "My title",
        .poll_or_wait = Tempo::Config::POLL
    };

    // Tempo::SetMultiViewportsFocusBehavior(true);

    MainApp* app = new MainApp();
    Tempo::Run(app, config);

    return 0;
}