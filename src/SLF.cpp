#include "slf.hpp"

using namespace fmt;
using json = nlohmann::json;

std::vector<std::string>                SLF_game_names;
std::vector<int>                        SLF_game_appids;
std::vector<int>                        SLF_game_playtimes;
std::string                             SLF_api_key;
std::string                             SLF_account_username = "";
std::string                             SLF_account_location = "";
std::string                             SLF_account_steamid  = "";
std::string                             SLF_filename    = "data/fetch.json";
bool                                    SLF_verbose     = false;
bool                                    SLF_has_fetched = false;
GameBST                                 SLF_game_bst;
GameT                                   SLF_game_tree;
std::unordered_map<std::string, size_t> SLF_game_hash;

/** @brief Mengubah string menjadi huruf kecil
 *  @param s String yang diubah
 *  @return String dalam huruf kecil
 */
std::string SLF_to_lower(const std::string& s)
{
        std::string lower = s;
        for (size_t i = 0; i < lower.length(); ++i) {
                lower[i] = std::tolower(static_cast<unsigned char>(lower[i]));
        }
        return lower;
}

// --- BST ---
/** @brief Clean BST scr rekursif */
void GameBST::clear(BSTNode* node)
{
        if (!node)
                return;
        clear(node->left);
        clear(node->right);
        delete node;
}

/** @brief Input node baru ke BST scr rekursif */
BSTNode* GameBST::insert(
    BSTNode*           node,
    const std::string& name,
    int                appid,
    size_t             index)
{
        if (!node) {
                return new BSTNode(name, appid, index);
        }
        std::string lower_name      = SLF_to_lower(name);
        std::string lower_node_name = SLF_to_lower(node->name);
        if (lower_name < lower_node_name) {
                node->left = insert(node->left, name, appid, index);
        } else {
                node->right = insert(node->right, name, appid, index);
        }
        return node;
}

/** @brief Print BST in-order */
void GameBST::inOrder(BSTNode* node, size_t name_width) const
{
        if (!node)
                return;
        inOrder(node->left, name_width);
        std::string display_name = SLF_game_names[node->index];
        if (display_name.length() > name_width) {
                display_name = display_name.substr(0, name_width - 3) + "...";
        }
        print(fg(color::white), "{:<{}}\n", display_name, name_width);
        inOrder(node->right, name_width);
}

/** @brief Destruktor */
GameBST::~GameBST()
{
        clear(root);
}

/** @brief Input game ke BST */
void GameBST::insert(const std::string& name, int appid, size_t index)
{
        root = insert(root, name, appid, index);
}

/** @brief Print semua game scr urut */
void GameBST::displayAll(size_t name_width) const
{
        inOrder(root, name_width);
}

/** @brief Clear Tree scr rekursif */
void GameT::clear(TreeNode* node)
{
        if (!node)
                return;
        for (auto& child : node->children) {
                clear(child.second);
        }
        delete node;
}

/** @brief Mengumpulkan sluruh indeks di bawah node */
void GameT::collectIndices(TreeNode* node, std::vector<size_t>& indices) const
{
        if (!node)
                return;
        indices.insert(
            indices.end(),
            node->indices.begin(),
            node->indices.end());
        for (const auto& child : node->children) {
                collectIndices(child.second, indices);
        }
}

/** @brief Destruktor */
GameT::~GameT()
{
        clear(root);
}

/** @brief Input nama game ke tree */
void GameT::insert(const std::string& name, size_t index)
{
        TreeNode*   current    = root;
        std::string lower_name = SLF_to_lower(name);
        for (char c : lower_name) {
                if (current->children.find(c) == current->children.end()) {
                        current->children[c] = new TreeNode();
                }
                current = current->children[c];
        }
        current->indices.push_back(index);
}

/** @brief Search semua indeks game dengan prefiks tertentu */
std::vector<size_t> GameT::searchPrefix(const std::string& prefix) const
{
        std::vector<size_t> indices;
        TreeNode*           current      = root;
        std::string         lower_prefix = SLF_to_lower(prefix);
        for (char c : lower_prefix) {
                if (current->children.find(c) == current->children.end()) {
                        return indices; // Prefiks tidak ditemukan
                }
                current = current->children[c];
        }
        collectIndices(current, indices);
        return indices;
}

