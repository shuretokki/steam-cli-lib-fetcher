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

/** @brief Nama file untuk menyimpan data game. */
extern std::string data_filename;

/** @brief Status apakah data game telah diambil. */
extern bool has_fetched;

/** @brief Kunci API untuk mengakses Steam API. */
extern std::string api_key;

/** @brief Struktur untuk menyimpan data game. */
struct Game;

/** @brief Struktur untuk menyimpan data pengguna. */
struct User;

/** @brief Vektor yang menyimpan daftar game. */
extern std::vector<Game> games;

/** @brief Objek yang menyimpan informasi pengguna. */
extern User user;

/** @brief Struktur untuk menyimpan prefiks nama game dalam struktur pohon.
 *
 * Digunakan untuk pencarian cepat berdasarkan prefiks nama game.
 */
struct Prefix
{
        struct Node
        {
                std::unordered_map<char, Node*> children;
                std::vector<size_t>             ix;
        };
        Node* root;
        Prefix() : root(new Node())
        {
        }
        ~Prefix();

        /** @brief Menyisipkan nama game ke dalam struktur pohon.
         * @param name Nama game yang akan disisipkan.
         * @param index Indeks game dalam vektor games.
         */
        void insert(const std::string& name, size_t index);

        /** @brief Mencari game berdasarkan prefiks nama.
         * @param prefix Prefiks nama game yang dicari.
         * @return Vektor indeks game yang cocok dengan prefiks.
         */
        std::vector<size_t> search(const std::string& prefix) const;

      private:
        /** @brief Membersihkan memori node dalam struktur pohon.
         * @param node Pointer ke node yang akan dihapus.
         */
        void clear(Node* node);

        /** @brief Mengumpulkan indeks game dari node dan anak-anaknya.
         * @param node Pointer ke node yang sedang diproses.
         * @param indices Vektor untuk menyimpan indeks yang dikumpulkan.
         */
        void collect_indices(Node* node, std::vector<size_t>& indices) const;
};

/** @brief Objek pohon prefiks untuk pencarian nama game. */
extern Prefix games_prefix;

/** @brief Peta yang menyimpan nama game ke indeksnya. */
extern std::unordered_map<std::string, size_t> games_map;

/** @brief Mengubah string menjadi huruf kecil.
 * @param s String yang akan diubah.
 * @return String dalam huruf kecil.
 */
std::string to_lower(const std::string& s);

/** @brief Menyimpan data game ke file JSON. */
void save_games();

/** @brief Memuat data game dari file JSON. */
void load_games();

/** @brief Mengambil data game dari Steam berdasarkan ID.
 * @param id SteamID64 atau ID unik pengguna.
 * @return True jika berhasil, false jika gagal.
 */
bool fetch_games(const std::string& id);

/** @brief Mencari game berdasarkan prefiks nama.
 * @param prefix Prefiks nama game yang dicari.
 */
void search_prefix(const std::string& prefix);

/** @brief Mencari game berdasarkan nama yang tepat.
 * @param name Nama game yang dicari.
 */
void lookup_exact(const std::string& name);

/** @brief Menghitung jumlah game yang pernah dimainkan. */
void count_played();

/** @brief Mengekspor data game ke file CSV.
 * @param filename Nama file CSV tanpa ekstensi.
 */
void export_csv(const std::string& filename);

/** @brief Menampilkan daftar nama game secara alfabetis. */
void list_games();

/** @brief Menampilkan tabel dengan AppID, nama, dan waktu bermain. */
void list_table();

/** @brief Menampilkan daftar game dikelompokkan berdasarkan huruf awal. */
void list_by_letter();

/** @brief Menampilkan daftar game diurutkan berdasarkan waktu bermain. */
void list_by_playtime();

/** @brief Menampilkan panduan penggunaan perintah. */
void show_help();

/** @brief Memparsing perintah dari input pengguna.
 * @param line String input perintah.
 * @return Vektor string berisi argumen perintah.
 */
std::vector<std::string> parse_command(const std::string& line);

/** @brief Memproses perintah dari pengguna.
 * @param args Vektor argumen perintah.
 */
void process_command(const std::vector<std::string>& args);