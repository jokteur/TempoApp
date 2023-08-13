#include "glfw_window_handler.h"

#include <iostream>

#include "../events.h"
#include "../utils.h"
#include "../keyboard_shortcuts.h"
#include "../config.h"

namespace Tempo {
    std::multimap<int, GLFWwindow*> GLFWwindowHandler::windows;
    bool GLFWwindowHandler::focus_all = false;
    bool GLFWwindowHandler::all_windows_unfocused = false;
    App* GLFWwindowHandler::application = nullptr;
    std::string GLFWwindowHandler::config_name = "default";

    void GLFWwindowHandler::focus_callback(GLFWwindow*, int) {
        // If previously all windows were unfocused and
        // the user clicked on a window, we can put all windows from
        // the app to the front (if focus_all == true)

        // For now, this is causing seg faults on MacOs Big Sur M1 chips
        // Disabling

        // if (all_windows_unfocused && focus_all && focused == GLFW_TRUE) {
        //     focusAll();
        //     all_windows_unfocused = false;
        //     return;
        // }
        // all_windows_unfocused = true;
        // for (auto& element : windows) {
        //     if (glfwGetWindowAttrib(element.second, GLFW_FOCUSED) == GLFW_TRUE) {
        //         all_windows_unfocused = false;
        //     }
        // }
    }

    void GLFWwindowHandler::focusAll() {
        for (auto& element : windows) {
            int focused = glfwGetWindowAttrib(element.second, GLFW_FOCUSED);
            if (focused == GLFW_FALSE)
                glfwShowWindow(element.second);
        }
    }

    void GLFWwindowHandler::setAppName(const std::string& name) {
        config_name = nameToAppConfigFile(name);
    }

    void GLFWwindowHandler::addWindow(GLFWwindow* window, int z_index, bool main_window) {
        windows.insert(std::pair<int, GLFWwindow*>(z_index, window));
        // glfwSetWindowFocusCallback(window, &GLFWwindowHandler::focus_callback);
        auto fun = glfwSetKeyCallback(window, &KeyboardShortCut::key_callback);
        KeyboardShortCut::set_prev_key_callback(fun);
        //glfwSetCharCallback(window, &KeyboardShortCut::character_callback);
        if (main_window) {
            glfwSetFramebufferSizeCallback(window, &GLFWwindowHandler::framebuffer_size_callback);
            glfwSetWindowMaximizeCallback(window, &GLFWwindowHandler::window_maximize_callback);
            glfwSetWindowPosCallback(window, &GLFWwindowHandler::window_pos_callback);
        }
    }

    void GLFWwindowHandler::removeWindow(GLFWwindow* window) {
        for (auto it = windows.begin(); it != windows.end(); it++) {
            if (it->second == window) {
                windows.erase(it);
                break;
            }
        }
    }

    void GLFWwindowHandler::setZIndex(GLFWwindow* window, int z_index) {
        removeWindow(window);
        addWindow(window, z_index);
    }

    void GLFWwindowHandler::framebuffer_size_callback(GLFWwindow* window, int width, int height) {
        // TODO Multi-threaded app: https://stackoverflow.com/a/56614042/8523520
        renderApplication(window, width, height, application);
        saveWindowSize(config_name, width, height);
    }
    void GLFWwindowHandler::window_maximize_callback(GLFWwindow*, int maximized) {
        saveWindowMaximized(config_name, maximized == GLFW_TRUE);
    }
    void GLFWwindowHandler::window_pos_callback(GLFWwindow*, int x, int y) {
        saveWindowPosition(config_name, x, y);
    }
}