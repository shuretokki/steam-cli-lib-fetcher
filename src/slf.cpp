#include "slf.hpp"

using namespace fmt;
using json = nlohmann::json;

struct Game
{
        std::string name;
        int         appid;
        int         playtime;
};

struct User
{
        std::string username = "";
        std::string location = "";
        std::string steamid  = "";
};

std::string                             data_filename = "data/games.json";
bool                                    has_fetched   = false;
std::string                             api_key;
std::vector<Game>                       games;
User                                    user;
Prefix                                  games_prefix;
std::unordered_map<std::string, size_t> games_map;

std::string to_lower(const std::string& s)
{
        std::string result = s;
        for (char& c : result) {
                c = std::tolower(c);
        }
        return result;
}

void Prefix::clear(Node* node)
{
        if (!node)
                return;
        for (auto& child : node->children) {
                clear(child.second);
        }
        delete node;
}

Prefix::~Prefix()
{
        clear(root);
}

void Prefix::insert(const std::string& name, size_t idx)
{
        Node*       current    = root;
        std::string lower_name = to_lower(name);
        for (char c : lower_name) {
                if (!current->children.count(c)) {
                        current->children[c] = new Node();
                }
                current = current->children[c];
        }
        current->ix.push_back(idx);
}

std::vector<size_t> Prefix::search(const std::string& prefix) const
{
        std::vector<size_t> result;
        Node*               current      = root;
        std::string         lower_prefix = to_lower(prefix);
        for (char c : lower_prefix) {
                if (!current->children.count(c)) {
                        return result;
                }
                current = current->children[c];
        }
        collect_indices(current, result);
        return result;
}

void Prefix::collect_indices(Node* node, std::vector<size_t>& ix) const
{
        if (!node)
                return;
        ix.insert(ix.end(), node->ix.begin(), node->ix.end());
        for (const auto& child : node->children) {
                collect_indices(child.second, ix);
        }
}

bool load_api_key()
{
        std::string   config_file = "data/config.json";
        std::ifstream ifs(config_file);
        if (!ifs.is_open()) {
                print(
                    fg(color::yellow),
                    "No API key found at {}. Enter your Steam API key:\n",
                    config_file);
                std::string key;
                std::getline(std::cin, key);
                if (key.empty()) {
                        print(
                            fg(color::indian_red),
                            "Error: API key cannot be empty.\n");
                        return false;
                }
                std::filesystem::create_directories("data");
                std::ofstream ofs(config_file);
                if (!ofs.is_open()) {
                        print(
                            fg(color::indian_red),
                            "Error: Cannot create {}.\n",
                            config_file);
                        return false;
                }
                json j;
                j["api_key"] = key;
                ofs << j.dump(4);
                api_key = key;
        } else {
                try {
                        json j;
                        ifs >> j;
                        api_key = j["api_key"].get<std::string>();
                        if (api_key.empty()) {
                                print(
                                    fg(color::indian_red),
                                    "Error: API key in {} is empty.\n",
                                    config_file);
                                return false;
                        }
                } catch (const std::exception& e) {
                        print(
                            fg(color::indian_red),
                            "Error: Failed to read {}: {}.\n",
                            config_file,
                            e.what());
                        return false;
                }
        }
        return true;
}

void save_games()
{
        std::filesystem::create_directories(
            std::filesystem::path(data_filename).parent_path());
        std::ofstream ofs(data_filename);
        if (!ofs.is_open()) {
                print(
                    fg(color::indian_red),
                    "Error: Cannot write to {}.\n",
                    data_filename);
                return;
        }
        json j;
        j["user"]["username"] = user.username;
        1j["user"]["location"] = user.location;
        j["user"]["steamid"]  = user.steamid;
        for (const auto& game : games) {
                json g;
                g["name"]     = game.name;
                g["appid"]    = game.appid;
                g["playtime"] = game.playtime;
                j["games"].push_back(g);
        }
        ofs << j.dump(4);
}