/** @brief Save data game ke file */
void SLF_save_to_file()
{
        std::filesystem::create_directories(
            std::filesystem::path(SLF_filename).parent_path());
        std::ofstream ofs(SLF_filename);
        if (!ofs.is_open()) {
                print(
                    fg(color::indian_red),
                    "Error: Failed to write to {}. Check permissions.\n",
                    SLF_filename);
                return;
        }

        json j;
        j["account"]["username"] = SLF_account_username;
        j["account"]["location"] = SLF_account_location;
        j["account"]["steamid"]  = SLF_account_steamid;
        for (size_t i = 0; i < SLF_game_names.size(); ++i) {
                json game;
                game["name"]             = SLF_game_names[i];
                game["appid"]            = SLF_game_appids[i];
                game["playtime_forever"] = SLF_game_playtimes[i];
                j["games"].push_back(game);
        }
        ofs << j.dump(4);
        if (SLF_verbose) {
                print(
                    fg(color::yellow),
                    "Saved game data to {}.\n",
                    SLF_filename);
        }
}

/** @brief Load API key atau buat config.json */
bool SLF_load_api_key()
{
        std::string   config_filename = "data/config.json";
        std::ifstream ifs(config_filename);
        if (!ifs.is_open()) {
                print(
                    fg(color::yellow),
                    "No API key configuration file found at {}. Please enter "
                    "your Steam API key:\n",
                    config_filename);
                std::string user_api_key;
                std::getline(std::cin, user_api_key);
                if (user_api_key.empty()) {
                        print(
                            fg(color::indian_red),
                            "Error: API key cannot be empty. Exiting.\n");
                        return false;
                }
                std::filesystem::create_directories("data");
                std::ofstream ofs(config_filename);
                if (!ofs.is_open()) {
                        print(
                            fg(color::indian_red),
                            "Error: Failed to create {}. Check permissions.\n",
                            config_filename);
                        return false;
                }
                json j;
                j["api_key"] = user_api_key;
                ofs << j.dump(4);
                SLF_api_key = user_api_key;
                if (SLF_verbose) {
                        print(
                            fg(color::yellow),
                            "Created {} with your API key.\n",
                            config_filename);
                }
        } else {
                try {
                        json j;
                        ifs >> j;
                        if (!j.contains("api_key")
                            || !j["api_key"].is_string()) {
                                print(
                                    fg(color::indian_red),
                                    "Error: Invalid format in {}. Expected an "
                                    "'api_key' string.\n",
                                    config_filename);
                                return false;
                        }
                        SLF_api_key = j["api_key"].get<std::string>();
                        if (SLF_api_key.empty()) {
                                print(
                                    fg(color::indian_red),
                                    "Error: API key in {} is empty.\n",
                                    config_filename);
                                return false;
                        }
                        if (SLF_verbose) {
                                print(
                                    fg(color::yellow),
                                    "Loaded API key from {}.\n",
                                    config_filename);
                        }
                } catch (const std::exception& e) {
                        print(
                            fg(color::indian_red),
                            "Error: Failed to parse {}: {}.\n",
                            config_filename,
                            e.what());
                        return false;
                }
        }
        return true;
}

