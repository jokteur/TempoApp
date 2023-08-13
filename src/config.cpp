#include "config.h"
#include <iostream>
#include <fstream>

namespace Tempo {
    void trunc_content(const std::string& path) {
        std::ofstream output(path);
    }
    std::fstream check_file(const std::string& path, bool nowrite = false) {
        auto flags = std::ios_base::binary | std::ios::out | std::ios::in;
        if (nowrite)
            flags = std::ios_base::binary | std::ios::in;

        std::fstream fs(path, flags);
        if (!fs.good()) {
            fs.close();
            trunc_content(path);
            fs = std::fstream(path, flags);
        }
        else {
            try {
                toml::parse(path);
            }
            catch (const std::exception& err) {
                std::cout << "Error parsing config file: " << err.what() << std::endl;
                std::cout << "Restarting with clean file" << std::endl;
                fs.close();
                trunc_content(path);
                fs = std::fstream(path, flags);
            }
        }
        return fs;
    }
    void saveWindowSize(const std::string& path, int width, int height) {
        auto fs = check_file(path);
        auto data = toml::parse(path);
        data["window"]["width"] = width;
        data["window"]["height"] = height;
        trunc_content(path);
        fs << data;
    }
    void saveWindowPosition(const std::string& path, int x, int y) {
        auto fs = check_file(path);
        auto data = toml::parse(path);
        data["window"]["x"] = x;
        data["window"]["y"] = y;
        trunc_content(path);
        fs << data;
    }
    void saveWindowMaximized(const std::string& path, bool maximized) {
        auto fs = check_file(path);
        auto data = toml::parse(path);
        data["window"]["maximized"] = maximized;
        trunc_content(path);
        fs << data;
    }
    WindowConfig loadWindowConfig(const std::string& path) {
        auto fs = check_file(path, true);
        auto data = toml::parse(path);

        WindowConfig out;
        if (data.contains("window")) {
            auto window = data["window"];
            if (window.contains("width"))
                out.width = window["width"].as_integer();
            if (window.contains("height"))
                out.height = window["height"].as_integer();
            if (window.contains("x"))
                out.x = window["x"].as_integer();
            if (window.contains("y"))
                out.y = window["y"].as_integer();
            if (window.contains("maximized"))
                out.maximized = window["maximized"].as_boolean();
        }
        return out;
    }
}