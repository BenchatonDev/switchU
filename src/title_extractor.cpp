#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <cstring>
#include <cstdio>
#include <dirent.h>
#include <sys/types.h>
#include <vector>
#include <string>

#include "title_extractor.hpp"

// Declare OSReport if it's not already declared in an included header
extern "C" void OSReport(const char *format, ...);

std::string get_title_from_meta(const char* path) {
    FILE* file = fopen(path, "r");
    if (!file) return "Unknown";

    char line[512];
    while (fgets(line, sizeof(line), file)) {
        char* start = strstr(line, "<name>");
        if (start) {
            start += strlen("<name>");
            char* end = strstr(start, "</name>");
            if (end) {
                *end = '\0';
                fclose(file);
                return std::string(start);
            }
        }
    }

    fclose(file);
    return "Unknown";
}

void load_apps(SDL_Renderer* renderer) {
    const char* apps_dir = "/vol/external01/wiiu/apps/";
    DIR* dir = opendir(apps_dir);
    if (!dir) {
        OSReport("Failed to open apps directory\n");
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {
            std::string app_path = std::string(apps_dir) + entry->d_name;
            std::string icon_path = app_path + "/icon.png";

            SDL_Texture* icon = load_texture(icon_path.c_str(), renderer);
            if (!icon) {
                OSReport("No icon for app: %s\n", entry->d_name);
                continue;
            }

            apps.push_back({ entry->d_name, icon });
        }
    }

    closedir(dir);
}
