#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <unordered_set>
#include <fstream>
#include <cstring>
#include <cstdio>
#include <dirent.h>
#include <sys/types.h>
#include <vector>
#include <string>

#include "common.hpp"
#include "title_extractor.hpp"

std::vector<App> apps;

std::unordered_set<std::string> load_ignored_apps() {
    std::unordered_set<std::string> ignored;
    std::ifstream file(SD_CARD_PATH "switchU/ignore.txt");
    if (!file.is_open()) {
        printf("No ignore.txt found or failed to open.\n");
        return ignored;
    }

    std::string line;
    while (std::getline(file, line)) {
        // Trim whitespace and \r
        line.erase(0, line.find_first_not_of(" \t\r\n")); // left trim
        line.erase(line.find_last_not_of(" \t\r\n") + 1); // right trim
        if (!line.empty()) {
            printf("Ignoring app: %s\n", line.c_str());
            ignored.insert(line);
        }
    }

    file.close();
    return ignored;
}

std::string get_title_from_meta(const char* path) {
    FILE* file = fopen(path, "r");
    if (!file) return "Unknown";

    char line[512];
    while (fgets(line, sizeof(line), file)) {
        char* start = strstr(line, "<title>");
        if (start) {
            start += strlen("<title>");
            char* end = strstr(start, "</title>");
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

static std::string find_launchable_file(const std::string& app_dir) {
    DIR* dir = opendir(app_dir.c_str());
    if (!dir) return "";

    std::string found_rpx, found_wuhb;
    struct dirent* entry;

    while ((entry = readdir(dir)) != nullptr) {
        std::string fname = entry->d_name;

        if (fname.size() >= 5 && fname.substr(fname.size() - 5) == ".wuhb") {
            found_wuhb = app_dir + "/" + fname;
            break;
        } else if (fname.size() >= 4 && fname.substr(fname.size() - 4) == ".rpx") {
            found_rpx = app_dir + "/" + fname;
        }
    }

    closedir(dir);

    if (!found_wuhb.empty()) return found_wuhb;
    return found_rpx;
}

void scan_apps(SDL_Renderer* renderer) {
    apps.clear();

    const char* apps_dir = SD_CARD_PATH "wiiu/apps/";
    const char* custom_icons_dir = SD_CARD_PATH "switchU/custom_icons/";

    std::unordered_set<std::string> ignored_apps = load_ignored_apps();

    DIR* dir = opendir(apps_dir);
    if (!dir) {
        printf("Failed to open apps directory\n");
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {
            std::string app_folder = entry->d_name;

            if (ignored_apps.find(app_folder) != ignored_apps.end()) {
                printf("Skipping ignored app: %s\n", app_folder.c_str());
                continue;
            }

            std::string app_path = std::string(apps_dir) + app_folder;

            std::string launch_file = find_launchable_file(app_path);
            if (launch_file.empty()) {
                printf("No launchable .wuhb or .rpx found in %s\n", app_folder.c_str());
                continue;
            }

            std::string custom_icon_path = std::string(custom_icons_dir) + app_folder + "/icon.png";
            std::string default_icon_path = app_path + "/icon.png";

            SDL_Texture* icon = nullptr;

            FILE* test = fopen(custom_icon_path.c_str(), "rb");
            if (test) {
                fclose(test);
                icon = load_texture(custom_icon_path.c_str(), renderer);
            } else {
                icon = load_texture(default_icon_path.c_str(), renderer);
            }

            if (!icon) {
                printf("No icon for app: %s\n", app_folder.c_str());
                continue;
            }

            App entry = { app_folder, launch_file, icon };
            apps.push_back(entry);
            printf("Loaded app: %s -> %s\n", app_folder.c_str(), launch_file.c_str());
        }
    }

    closedir(dir);
}

const char* get_selected_app_path() {
    if (apps.empty()) return nullptr;
    const std::string& full_path = apps[cur_selected_tile].launch_path;
    size_t pos = full_path.find("wiiu/apps/");
    if (pos == std::string::npos) return nullptr;
    printf("Selected index: %d, full path: %s, trimmed path: %s\n", cur_selected_tile, full_path.c_str(), full_path.c_str() + pos);
    return full_path.c_str() + pos;
}
