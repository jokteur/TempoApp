#pragma once

#include <toml.hpp>
#include <string>

namespace Tempo {
    struct WindowConfig {
        int width = 0;
        int height = 0;
        int x = -1000000;
        int y = -1000000;
        bool maximized = false;
    };

    void saveWindowSize(const std::string& path, int width, int height);
    void saveWindowPosition(const std::string& path, int x, int y);
    void saveWindowMaximized(const std::string& path, bool maximized);

    /**
     * Load window configuration from file
     * @param path path to the file
     *
     * @return WindowConfig, if some options are not present in the file, they will be set to default values
    */
    WindowConfig loadWindowConfig(const std::string& path);

}