/** @brief Load data game dari file */
void SLF_load_from_file()
{
        if (!SLF_load_api_key()) {
                print(
                    fg(color::indian_red),
                    "Error: Cannot proceed without a valid API key. "
                    "Exiting.\n");
                return;
        }
        std::ifstream ifs(SLF_filename);
        if (!ifs.is_open()) {
                if (SLF_verbose) {
                        print(
                            fg(color::yellow),
                            "No existing game data at {}. Use 'fetch' to load "
                            "games.\n",
                            SLF_filename);
                }
                return;
        }

        try {
                json j;
                ifs >> j;
                if (j.contains("account")) {
                        SLF_account_username =
                            j["account"]["username"].get<std::string>();
                        SLF_account_location =
                            j["account"]["location"].get<std::string>();
                        SLF_account_steamid =
                            j["account"]["steamid"].get<std::string>();
                        SLF_has_fetched = true;
                }
                if (j.contains("games")) {
                        SLF_game_names.clear();
                        SLF_game_appids.clear();
                        SLF_game_playtimes.clear();
                        SLF_game_bst  = GameBST(); // Reset BST
                        SLF_game_tree = GameT();   // Reset Tree
                        SLF_game_hash.clear();     // Reset Hash Table
                        for (size_t i = 0; i < j["games"].size(); ++i) {
                                std::string name =
                                    j["games"][i]["name"].get<std::string>();
                                int appid = j["games"][i]["appid"].get<int>();
                                int playtime =
                                    j["games"][i].contains("playtime_forever") ?
                                        j["games"][i]["playtime_forever"]
                                            .get<int>() :
                                        0;
                                SLF_game_names.push_back(name);
                                SLF_game_appids.push_back(appid);
                                SLF_game_playtimes.push_back(playtime);
                                SLF_game_bst.insert(name, appid, i);
                                SLF_game_tree.insert(name, i);
                                SLF_game_hash[SLF_to_lower(name)] = i;
                        }
                }
                if (SLF_verbose) {
                        print(
                            fg(color::yellow),
                            "Loaded {} games from {}.\n",
                            SLF_game_names.size(),
                            SLF_filename);
                }
        } catch (const std::exception& e) {
                print(
                    fg(color::indian_red),
                    "Error: Failed to parse {}: {}.\n",
                    SLF_filename,
                    e.what());
        }
}

/** @brief Mengambil data game dari Steam dari ID
 *  @param input_id SteamID atau Unique ID
 *  @return True jika berhasil, False jika gagal
 */
