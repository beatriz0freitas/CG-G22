#include "imageLoader.h"

#include <csetjmp>
#include <cstring>
#include <stddef.h>
#include <stdio.h>

extern "C" {
#include <jpeglib.h>
}

struct JpegError {
    jpeg_error_mgr pub;
    jmp_buf jumpBuffer;
};

static void jpegErrorExit(j_common_ptr cinfo) {
    auto* err = reinterpret_cast<JpegError*>(cinfo->err);
    longjmp(err->jumpBuffer, 1);
}

bool loadJpegImage(const std::string& filename, Image& image) {
    FILE* file = fopen(filename.c_str(), "rb");
    if (!file) return false;

    jpeg_decompress_struct cinfo;
    JpegError jerr;

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = jpegErrorExit;

    if (setjmp(jerr.jumpBuffer)) {
        jpeg_destroy_decompress(&cinfo);
        fclose(file);
        return false;
    }

    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, file);
    jpeg_read_header(&cinfo, TRUE);
    cinfo.out_color_space = JCS_RGB;
    jpeg_start_decompress(&cinfo);

    image.width = (int)cinfo.output_width;
    image.height = (int)cinfo.output_height;
    image.channels = (int)cinfo.output_components;

    const int rowStride = image.width * image.channels;
    std::vector<unsigned char> topDown(rowStride * image.height);

    while (cinfo.output_scanline < cinfo.output_height) {
        unsigned char* row = topDown.data() + cinfo.output_scanline * rowStride;
        jpeg_read_scanlines(&cinfo, &row, 1);
    }

    image.pixels.resize(topDown.size());
    for (int y = 0; y < image.height; ++y) {
        const unsigned char* src = topDown.data() + (image.height - 1 - y) * rowStride;
        unsigned char* dst = image.pixels.data() + y * rowStride;
        memcpy(dst, src, rowStride);
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(file);
    return true;
}