void load_games()
{
        if (!load_api_key()) {
                print(fg(color::indian_red), "Error: Need a valid API key.\n");
                return;
        }
        std::ifstream ifs(data_filename);
        if (!ifs.is_open()) {
                return;
        }
        try {
                json j;
                ifs >> j;
                if (j.contains("user")) {
                        user.username =
                            j["user"]["username"].get<std::string>();
                        user.location =
                            j["user"]["location"].get<std::string>();
                        user.steamid = j["user"]["steamid"].get<std::string>();
                        has_fetched  = true;
                }
                if (j.contains("games")) {
                        games.clear();
                        games_prefix = Prefix();
                        games_map.clear();
                        for (size_t i = 0; i < j["games"].size(); ++i) {
                                Game game;
                                game.name =
                                    j["games"][i]["name"].get<std::string>();
                                game.appid = j["games"][i]["appid"].get<int>();
                                game.playtime =
                                    j["games"][i]["playtime"].get<int>();
                                games.push_back(game);
                                games_prefix.insert(game.name, i);
                                games_map[to_lower(game.name)] = i;
                        }
                }
        } catch (const std::exception& e) {
                print(
                    fg(color::indian_red),
                    "Error: Failed to read {}: {}.\n",
                    data_filename,
                    e.what());
        }
}

bool fetch_games(const std::string& id)
{
        std::string steamid;
        if (std::regex_match(id, std::regex("\\d{17}"))) {
                steamid = id;
        } else {
                std::string custom_id = id;
                std::regex  url_regex("https?://steamcommunity.com/id/([^/]+)");
                std::smatch match;
                if (std::regex_match(id, match, url_regex)) {
                        custom_id = match[1].str();
                }
                httplib::Client cli("http://api.steampowered.com");
                cli.set_connection_timeout(5, 0);
                cli.set_read_timeout(5, 0);
                std::string url = fmt::format(
                    "/ISteamUser/ResolveVanityURL/v0001/?key={}&vanityurl={}",
                    api_key,
                    custom_id);
                auto res = cli.Get(url.c_str());
                if (!res || res->status != 200) {
                        print(
                            fg(color::indian_red),
                            "Error: Cannot resolve Steam ID.\n");
                        return false;
                }
                try {
                        json j = json::parse(res->body);
                        if (j["response"]["success"] != 1) {
                                print(
                                    fg(color::indian_red),
                                    "Error: Invalid custom ID '{}'.\n",
                                    custom_id);
                                return false;
                        }
                        steamid = j["response"]["steamid"].get<std::string>();
                } catch (const std::exception& e) {
                        print(
                            fg(color::indian_red),
                            "Error: Failed to parse response: {}.\n",
                            e.what());
                        return false;
                }
        }

        httplib::Client cli("http://api.steampowered.com");
        cli.set_connection_timeout(5, 0);
        cli.set_read_timeout(5, 0);
        std::string summary_url = fmt::format(
            "/ISteamUser/GetPlayerSummaries/v0002/?key={}&steamids={}",
            api_key,
            steamid);
        auto summary_res = cli.Get(summary_url.c_str());
        if (!summary_res || summary_res->status != 200) {
                print(
                    fg(color::indian_red),
                    "Error: Cannot fetch user info.\n");
                return false;
        }
        try {
                json j        = json::parse(summary_res->body);
                auto player   = j["response"]["players"][0];
                user.steamid  = steamid;
                user.username = player["personaname"].get<std::string>();
                user.location =
                    player.contains("locstatecode")
                            && player.contains("loccountrycode") ?
                        fmt::format(
                            "{}, {}",
                            player["locstatecode"].get<std::string>(),
                            player["loccountrycode"].get<std::string>()) :
                        "Unknown";
        } catch (const std::exception& e) {
                print(
                    fg(color::indian_red),
                    "Error: Failed to parse user info: {}.\n",
                    e.what());
                return false;
        }

        std::string games_url = fmt::format(
            "/IPlayerService/GetOwnedGames/v0001/"
            "?key={}&steamid={}&format=json&include_appinfo=1",
            api_key,
            steamid);
        auto games_res = cli.Get(games_url.c_str());
        if (!games_res || games_res->status != 200) {
                print(fg(color::indian_red), "Error: Cannot fetch games.\n");
                return false;
        }
        try {
                json j = json::parse(games_res->body);
                if (!j.contains("response")
                    || !j["response"].contains("games")) {
                        print(
                            fg(color::indian_red),
                            "Error: No games found.\n");
                        return false;
                }
                auto game_list = j["response"]["games"];
                if (game_list.empty()) {
                        print(
                            fg(color::yellow),
                            "Warning: No games or profile is private.\n");
                        return false;
                }
                games.clear();
                games_prefix = Prefix();
                games_map.clear();
                has_fetched = true;
                for (size_t i = 0; i < game_list.size(); ++i) {
                        Game game;
                        game.name  = game_list[i]["name"].get<std::string>();
                        game.appid = game_list[i]["appid"].get<int>();
                        game.playtime =
                            game_list[i].contains("playtime_forever") ?
                                game_list[i]["playtime_forever"].get<int>() :
                                0;
                        games.push_back(game);
                        games_prefix.insert(game.name, i);
                        games_map[to_lower(game.name)] = i;
                }
                save_games();
                print(
                    fg(color::light_green),
                    "Fetched {} games.\n",
                    games.size());
                print(
                    fg(color::yellow),
                    "Profile: https://steamcommunity.com/profiles/{}\n",
                    steamid);
                return true;
        } catch (const std::exception& e) {
                print(
                    fg(color::indian_red),
                    "Error: Failed to parse games: {}.\n",
                    e.what());
                return false;
        }
}

