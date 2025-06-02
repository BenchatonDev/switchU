#include <cstring>
#include <cstdio>
#include <dirent.h>
#include <sys/types.h>
#include <vector>
#include <string>

// Declare OSReport if it's not already declared in an included header
extern "C" void OSReport(const char *format, ...);

std::vector<std::string> game_titles;

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

void load_titles_from_apps() {
    DIR* dir = opendir("/vol/external01/wiiu/apps");
    if (!dir) {
        OSReport("Failed to open apps directory\n");
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char meta_path[512];
            snprintf(meta_path, sizeof(meta_path), "/vol/external01/wiiu/apps/%s/meta.xml", entry->d_name);

            std::string title = get_title_from_meta(meta_path);
            game_titles.push_back(title);
        }
    }

    closedir(dir);
}
