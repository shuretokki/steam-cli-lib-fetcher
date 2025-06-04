#include "steam/api_key.hpp"
#include "steam/data.hpp"
#include "steam/utility.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>

using namespace fmt;
using json = nlohmann::json;
STEAM_BEGIN_NAMESPACE

namespace api_key {

bool isSteamAPIKeyValid(const std::string& key)
{
        if (key.empty()) {
                return false;
        }
        httplib::Client client("api.steampowered.com");
        client.set_connection_timeout(5, 0);
        client.set_read_timeout(10, 0);

        std::string endpoint = format("/ISteamWebAPIUtil/GetSupportedAPIList/v1/?key={}", key);
        auto        res      = client.Get(endpoint.c_str());
        if (!res) {
                print(fg(color::indian_red), "Error: Could not connect to Steam API to validate key.\n");
                return false;
        }
        if (res->status == 200) {
                try {
                        auto json_body = json::parse(res->body);
                        if (json_body.contains("apilist") && json_body["apilist"].contains("interfaces")) {

                                return true;
                        } else {
                                print(
                                    fg(color::yellow),
                                    "Warning: API key was accepted but response format unexpected.\n");
                                return true;
                        }
                } catch (const json::exception& e) {
                        print(fg(color::indian_red), "Error: Failed to parse API validation response: {}.\n", e.what());
                        return false;
                }
        } else if (res->status == 403) {
                print(fg(color::indian_red), "Error: Steam API key is invalid (403 Forbidden).\n");
                return false;
        } else {
                print(fg(color::indian_red), "Error: Steam API returned status {} for key validation.\n", res->status);
                return false;
        }
}

bool LoadApiKeyFromEnv()
{

        dotenv::init();
        const char* env_api_key_cstr = std::getenv("STEAM_API_KEY");

        if (env_api_key_cstr != nullptr && std::string(env_api_key_cstr).length() > 0) {
                steam_api_key = env_api_key_cstr;
                if (isSteamAPIKeyValid(steam_api_key)) {
                        return true;
                } else {
                        steam_api_key.clear();
                }
        }

        std::ifstream env_file(".env");
        if (env_file.is_open()) {
                std::string line;
                while (std::getline(env_file, line)) {
                        if (line.rfind("STEAM_API_KEY=", 0) == 0) {
                                std::string potential_key = line.substr(std::string("STEAM_API_KEY=").length());
                                if (!potential_key.empty()) {
                                        if (isSteamAPIKeyValid(potential_key)) {
                                                steam_api_key = potential_key;
                                                env_file.close();
                                                return true;
                                        } else {
                                                print(fg(color::yellow), "STEAM_API_KEY in .env file is invalid.\n");
                                                steam_api_key.clear();
                                                break;
                                        }
                                }
                        }
                }
                env_file.close();
        }

        return !steam_api_key.empty();
}
} // namespace api_key
STEAM_END_NAMESPACE