#pragma once
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem> // C++17 标准库，用于处理路径

class WebPage {
public:
    static std::string readFile(const std::string& filename) {
        std::string path = "static/" + filename;

        std::ifstream in(path, std::ios::in | std::ios::binary);
        if (in) {
            std::ostringstream contents;
            contents << in.rdbuf();
            in.close();
            return contents.str();
        } else {
            std::cerr << "Warning: Could not open file: " << path << std::endl;
            return "";
        }
    }

    static std::string getMimeType(const std::string& filename) {
        if (filename.find(".html") != std::string::npos) return "text/html";
        if (filename.find(".png") != std::string::npos) return "image/png";
        if (filename.find(".jpg") != std::string::npos || filename.find(".jpeg") != std::string::npos) return "image/jpeg";
        if (filename.find(".css") != std::string::npos) return "text/css";
        if (filename.find(".js") != std::string::npos) return "application/javascript";
        if (filename.find(".mp3") != std::string::npos) return "audio/mpeg";
        return "text/plain";
    }
};