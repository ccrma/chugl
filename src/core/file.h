#pragma once

#include <cstdlib>
#include <cstring>

#include "log.h"
#include "macros.h"

struct FileReadResult {
    u32 size;
    u8* data_owned;
};

std::string File_dirname(const std::string& path)
{
    size_t last_slash = path.find_last_of("/\\");
    if (last_slash == std::string::npos) {
        return path;
    }
    return path.substr(0, last_slash + 1);
}

int File_exists(const char* filename)
{
    /* try to open file to read */
    FILE* file;
    if ((file = fopen(filename, "r"))) {
        fclose(file);
        return 1;
    }
    return 0;
}

const char* File_getExtension(const char* filename)
{
    const char* dot = strrchr(filename, '.');
    if (!dot || dot == filename) {
        return "";
    }
    return dot + 1;
}

int File_hasExtension(const char* filename, const char* extension)
{
    const char* filename_extension = File_getExtension(filename);
    return strcmp(filename_extension, extension) == 0 ? 1 : 0;
}

FileReadResult File_read(const char* filename, int is_text_file)
{
    ASSERT(filename);
    FileReadResult result = {};

    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        log_error("Unable to open file '%s'\n", filename);
        exit(1);
    }
    fseek(file, 0, SEEK_END);
    result.size = ftell(file);
    fseek(file, 0, SEEK_SET);

    result.data_owned = (u8*)malloc(result.size + (is_text_file == 0 ? 0 : 1));
    fread(result.data_owned, 1, result.size, file);
    fclose(file);
    if (is_text_file != 0) {
        result.data_owned[result.size] = 0;
    }
    return result;
}