void search_prefix(const std::string& prefix)
{
        auto ix = games_prefix.search(prefix);
        if (ix.empty()) {
                print(fg(color::indian_red), "No games found.\n");
                return;
        }
        size_t name_width = 0;
        for (const auto& game : games) {
                name_width = std::max(name_width, game.name.length());
        }
        name_width = std::min(name_width + 4, size_t(34));
        print(fg(color::light_green) | emphasis::bold, "Matching games:\n");
        for (size_t idx : ix) {
                std::string name = games[idx].name;
                if (name.length() > name_width) {
                        name = name.substr(0, name_width - 3) + "...";
                }
                std::string playtime =
                    games[idx].playtime > 0 ?
                        std::to_string(games[idx].playtime) + " min" :
                        "Not played";
                print(
                    fg(color::white),
                    "{:<10} {:<{}} {:<15}\n",
                    games[idx].appid,
                    name,
                    name_width,
                    playtime);
        }
        print(fg(color::light_green), "\nTotal matches: {}\n", ix.size());
}

void count_played()
{
        if (games.empty()) {
                print(
                    fg(color::indian_red),
                    "No games found. Use 'fetch' first.\n");
                return;
        }
        size_t played = 0;
        for (const auto& pair : games_map) {
                size_t idx = pair.second;
                if (games[idx].playtime > 0) {
                        ++played;
                }
        }
        print(fg(color::light_green), "Number of games played: {}\n", played);
}

void export_csv(const std::string& filename)
{
        if (games.empty()) {
                print(
                    fg(color::indian_red),
                    "No games to export. Use 'fetch' first.\n");
                return;
        }
        std::string path = "data/exported/" + filename + ".csv";
        std::filesystem::create_directories(
            std::filesystem::path(path).parent_path());
        std::ofstream ofs(path);
        if (!ofs.is_open()) {
                print(
                    fg(color::indian_red),
                    "Error: Cannot write to {}.\n",
                    path);
                return;
        }
        ofs << "AppID,Name,Playtime\n";
        for (const auto& game : games) {
                std::string name = game.name;
                std::replace(name.begin(), name.end(), '"', '\'');
                if (name.find(',') != std::string::npos) {
                        name = "\"" + name + "\"";
                }
                std::string playtime =
                    game.playtime > 0 ? std::to_string(game.playtime) + " min" :
                                        "Not played";
                ofs << game.appid << "," << name << "," << playtime << "\n";
        }
        print(
            fg(color::light_green),
            "Exported {} games to {}.\n",
            games.size(),
            path);
}

void list_games()
{
        if (games.empty()) {
                print(
                    fg(color::indian_red),
                    "No games found. Use 'fetch' first.\n");
                return;
        }
        size_t name_width = 0;
        for (const auto& game : games) {
                name_width = std::max(name_width, game.name.length());
        }
        name_width = std::min(name_width + 4, size_t(34));
        print(fg(color::gold) | emphasis::bold, "Games:\n");
        std::vector<size_t> ix(games.size());
        for (size_t i = 0; i < games.size(); ++i)
                ix[i] = i;
        std::sort(ix.begin(), ix.end(), [](size_t a, size_t b) {
                return to_lower(games[a].name) < to_lower(games[b].name);
        });
        for (size_t idx : ix) {
                std::string name = games[idx].name;
                if (name.length() > name_width) {
                        name = name.substr(0, name_width - 3) + "...";
                }
                print(fg(color::white), "{:<{}}\n", name, name_width);
        }
        print(fg(color::light_green), "\nTotal games: {}\n\n", games.size());
}

