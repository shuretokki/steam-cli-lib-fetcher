#pragma once

#include <algorithm>
#include <filesystem>
#include <fmt/color.h>
#include <fmt/format.h>
#include <fstream>
#include <httplib.h>
#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>
#include <regex>
#include <string>
#include <unordered_map>
#include <vector>

extern std::string data_filename;
extern bool        verbose;
extern bool        has_fetched;
extern std::string api_key;

struct Game;
struct User;
extern std::vector<Game> games;
extern User              user;

struct Prefix
{
        struct Node
        {
                std::unordered_map<char, Node*> children;
                std::vector<size_t>             indices;
        };
        Node* root;
        Prefix() : root(new Node())
        {
        }
        ~Prefix();
        void                insert(const std::string& name, size_t index);
        std::vector<size_t> search(const std::string& prefix) const;

      private:
        void clear(Node* node);
        void collect_indices(Node* node, std::vector<size_t>& indices) const;
};

extern Prefix                                  games_prefix;
extern std::unordered_map<std::string, size_t> games_map;

std::string              to_lower(const std::string& s);
void                     save_games();
void                     load_games();
bool                     fetch_games(const std::string& id);
void                     search_prefix(const std::string& prefix);
void                     count_played();
void                     export_csv(const std::string& filename);
void                     list_games();
void                     list_table();
void                     list_by_letter();
void                     list_by_playtime();
void                     show_help();
std::vector<std::string> parse_command(const std::string& line);
void                     process_command(const std::vector<std::string>& args);