#include "utils.h"

#include "tempo.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_glfw.h"

static int min(int x, int y) {
    return x < y ? x : y;
}
static int max(int x, int y) {
    return x > y ? x : y;
}

namespace Tempo {
    bool is_alpha_numeric(char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9');
    }
    std::string strToPathFriendly(const std::string& str) {
        std::string result = "";
        for (char c : str) {
            if (is_alpha_numeric(c))
                result += c;
            else
                result += '_';
        }
        return result;
    }
    std::string nameToAppConfigFile(const std::string& name) {
        return "cfg_" + strToPathFriendly(name) + ".toml";
    }

    // Adapted from https://stackoverflow.com/a/31526753
    GLFWmonitor* getCurrentMonitor(GLFWwindow* window) {
        int nmonitors, i;
        int wx, wy, ww, wh;
        int mx, my, mw, mh;
        int overlap, bestoverlap;
        GLFWmonitor* bestmonitor;
        GLFWmonitor** monitors;
        const GLFWvidmode* mode;

        bestoverlap = 0;
        bestmonitor = glfwGetPrimaryMonitor();

        glfwGetWindowPos(window, &wx, &wy);
        glfwGetWindowSize(window, &ww, &wh);
        monitors = glfwGetMonitors(&nmonitors);

        for (i = 0; i < nmonitors; i++) {
            mode = glfwGetVideoMode(monitors[i]);
            glfwGetMonitorPos(monitors[i], &mx, &my);
            mw = mode->width;
            mh = mode->height;

            overlap = max(0, min(wx + ww, mx + mw) - max(wx, mx)) *
                max(0, min(wy + wh, my + mh) - max(wy, my));

            if (bestoverlap < overlap) {
                bestoverlap = overlap;
                bestmonitor = monitors[i];
            }
        }
        return bestmonitor;
    }
    void renderApplication(GLFWwindow* window, int width, int height, App* application) {
        // Rendering
        ImGuiIO& io = ImGui::GetIO();


        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        if (application != nullptr)
            application->FrameUpdate();
        ImGui::Render();

        // int width, display_h;
        // glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        glClearColor(0.5, 0.5, 0.5, 0);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        if (!app_state.skip_frame)
            glfwSwapBuffers(window);
        app_state.skip_frame = false;
    }
}