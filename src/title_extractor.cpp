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
    const char* custom_icons_dir = "/vol/external01/switchU/custom_icons/";

    DIR* dir = opendir(apps_dir);
    if (!dir) {
        OSReport("Failed to open apps directory\n");
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {
            std::string app_name = entry->d_name;

            std::string custom_icon_path = std::string(custom_icons_dir) + app_name + "/icon.png";
            std::string default_icon_path = std::string(apps_dir) + app_name + "/icon.png";

            SDL_Texture* icon = nullptr;

            // Try loading custom icon first
            FILE* test = fopen(custom_icon_path.c_str(), "rb");
            if (test) {
                fclose(test);
                icon = load_texture(custom_icon_path.c_str(), renderer);
                OSReport("Loaded custom icon for %s\n", app_name.c_str());
            } else {
                // Fallback to default icon
                icon = load_texture(default_icon_path.c_str(), renderer);
                if (!icon) {
                    OSReport("No icon for app: %s\n", app_name.c_str());
                    continue;
                }
            }

            apps.push_back({ app_name, icon });
        }
    }

    closedir(dir);
}
