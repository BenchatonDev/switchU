#ifndef TITLE_EXTRACTOR_HPP
#define TITLE_EXTRACTOR_HPP

#include <string>
#include <vector>

// Externally accessible vector of game titles loaded from meta.xml files
extern std::vector<std::string> game_titles;

// Parses and returns the <name> from a given meta.xml path
std::string get_title_from_meta(const char* path);

// Loads all game titles from the /wiiu/apps/*/meta.xml directory
void load_titles_from_apps();

#endif // TITLE_EXTRACTOR_HPP
