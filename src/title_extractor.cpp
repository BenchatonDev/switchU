#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <unordered_set>
#include <fstream>
#include <cstring>
#include <cstdio>
#include <dirent.h>
#include <sys/types.h>

#include "util.hpp"
#include "title_extractor.hpp"

int MAX_GAME_LOADS = 12;

std::vector<App> apps;

std::string sanitize_title_for_path(const std::string& title) {
    std::string sanitized = title;
    for (char& c : sanitized) {
        if (!(isalnum(c) || c == '_' || c == '-' || c == ' ')) {
            c = '_';  // Replace anything not a-zA-Z0-9_-space with underscore
        }
    }
    return sanitized;
}

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

App create_sysapp_entry(const MCPTitleListType& title_info, SDL_Renderer* renderer) {
    std::string title = "Unknown / Error";
    std::string base_path = ROOT_PATH + std::string(title_info.path);
    std::string meta_path = base_path + "/meta/meta.xml";
    std::string app_icon = base_path + "/meta/iconTex.tga";

    // Parse title from meta.xml
    FILE* file = fopen(meta_path.c_str(), "r");
    if (file) {
        char line[512];
        while (fgets(line, sizeof(line), file)) {
            char* start = strstr(line, "<longname_en type=\"string\" length=\"512\">");
            if (start) {
                start += strlen("<longname_en type=\"string\" length=\"512\">");
                char* end = strstr(start, "</longname_en>");
                if (end) {
                    *end = '\0';
                    title = std::string(start);
                    break;
                }
            }
        }
        fclose(file);
    } else {
        printf("Failed to open meta.xml for %s\n", meta_path.c_str());
    }

    // Attempt to load custom icon from SD
    std::string safe_folder_name = sanitize_title_for_path(title);
    std::string custom_icon_path = SD_CARD_PATH "switchU/custom_icons/" + safe_folder_name + "/icon.png";
    SDL_Texture* icon = load_texture(custom_icon_path.c_str(), renderer);

    // Fallback to iconTex.tga if custom icon not found
    if (!icon) {
        SDL_RWops* tmp = SDL_RWFromFile(app_icon.c_str(), "rb");
        if (tmp) {
            SDL_Surface* surface = IMG_LoadTGA_RW(tmp);
            if (surface) {
                icon = SDL_CreateTextureFromSurface(renderer, surface);
                SDL_FreeSurface(surface);
            }
            SDL_FreeRW(tmp);
        }
        if (!icon) {
            printf("Failed to load icon for system app: %s\n", title.c_str());
        }
    }

    App entry = {
        title,
        base_path,
        title_info.indexedDevice,
        title_info.titleId,
        icon
    };

    return entry;
}

static std::string find_launchable_file(const std::string& app_dir) {
    DIR* dir = opendir(app_dir.c_str());
    if (!dir) return "";

    std::string found_rpx, found_wuhb;
    struct dirent* entry;
    int loaded_count = 0;

    while ((entry = readdir(dir)) != nullptr && loaded_count < MAX_GAME_LOADS) {
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

    printf("Starting Hombrew app scan...\n");
    struct dirent* entry;
    int loaded_count = 0;

    while ((entry = readdir(dir)) != nullptr && loaded_count < MAX_GAME_LOADS) {
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

            App entry = { app_folder, launch_file, "sd", 0, icon };
            apps.push_back(entry);
            loaded_count++;
            printf("Loaded app: %s -> %s\n", app_folder.c_str(), launch_file.c_str());
        }
    }
    closedir(dir);

    printf("Starting System app scan...\n");
    MCPError handle = MCP_Open();
    if (handle < 0) {
        printf("Failed to start MCP\n");
        return;
    }

    uint32_t game_count = 0;
    uint32_t title_count = MCP_TitleCount(handle);
    if (title_count <= 0) {
        printf("No titles found\n");
        MCP_Close(handle);
        return;
    }
    printf("Found %d apps\n", title_count);

    // More stuff from Launchiine, my way only worked on Cemu for some reason
    std::vector<MCPTitleListType> titles(title_count);
    for (MCPAppType type : supported_sys_app_type) {
        uint32_t game_count_per_type = 0;
        MCPError err = MCP_TitleListByAppType(
                 handle, type,&game_count_per_type, titles.data() + game_count,
                 (titles.size() - game_count) * sizeof(decltype(titles)::value_type));

        if (err < 0) {
            printf("Failed to get installed games of type %d\n", type);
            MCP_Close(handle);
            return;
        }

        game_count += game_count_per_type;
    }

    if (game_count != titles.size()) {
        titles.resize(game_count);
    }
    printf("Found %d system games\n", game_count);

    for (auto game : titles) {
        if (apps.size() >= MAX_GAME_LOADS) break;
        App entry = create_sysapp_entry(game, renderer);
        if (entry.storage_device == device_odd) {
            apps.insert(apps.begin(), entry); // Making ODD Always First
        } else {
            apps.push_back(entry);
        }
        printf("Loaded system app: %s -> %s\n", entry.title.c_str(), entry.app_path.c_str());
    }
    MCP_Close(handle);

    const char* path_char = SD_CARD_PATH "scanresult.txt";
    std::string path = path_char;
    FILE* out = fopen(path.c_str(), "w");
    for (auto app : apps) {
        fprintf(out,    "App: %s, Path: %s, Device: %s, TitleID: %llu\n",
               app.title.c_str(), app.app_path.c_str(), app.storage_device.c_str(), app.titleid);
    }
    fclose(out);
}

const char* get_selected_app_path() {
    if (apps.empty()) return nullptr;
    const std::string& full_path = apps[cur_selected_tile].app_path;
    size_t pos = full_path.find("wiiu/apps/");
    if (pos == std::string::npos) return nullptr;
    printf("Selected index: %d, full path: %s, trimmed path: %s\n", cur_selected_tile, full_path.c_str(), full_path.c_str() + pos);
    return full_path.c_str() + pos;
}
