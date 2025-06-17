#pragma once

#include <coreinit/mcp.h>
#include <nn/acp/title.h>
#include <SDL2/SDL.h>
#include <string>
#include <vector>

struct App {
    std::string title;
    std::string app_path;
    uint64_t titleid;
    SDL_Texture* icon;
};

static const std::vector<MCPAppType> supported_sys_app_type {
    MCP_APP_TYPE_GAME,
    MCP_APP_TYPE_GAME_WII
};

extern std::vector<App> apps;
extern int cur_selected_tile;

SDL_Texture* load_texture(const char* path, SDL_Renderer* renderer);

// Parses and returns the <name> from a given meta.xml path
std::string get_title_from_meta(const char* path);

// Returns a new App entry for a system app with icon and all
App create_sysapp_entry(const MCPTitleListType& title_info, SDL_Renderer* renderer);

// Fills the apps vector with valid launchable apps (populates icon, launch_path, etc.)
void scan_apps(SDL_Renderer* renderer);

// Returns the selected app's path to pass into RPXLoader_LaunchHomebrew()
const char* get_selected_app_path();