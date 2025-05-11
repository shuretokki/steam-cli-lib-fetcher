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

/** @brief Daftar nama game disimpan */
extern std::vector<std::string> SLF_game_names;

/** @brief Daftar ID Steam game */
extern std::vector<int> SLF_game_appids;

/** @brief Daftar playtime game dalam menit */
extern std::vector<int> SLF_game_playtimes;

/** @brief Nama Steam User Account */
extern std::string SLF_account_username;

/** @brief Lokasi Steam User Account */
extern std::string SLF_account_location;

/** @brief SteamID user */
extern std::string SLF_account_steamid;

/** @brief Nama file untuk menyimpan data game */
extern std::string SLF_filename;

/** @brief API Key Steam */
extern std::string SLF_api_key;

/** @brief Status mode verbose (true : aktif) */
extern bool SLF_verbose;

/** @brief Status apakah data sudah diambil (true : sudah) */
extern bool SLF_has_fetched;

// --- BST ---

/** @brief Struktur node untuk BST */
struct BSTNode
{
        std::string name;  /**< Nama game */
        int         appid; /**< APPID game */
        BSTNode*    left;  /**< Pointer kiri */
        BSTNode*    right; /**< Pointer kanan */
        size_t      index; /**< Indeks di vektor SLF_game_names */

        BSTNode(const std::string& n, int id, size_t idx)
            : name(n), appid(id), left(nullptr), right(nullptr), index(idx)
        {
        }
};

/** @brief Kelas untuk me-manage Binary Search Tree game */
class GameBST
{
      private:
        BSTNode* root;

        /** @brief Clean BST dengan rekursif */
        void clear(BSTNode* node);

        /** @brief Inpuy node baru ke BST dengan rekursif */
        BSTNode* insert(
            BSTNode*           node,
            const std::string& name,
            int                appid,
            size_t             index);

        /** @brief Print BST secara in-order */
        void inOrder(BSTNode* node, size_t name_width) const;

      public:
        GameBST() : root(nullptr)
        {
        }
        ~GameBST();

        /** @brief Inpuyt game ke BST */
        void insert(const std::string& name, int appid, size_t index);

        /** @brief Print semua game dengan urut */
        void displayAll(size_t name_width) const;
};

/** @brief Binary Search Tree untuk menyimpan game */
extern GameBST SLF_game_bst;

/** @brief Struktur node tree */
struct TreeNode
{
        std::unordered_map<char, TreeNode*> children; /**< Anak-anak node trie
                                                       */
        std::vector<size_t> indices; /**< Indeks game dengan prefiks ini */
        TreeNode()
        {
        }
};

/** @brief Kelas untuk me-manage game Tree */
class GameT
{
      private:
        TreeNode* root;

        /** @brief Membersihkan Trie secara rekursif */
        void clear(TreeNode* node);

        /** @brief Mengumpulkan semua indeks di bawah node */
        void collectIndices(TreeNode* node, std::vector<size_t>& indices) const;

      public:
        GameT() : root(new TreeNode())
        {
        }
        ~GameT();

        /** @brief Memasukkan nama game ke Trie */
        void insert(const std::string& name, size_t index);

        /** @brief Mencari semua indeks game dengan prefiks tertentu */
        std::vector<size_t> searchPrefix(const std::string& prefix) const;
};

/** @brief Trie untuk pencarian prefiks game */
extern GameT SLF_game_tree;

/** @brief Hash table untuk mencari game berdasarkan nama */
extern std::unordered_map<std::string, size_t> SLF_game_hash;

/** @brief Mengubah string menjadi huruf kecil
 *  @param s String yang diubah
 *  @return String dalam huruf kecil
 */
std::string SLF_to_lower(const std::string& s);

/** @brief Save data game ke file */
void SLF_save_to_file();

/** @brief Load data game dari file */
void SLF_load_from_file();

/** @brief Mengambil data game dari Steam berdasarkan ID
 *  @param input_id SteamID atau ID custom user
 *  @return True jika berhasil, False jika gagal
 */
bool SLF_fetch_games(const std::string& input_id);

/** @brief Mencari game berdasarkan awalan nama
 *  @param prefix Awalan nama game yang dicari
 */
void SLF_search_prefix(const std::string& prefix);

/** @brief Mencari game berdasarkan nama lengkap
 *  @param name Nama lengkap game yang dicari
 */
void SLF_lookup_exact(const std::string& name);

/** @brief Mengekspor daftar game ke file CSV
 *  @param csv_filename Nama file CSV tujuan
 */
void SLF_export_to_csv(const std::string& csv_filename);

/** @brief Menampilkan semua game, dikelompokkan berdasarkan huruf awal */
void SLF_display_all();

/** @brief Menampilkan daftar game dalam bentuk tabel */
void SLF_display_table();

/** @brief Menampilkan nama dan AppID game, dikelompokkan berdasarkan huruf awal
 */
void SLF_display_number_name();

/** @brief Menampilkan daftar game, diurutkan berdasarkan waktu bermain */
void SLF_display_by_playtime();

/** @brief Menampilkan bantuan command */
void SLF_print_help();

/** @brief Memparse command dari user input
 *  @param line Input command dari user
 *  @return Vektor string berisi command args
 */
std::vector<std::string> SLF_parse_command(const std::string& line);

/** @brief Memproses command yang diberikan user
 *  @param args Vektor command args
 */
void SLF_process_command(const std::vector<std::string>& args);