void list_table()
{
        if (games.empty()) {
                print(
                    fg(color::indian_red),
                    "No games found. Use 'fetch' first.\n");
                return;
        }
        size_t name_width = 0;
        for (const auto& game : games) {
                name_width = std::max(name_width, game.name.length());
        }
        name_width = std::min(name_width + 4, size_t(34));
        print(
            fg(color::cyan) | emphasis::bold,
            "{:<10} {:<{}} {:<15}\n",
            "AppID",
            "Name",
            name_width,
            "Playtime");
        print(
            fg(color::cyan),
            "{:-<10} {:-<{}} {:-<15}\n",
            "",
            "",
            name_width,
            "");
        std::vector<size_t> ix(games.size());
        for (size_t i = 0; i < games.size(); ++i)
                ix[i] = i;
        std::sort(ix.begin(), ix.end(), [](size_t a, size_t b) {
                return to_lower(games[a].name) < to_lower(games[b].name);
        });
        for (size_t idx : ix) {
                std::string name = games[idx].name;
                if (name.length() > name_width) {
                        name = name.substr(0, name_width - 3) + "...";
                }
                std::string playtime =
                    games[idx].playtime > 0 ?
                        std::to_string(games[idx].playtime) + " min" :
                        "Not played";
                print(
                    fg(color::white),
                    "{:<10} {:<{}} {:<15}\n",
                    games[idx].appid,
                    name,
                    name_width,
                    playtime);
        }
        print(fg(color::light_green), "\nTotal games: {}\n", games.size());
}

void list_by_letter()
{
        if (games.empty()) {
                print(
                    fg(color::indian_red),
                    "No games found. Use 'fetch' first.\n");
                return;
        }
        std::vector<std::vector<size_t>> digits;
        std::vector<std::vector<size_t>> letters(26);
        for (size_t i = 0; i < games.size(); ++i) {
                char first = games[i].name.empty() ? ' ' : games[i].name[0];
                if (std::isdigit(first)) {
                        digits.push_back({ i });
                } else if (std::isalpha(first)) {
                        int idx = std::tolower(first) - 'a';
                        if (idx >= 0 && idx < 26) {
                                letters[idx].push_back(i);
                        }
                }
        }
        size_t name_width = 0;
        for (const auto& game : games) {
                name_width = std::max(name_width, game.name.length());
        }
        name_width = std::min(name_width + 4, size_t(34));
        if (!digits.empty()) {
                print(fg(color::gold) | emphasis::bold, "0-9\n");
                for (const auto& idx_vec : digits) {
                        size_t      idx  = idx_vec[0];
                        std::string name = games[idx].name;
                        if (name.length() > name_width) {
                                name = name.substr(0, name_width - 3) + "...";
                        }
                        print(
                            fg(color::white),
                            "{:<{}} {:<10}\n",
                            name,
                            name_width,
                            games[idx].appid);
                }
        }
        for (int i = 0; i < 26; ++i) {
                if (!letters[i].empty()) {
                        if (!digits.empty() || i > 0)
                                print("\n");
                        print(
                            fg(color::gold) | emphasis::bold,
                            "{}\n",
                            static_cast<char>('A' + i));
                        for (size_t idx : letters[i]) {
                                std::string name = games[idx].name;
                                if (name.length() > name_width) {
                                        name = name.substr(0, name_width - 3)
                                               + "...";
                                }
                                print(
                                    fg(color::white),
                                    "{:<{}} {:<10}\n",
                                    name,
                                    name_width,
                                    games[idx].appid);
                        }
                }
        }
        print(fg(color::light_green), "\nTotal games: {}\n", games.size());
}

void list_by_playtime()
{
        if (games.empty()) {
                print(
                    fg(color::indian_red),
                    "No games found. Use 'fetch' first.\n");
                return;
        }
        std::vector<size_t> ix(games.size());
        for (size_t i = 0; i < games.size(); ++i)
                ix[i] = i;
        std::sort(ix.begin(), ix.end(), [](size_t a, size_t b) {
                if (games[a].playtime != games[b].playtime) {
                        return games[a].playtime > games[b].playtime;
                }
                return to_lower(games[a].name) < to_lower(games[b].name);
        });
        size_t name_width = 0;
        for (const auto& game : games) {
                name_width = std::max(name_width, game.name.length());
        }
        name_width = std::min(name_width + 4, size_t(34));
        print(
            fg(color::cyan) | emphasis::bold,
            "{:<10} {:<{}} {:<15}\n",
            "AppID",
            "Name",
            name_width,
            "Playtime");
        print(
            fg(color::cyan),
            "{:-<10} {:-<{}} {:-<15}\n",
            "",
            "",
            name_width,
            "");
        for (size_t idx : ix) {
                std::string name = games[idx].name;
                if (name.length() > name_width) {
                        name = name.substr(0, name_width - 3) + "...";
                }
                std::string playtime =
                    games[idx].playtime > 0 ?
                        std::to_string(games[idx].playtime) + " min" :
                        "Not played";
                print(
                    fg(color::white),
                    "{:<10} {:<{}} {:<15}\n",
                    games[idx].appid,
                    name,
                    name_width,
                    playtime);
        }
        print(fg(color::light_green), "\nTotal games: {}\n", games.size());
}

