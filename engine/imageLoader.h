#pragma once

#include <string>
#include <vector>

struct Image {
    int width = 0;
    int height = 0;
    int channels = 0;
    std::vector<unsigned char> pixels;
};

bool loadJpegImage(const std::string& filename, Image& image);
