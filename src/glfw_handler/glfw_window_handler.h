#pragma once

#include <map>

#include "tempo.h"

namespace Tempo {
    /**
     * This class manages all GLFW windows (Created by the user or ImGui)
     * This class for examples allows to bring all windows to the front
     * if the user clicks on only one of the window
     */
    class GLFWwindowHandler {
    private:
        static std::multimap<int, GLFWwindow*> windows;
        static bool all_windows_unfocused;

    public:
        /**
         * If focus_all is set to true, then
         */
        static bool focus_all;
        static App* application;

        /**
         * Callback for GLFW when resizing the window
         * @param window
         * @param width
         * @param height
         */
        static void framebuffer_size_callback(GLFWwindow* window, int width, int height);

        /**
         * Callback for GLFW when any window is focused or defocused
         * @param window glfw window pointer
         * @param focused state of the focus (GLFW_TRUE or GLFW_FALSE)
         */
        static void focus_callback(GLFWwindow* window, int focused);

        /**
         * Shows all the current windows ordered by the z_index
         */
        static void focusAll();

        /**
         * Sets the z index for a particular GLFWwindow
         * The window with the lowest index will be in background
         * and the window with the highest index will be on the front
         * (when calling focusAll())
         * @param window glfw window pointer
         * @param z_index z index (low to back, high to front)
         */
        static void setZIndex(GLFWwindow* window, int z_index = 0);

        /**
         * Makes the class aware of a GLFW window
         * @param window glfw window pointer
         * @param z_index z index (low to back, high to front)
         */
        static void addWindow(GLFWwindow* window, int z_index = 0, bool resize_callback = false);

        /**
         * Removes a GLFW window from the class
         * (to be called when the users destroy the window pointer)
         * @param window
         */
        static void removeWindow(GLFWwindow* window);
    };
}