void show_help()
{
        if (has_fetched) {
                print(
                    fg(color::cyan) | emphasis::bold,
                    "\n"
                    "Current Account:\n");
                print(fg(color::white), "Username: {}\n", user.username);
                print(fg(color::white), "Location: {}\n", user.location);
                print(fg(color::white), "SteamID: {}\n", user.steamid);
                print(fg(color::white), "Link to Profile: ");
                print(
                    fg(color::cyan),
                    "https://steamcommunity.com/profiles/{}/\n",
                    user.steamid);
                print(fg(color::cyan), "╾━━━━━━━━╼\n");
        }
        print(fg(color::cyan) | emphasis::bold, "Commands:\n");
        print(
            fg(color::white),
            "  fetch <steamid>           "
            "- Fetch games by SteamID64 or Unique ID\n"
            "  search \"<name>\"           "
            "- Search for games starting with name (use quotes)\n"
            "  count_played              "
            "- Count games with playtime\n"
            "  list                      "
            "- Show names in alphabetical order\n"
            "  list -l                   "
            "- Show AppID, name, playtime\n"
            "  list -n                   "
            "- Show name and AppID, grouped by first letter\n"
            "  list -p                   "
            "- Show AppID, name, playtime, sorted by playtime\n"
            "  export <filename>         "
            "- Export games to exported/filename.csv\n"
            "  help                      "
            "- Show this help\n"
            "  exit                      "
            "- Exit the program\n");
        print(fg(color::cyan), "╾━━━━━━━━╼\n");
        print(fg(color::yellow), "Notes:\n");
        print(fg(color::yellow), "- Steam profile must be public.\n");
        print(
            fg(color::yellow),
            "- SteamID is a 17-digit SteamID64 or Unique ID.\n\n");
}

std::vector<std::string> parse_command(const std::string& line)
{
        std::vector<std::string> args;
        std::string              current;
        bool                     in_quotes = false;
        for (char c : line) {
                if (c == '"') {
                        in_quotes = !in_quotes;
                        continue;
                }
                if (c == ' ' && !in_quotes && !current.empty()) {
                        args.push_back(current);
                        current.clear();
                } else if (c != ' ' || in_quotes) {
                        current += c;
                }
        }
        if (!current.empty()) {
                args.push_back(current);
        }
        return args;
}

void process_command(const std::vector<std::string>& args)
{
        if (args.empty()) {
                print(
                    fg(color::indian_red),
                    "Error: No command. Type 'help' for commands.\n");
                return;
        }
        if (args[0] == "help") {
                show_help();
        } else if (args[0] == "fetch" && args.size() == 2) {
                if (!fetch_games(args[1])) {
                        print(
                            fg(color::indian_red),
                            "Error: Failed to fetch games.\n");
                }
        } else if (args[0] == "search" && args.size() == 2) {
                if (args[1].empty()) {
                        print(
                            fg(color::indian_red),
                            "Error: Search term cannot be empty.\n");
                        return;
                }
                search_prefix(args[1]);
        } else if (args[0] == "count_played" && args.size() == 1) {
                count_played();
        } else if (args[0] == "list") {
                if (args.size() == 1) {
                        list_games();
                } else if (args.size() == 2 && args[1] == "-l") {
                        list_table();
                } else if (args.size() == 2 && args[1] == "-n") {
                        list_by_letter();
                } else if (args.size() == 2 && args[1] == "-p") {
                        list_by_playtime();
                } else {
                        print(
                            fg(color::indian_red),
                            "Invalid list option. Type 'help' for commands.\n");
                }
        } else if (args[0] == "export" && args.size() == 2) {
                if (args[1].empty()) {
                        print(
                            fg(color::indian_red),
                            "Error: Filename cannot be empty.\n");
                        return;
                }
                export_csv(args[1]);
        } else if (args[0] == "exit") {
                throw std::runtime_error("exit");
        } else {
                print(
                    fg(color::indian_red),
                    "Invalid command. Type 'help' for commands.\n");
        }
}