bool SLF_fetch_games(const std::string& input_id)
{
        std::string steamid;
        if (std::regex_match(input_id, std::regex("\\d{17}"))) {
                steamid = input_id;
        } else {
                std::string custom_id = input_id;
                std::regex  url_regex("https?://steamcommunity.com/id/([^/]+)");
                std::smatch match;
                if (std::regex_match(input_id, match, url_regex)) {
                        custom_id = match[1].str();
                }
                httplib::Client cli("http://api.steampowered.com");
                cli.set_connection_timeout(5, 0);
                cli.set_read_timeout(5, 0);
                std::string url = fmt::format(
                    "/ISteamUser/ResolveVanityURL/v0001/?key={}&vanityurl={}",
                    SLF_api_key,
                    custom_id);
                if (SLF_verbose) {
                        print(
                            fg(color::yellow),
                            "Resolving vanity URL: {}\n",
                            url);
                }
                auto res = cli.Get(url.c_str());
                if (!res) {
                        print(
                            fg(color::indian_red),
                            "Error: Failed to resolve Steam ID. Check "
                            "network.\n");
                        return false;
                }
                if (res->status != 200) {
                        print(
                            fg(color::indian_red),
                            "Error: Resolve API returned status {}.\n",
                            res->status);
                        return false;
                }
                try {
                        json j = json::parse(res->body);
                        if (j["response"]["success"] != 1) {
                                print(
                                    fg(color::indian_red),
                                    "Error: Could not resolve custom ID '{}'. "
                                    "Ensure it exists.\n",
                                    custom_id);
                                return false;
                        }
                        steamid = j["response"]["steamid"].get<std::string>();
                } catch (const std::exception& e) {
                        print(
                            fg(color::indian_red),
                            "Error: Failed to parse Resolve API response: "
                            "{}.\n",
                            e.what());
                        return false;
                }
        }

        httplib::Client cli("http://api.steampowered.com");
        cli.set_connection_timeout(5, 0);
        cli.set_read_timeout(5, 0);
        std::string summary_url = fmt::format(
            "/ISteamUser/GetPlayerSummaries/v0002/?key={}&steamids={}",
            SLF_api_key,
            steamid);
        if (SLF_verbose) {
                print(
                    fg(color::yellow),
                    "Fetching user summary: {}\n",
                    summary_url);
        }
        auto summary_res = cli.Get(summary_url.c_str());
        if (!summary_res || summary_res->status != 200) {
                print(
                    fg(color::indian_red),
                    "Error: Failed to fetch user summary (status {}). Profile "
                    "may be private.\n",
                    summary_res ? summary_res->status : -1);
                return false;
        }

        std::string new_username, new_location, new_steamid;
        try {
                json summary_j = json::parse(summary_res->body);
                auto player    = summary_j["response"]["players"][0];
                new_steamid    = steamid;
                new_username   = player["personaname"].get<std::string>();
                new_location =
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
                    "Error: Failed to parse user summary: {}.\n",
                    e.what());
                return false;
        }

        std::string games_url = fmt::format(
            "/IPlayerService/GetOwnedGames/v0001/"
            "?key={}&steamid={}&format=json&include_appinfo=1",
            SLF_api_key,
            steamid);
        if (SLF_verbose) {
                print(fg(color::yellow), "Fetching games: {}\n", games_url);
        }
        auto games_res = cli.Get(games_url.c_str());
        if (!games_res) {
                print(
                    fg(color::indian_red),
                    "Error: Failed to connect to Steam API. Check network.\n");
                return false;
        }
        if (games_res->status != 200) {
                print(
                    fg(color::indian_red),
                    "Error: Steam API returned status {}.\n",
                    games_res->status);
                if (games_res->status == 403) {
                        print(
                            fg(color::yellow),
                            "Possible cause: Invalid API key.\n");
                } else if (games_res->status == 401) {
                        print(
                            fg(color::yellow),
                            "Possible cause: Private profile or invalid Steam "
                            "ID.\n");
                }
                print(
                    fg(color::yellow),
                    "Check profile: https://steamcommunity.com/profiles/{}\n",
                    steamid);
                return false;
        }

        try {
                json games_j = json::parse(games_res->body);
                if (!games_j.contains("response")
                    || !games_j["response"].contains("games")) {
                        print(
                            fg(color::indian_red),
                            "Error: No games found. Profile may be private.\n");
                        print(
                            fg(color::yellow),
                            "Check profile: "
                            "https://steamcommunity.com/profiles/{}\n",
                            steamid);
                        return false;
                }
                auto games = games_j["response"]["games"];
                if (games.empty()) {
                        print(
                            fg(color::yellow),
                            "Warning: Steam profile has no games or is "
                            "private.\n");
                        print(
                            fg(color::yellow),
                            "Check profile: "
                            "https://steamcommunity.com/profiles/{}\n",
                            steamid);
                        return false;
                }

                SLF_game_names.clear();
                SLF_game_appids.clear();
                SLF_game_playtimes.clear();
                SLF_game_bst  = GameBST(); // Reset BST
                SLF_game_tree = GameT();   // Reset Tree
                SLF_game_hash.clear();     // Reset hash table
                SLF_account_username = new_username;
                SLF_account_location = new_location;
                SLF_account_steamid  = new_steamid;
                SLF_has_fetched      = true;

                for (size_t i = 0; i < games.size(); ++i) {
                        std::string name  = games[i]["name"].get<std::string>();
                        int         appid = games[i]["appid"].get<int>();
                        int         playtime =
                            games[i].contains("playtime_forever") ?
                                        games[i]["playtime_forever"].get<int>() :
                                        0;
                        SLF_game_names.push_back(name);
                        SLF_game_appids.push_back(appid);
                        SLF_game_playtimes.push_back(playtime);
                        SLF_game_bst.insert(name, appid, i); // Masuk ke BST
                        SLF_game_tree.insert(name, i);       // Masuk ke Tree
                        SLF_game_hash[SLF_to_lower(name)] =
                            i; // Masuk ke hash table
                }

                SLF_save_to_file();
                print(
                    fg(color::light_green),
                    "Fetched {} games successfully.\n",
                    games.size());
                print(
                    fg(color::yellow),
                    "Profile: https://steamcommunity.com/profiles/{}\n",
                    steamid);
                return true;
        } catch (const std::exception& e) {
                print(
                    fg(color::indian_red),
                    "Error: Failed to parse Steam API response: {}.\n",
                    e.what());
                return false;
        }
}

