#include "steam/data.hpp"

#include "steam/utility.hpp" // Includes filesystem

#include <algorithm> // For std::transform
#include <cctype>    // For std::tolower
#include <string>

STEAM_BEGIN_NAMESPACE

std::string ToLower(const std::string& input_string)
{
        std::string result = input_string;
        std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
        });
        return result;
}

std::filesystem::path GetGamesDataPath()
{
        std::filesystem::path data_dir_path = kDataDirectory;
        if (!std::filesystem::exists(data_dir_path)) {
                std::filesystem::create_directories(data_dir_path);
        }
        return data_dir_path / kGamesDataJsonFile;
}
STEAM_END_NAMESPACE