/** @brief Mencari game dari awalan nama menggunakan Tree
 *  @param prefix Awalan nama game yang dicari
 */
void SLF_search_prefix(const std::string& prefix)
{
        std::vector<size_t> indices = SLF_game_tree.searchPrefix(prefix);

        if (indices.empty()) {
                print(fg(color::indian_red), "No games found.\n");
                return;
        }

        size_t name_width = 0;
        for (size_t i = 0; i < SLF_game_names.size(); ++i) {
                if (SLF_game_names[i].length() > name_width) {
                        name_width = SLF_game_names[i].length();
                }
        }
        name_width = std::min(name_width + 4, size_t(34));

        print(fg(color::light_green) | emphasis::bold, "\nMatching games:\n");
        for (size_t idx : indices) {
                std::string display_name = SLF_game_names[idx];
                if (display_name.length() > name_width) {
                        display_name =
                            display_name.substr(0, name_width - 3) + "...";
                }
                std::string playtime_str =
                    SLF_game_playtimes[idx] > 0 ?
                        std::to_string(SLF_game_playtimes[idx]) + " min" :
                        "Not played";
                print(
                    fg(color::white),
                    "{:<10} {:<{}} {:<15}\n",
                    SLF_game_appids[idx],
                    display_name,
                    name_width,
                    playtime_str);
        }
        print(
            fg(color::light_green),
            "\nTotal matches: {}\n\n",
            indices.size());
}

/** @brief Mencari game dari nama lengkap menggunakan hash table
 *  @param name Nama lengkap game yang dicari
 */
void SLF_lookup_exact(const std::string& name)
{
        std::string lower_name = SLF_to_lower(name);
        auto        it         = SLF_game_hash.find(lower_name);
        if (it == SLF_game_hash.end()) {
                print(fg(color::indian_red), "Game '{}' not found.\n", name);
                return;
        }

        size_t idx        = it->second;
        size_t name_width = 0;
        for (size_t i = 0; i < SLF_game_names.size(); ++i) {
                if (SLF_game_names[i].length() > name_width) {
                        name_width = SLF_game_names[i].length();
                }
        }
        name_width = std::min(name_width + 4, size_t(34));

        print(fg(color::light_green) | emphasis::bold, "Matching game:\n");
        std::string display_name = SLF_game_names[idx];
        if (display_name.length() > name_width) {
                display_name = display_name.substr(0, name_width - 3) + "...";
        }
        std::string playtime_str =
            SLF_game_playtimes[idx] > 0 ?
                std::to_string(SLF_game_playtimes[idx]) + " min" :
                "Not played";
        print(
            fg(color::white),
            "{:<10} {:<{}} {:<15}\n",
            SLF_game_appids[idx],
            display_name,
            name_width,
            playtime_str);
}

/** @brief Mengekspor daftar game ke file CSV
 *  @param csv_filename Nama file CSV tujuan
 */
void SLF_export_to_csv(const std::string& csv_filename)
{
        if (SLF_game_names.empty()) {
                print(
                    fg(color::indian_red),
                    "No games to export. Try 'fetch <steamid>' first.\n");
                return;
        }

        std::string export_path = "data/exported/" + csv_filename + ".csv";
        std::filesystem::create_directories(
            std::filesystem::path(export_path).parent_path());
        std::ofstream ofs(export_path);
        if (!ofs.is_open()) {
                print(
                    fg(color::indian_red),
                    "Error: Failed to write to {}. Check permissions.\n",
                    export_path);
                return;
        }

        size_t name_width     = 0;
        size_t playtime_width = 15;
        for (size_t i = 0; i < SLF_game_names.size(); ++i) {
                if (SLF_game_names[i].length() > name_width) {
                        name_width = SLF_game_names[i].length();
                }
                std::string playtime_str =
                    SLF_game_playtimes[i] > 0 ?
                        std::to_string(SLF_game_playtimes[i]) + " min" :
                        "Not played";
                if (playtime_str.length() > playtime_width) {
                        playtime_width = playtime_str.length();
                }
        }
        name_width = std::min(name_width, size_t(30));

        ofs << std::left << std::setw(10) << "AppID" << ","
            << std::setw(name_width) << "Name" << ","
            << std::setw(playtime_width) << "Playtime" << "\n";

        for (size_t i = 0; i < SLF_game_names.size(); ++i) {
                std::string name = SLF_game_names[i];
                if (name.length() > name_width) {
                        name = name.substr(0, name_width - 3) + "...";
                }
                std::replace(name.begin(), name.end(), '"', '\'');
                if (name.find(',') != std::string::npos) {
                        name = "\"" + name + "\"";
                }
                std::string playtime_str =
                    SLF_game_playtimes[i] > 0 ?
                        std::to_string(SLF_game_playtimes[i]) + " min" :
                        "Not played";
                ofs << std::left << std::setw(10) << SLF_game_appids[i] << ","
                    << std::setw(name_width) << name << ","
                    << std::setw(playtime_width) << playtime_str << "\n";
        }

        print(
            fg(color::light_green),
            "Exported {} games to {}.\n",
            SLF_game_names.size(),
            export_path);
}

/** @brief Print semua game menggunakan BST
 *  @command: 'list'
 */
void SLF_display_all()
{
        if (SLF_game_names.empty()) {
                print(
                    fg(color::indian_red),
                    "No games found. Try 'fetch <steamid>' first.\n");
                return;
        }

        size_t name_width = 0;
        for (size_t i = 0; i < SLF_game_names.size(); ++i) {
                if (SLF_game_names[i].length() > name_width) {
                        name_width = SLF_game_names[i].length();
                }
        }
        name_width = std::min(name_width + 4, size_t(34));

        print(fg(color::gold) | emphasis::bold, "Games:\n");
        SLF_game_bst.displayAll(name_width);
        print(
            fg(color::light_green),
            "\nTotal games: {}\n\n",
            SLF_game_names.size());
}

/** @brief Print daftar game dalam bentuk tabel
 *  @command: 'list -l'
 */
void SLF_display_table()
{
        if (SLF_game_names.empty()) {
                print(
                    fg(color::indian_red),
                    "No games found. Try 'fetch <steamid>' first.\n");
                return;
        }

        size_t name_width = 0;
        for (size_t i = 0; i < SLF_game_names.size(); ++i) {
                if (SLF_game_names[i].length() > name_width) {
                        name_width = SLF_game_names[i].length();
                }
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
        for (size_t i = 0; i < SLF_game_names.size(); ++i) {
                std::string display_name = SLF_game_names[i];
                if (display_name.length() > name_width) {
                        display_name =
                            display_name.substr(0, name_width - 3) + "...";
                }
                std::string playtime_str =
                    SLF_game_playtimes[i] > 0 ?
                        std::to_string(SLF_game_playtimes[i]) + " min" :
                        "Not played";
                print(
                    fg(color::white),
                    "{:<10} {:<{}} {:<15}\n",
                    SLF_game_appids[i],
                    display_name,
                    name_width,
                    playtime_str);
        }
        print(
            fg(color::light_green),
            "\nTotal games: {}\n\n",
            SLF_game_names.size());
}

/** @brief Print nama & AppID game, dikelompokkan dari huruf awal,
 *  @command: 'list -n'
 */
void SLF_display_number_name()
{
        if (SLF_game_names.empty()) {
                print(
                    fg(color::indian_red),
                    "No games found. Try 'fetch <steamid>' first.\n");
                return;
        }

        std::vector<std::vector<size_t>> digits;
        std::vector<std::vector<size_t>> letters(26);
        for (size_t i = 0; i < SLF_game_names.size(); ++i) {
                char first =
                    SLF_game_names[i].empty() ? ' ' : SLF_game_names[i][0];
                if (std::isdigit(first)) {
                        digits.push_back({ i });
                } else if (std::isalpha(first)) {
                        int index = std::tolower(first) - 'a';
                        if (index >= 0 && index < 26) {
                                letters[index].push_back(i);
                        }
                }
        }

        size_t name_width = 0;
        for (size_t i = 0; i < SLF_game_names.size(); ++i) {
                if (SLF_game_names[i].length() > name_width) {
                        name_width = SLF_game_names[i].length();
                }
        }
        name_width = std::min(name_width + 4, size_t(34));

        if (!digits.empty()) {
                print(fg(color::gold) | emphasis::bold, "0-9\n");
                for (size_t i = 0; i < digits.size(); ++i) {
                        size_t      idx          = digits[i][0];
                        std::string display_name = SLF_game_names[idx];
                        if (display_name.length() > name_width) {
                                display_name =
                                    display_name.substr(0, name_width - 3)
                                    + "...";
                        }
                        print(
                            fg(color::white),
                            "{:<{}} {:<10}\n",
                            display_name,
                            name_width,
                            SLF_game_appids[idx]);
                }
        }

        for (int i = 0; i < 26; ++i) {
                if (!letters[i].empty()) {
                        if (!digits.empty() || i > 0) {
                                print("\n");
                        }
                        print(
                            fg(color::gold) | emphasis::bold,
                            "{}\n",
                            static_cast<char>('A' + i));
                        for (size_t j = 0; j < letters[i].size(); ++j) {
                                size_t      idx          = letters[i][j];
                                std::string display_name = SLF_game_names[idx];
                                if (display_name.length() > name_width) {
                                        display_name = display_name.substr(
                                                           0,
                                                           name_width - 3)
                                                       + "...";
                                }
                                print(
                                    fg(color::white),
                                    "{:<{}} {:<10}\n",
                                    display_name,
                                    name_width,
                                    SLF_game_appids[idx]);
                        }
                }
        }
        print(
            fg(color::light_green),
            "\nTotal games: {}\n\n",
            SLF_game_names.size());
}

/** @brief Print daftar game, urut dari playtime,
 *  @command: 'list -p'
 */
void SLF_display_by_playtime()
{
        if (SLF_game_names.empty()) {
                print(
                    fg(color::indian_red),
                    "No games found. Try 'fetch <steamid>' first.\n");
                return;
        }

        std::vector<size_t> indices(SLF_game_names.size());
        for (size_t i = 0; i < indices.size(); ++i) {
                indices[i] = i;
        }
        for (size_t i = 0; i < indices.size(); ++i) {
                for (size_t j = i + 1; j < indices.size(); ++j) {
                        if (SLF_game_playtimes[indices[i]]
                                < SLF_game_playtimes[indices[j]]
                            || (SLF_game_playtimes[indices[i]]
                                    == SLF_game_playtimes[indices[j]]
                                && SLF_to_lower(SLF_game_names[indices[i]])
                                       > SLF_to_lower(
                                           SLF_game_names[indices[j]]))) {
                                std::swap(indices[i], indices[j]);
                        }
                }
        }

        size_t name_width = 0;
        for (size_t i = 0; i < SLF_game_names.size(); ++i) {
                if (SLF_game_names[i].length() > name_width) {
                        name_width = SLF_game_names[i].length();
                }
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
        for (size_t i = 0; i < indices.size(); ++i) {
                size_t      idx          = indices[i];
                std::string display_name = SLF_game_names[idx];
                if (display_name.length() > name_width) {
                        display_name =
                            display_name.substr(0, name_width - 3) + "...";
                }
                std::string playtime_str =
                    SLF_game_playtimes[idx] > 0 ?
                        std::to_string(SLF_game_playtimes[idx]) + " min" :
                        "Not played";
                print(
                    fg(color::white),
                    "{:<10} {:<{}} {:<15}\n",
                    SLF_game_appids[idx],
                    display_name,
                    name_width,
                    playtime_str);
        }
        print(
            fg(color::light_green),
            "\nTotal games: {}\n\n",
            SLF_game_names.size());
}

/** @brief Print helper
 *  @command: 'help'
 */
void SLF_print_help()
{
        if (SLF_has_fetched) {
                print(fg(color::cyan) | emphasis::bold, "\nCurrent Account:\n");
                print(fg(color::white), "Username: {}\n", SLF_account_username);
                print(fg(color::white), "Location: {}\n", SLF_account_location);
                print(fg(color::white), "SteamID:  {}\n", SLF_account_steamid);
                print(fg(color::cyan), "╾━━━━━━━━╼\n");
        }
        print(fg(color::cyan) | emphasis::bold, "Commands:\n");
        print(
            fg(color::white),
            "  fetch <steamid>           - Fetch games by SteamID64 or Unique "
            "ID\n"
            "  search \"<name>\"           - Search for games starting with "
            "name (use quotes)\n"
            "  lookup \"<name>\"           - Look up a game by exact name (use "
            "quotes)\n"
            "  list                      - Show names in alphabetical order\n"
            "  list -l                   - Show AppID, name, playtime\n"
            "  list -n                   - Show name and AppID, grouped by "
            "first letter\n"
            "  list -p                   - Show AppID, name, playtime, sorted "
            "by playtime\n"
            "  export <filename>         - Export games to "
            "exported/filename.csv\n"
            "  verbose on/off            - Toggle verbose logging\n"
            "  help                      - Show this help\n"
            "  exit                      - Exit the program\n");
        print(fg(color::cyan), "╾━━━━━━━━╼\n");
        print(fg(color::yellow), "Notes:\n");
        print(fg(color::yellow), "- Steam profile must be public.\n");
        print(
            fg(color::yellow),
            "- SteamID is a 17-digit SteamID64 (e.g., 76561197960435530) or "
            "Unique ID.\n\n");
}

/** @brief Parsing command dari user input
 *  @param line Input command dari user
 *  @return Vektor string berisi command args
 */
std::vector<std::string> SLF_parse_command(const std::string& line)
{
        std::vector<std::string> args;
        std::string              current;
        bool                     in_quotes = false;

        for (size_t i = 0; i < line.length(); ++i) {
                char c = line[i];
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

/** @brief Memproses command yang diberikan user
 *  @param args Vektor command args
 */
void SLF_process_command(const std::vector<std::string>& args)
{
        if (args.empty()) {
                print(
                    fg(color::indian_red),
                    "Error: No command provided. Type 'help' for commands.\n");
                return;
        }

        if (args[0] == "help") {
                SLF_print_help();
        } else if (args[0] == "fetch" && args.size() == 2) {
                if (!SLF_fetch_games(args[1])) {
                        print(
                            fg(color::indian_red),
                            "Error: Failed to fetch games. Check Steam ID, "
                            "profile privacy, or network.\n");
                }
        } else if (args[0] == "search" && args.size() == 2) {
                if (args[1].empty()) {
                        print(
                            fg(color::indian_red),
                            "Error: Search term cannot be empty.\n");
                        return;
                }
                SLF_search_prefix(args[1]);
        } else if (args[0] == "lookup" && args.size() == 2) {
                if (args[1].empty()) {
                        print(
                            fg(color::indian_red),
                            "Error: Lookup name cannot be empty.\n");
                        return;
                }
                SLF_lookup_exact(args[1]);
        } else if (args[0] == "list") {
                if (args.size() == 1) {
                        SLF_display_all();
                } else if (args.size() == 2 && args[1] == "-l") {
                        SLF_display_table();
                } else if (args.size() == 2 && args[1] == "-n") {
                        SLF_display_number_name();
                } else if (args.size() == 2 && args[1] == "-p") {
                        SLF_display_by_playtime();
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
                SLF_export_to_csv(args[1]);
        } else if (args[0] == "verbose" && args.size() == 2) {
                if (args[1] == "on") {
                        SLF_verbose = true;
                        print(
                            fg(color::light_green),
                            "Verbose mode enabled.\n");
                } else if (args[1] == "off") {
                        SLF_verbose = false;
                        print(
                            fg(color::light_green),
                            "Verbose mode disabled.\n");
                } else {
                        print(
                            fg(color::indian_red),
                            "Error: Use 'verbose on' or 'verbose off'.\n");
                }
        } else if (args[0] == "exit") {
                throw std::runtime_error("exit");
        } else {
                print(
                    fg(color::indian_red),
                    "Invalid command. Type 'help' for commands.\n");
        }
}