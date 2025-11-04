#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>
#include <map>
#include <set>
#include <functional>

const std::string DATA_FILE = "database.dat";

class FileStorageBPT {
private:
    std::map<std::string, std::set<int>> database;

    void save_to_file() {
        std::ofstream file(DATA_FILE, std::ios::binary);
        if (!file.is_open()) return;

        size_t map_size = database.size();
        file.write(reinterpret_cast<const char*>(&map_size), sizeof(map_size));

        for (const auto& pair : database) {
            size_t key_len = pair.first.length();
            file.write(reinterpret_cast<const char*>(&key_len), sizeof(key_len));
            file.write(pair.first.c_str(), key_len);

            size_t set_size = pair.second.size();
            file.write(reinterpret_cast<const char*>(&set_size), sizeof(set_size));

            for (int value : pair.second) {
                file.write(reinterpret_cast<const char*>(&value), sizeof(value));
            }
        }
        file.close();
    }

    void load_from_file() {
        std::ifstream file(DATA_FILE, std::ios::binary);
        if (!file.is_open()) return;

        database.clear();

        size_t map_size;
        file.read(reinterpret_cast<char*>(&map_size), sizeof(map_size));

        for (size_t i = 0; i < map_size; i++) {
            size_t key_len;
            file.read(reinterpret_cast<char*>(&key_len), sizeof(key_len));

            std::string key(key_len, '\0');
            file.read(&key[0], key_len);

            size_t set_size;
            file.read(reinterpret_cast<char*>(&set_size), sizeof(set_size));

            std::set<int> values;
            for (size_t j = 0; j < set_size; j++) {
                int value;
                file.read(reinterpret_cast<char*>(&value), sizeof(value));
                values.insert(value);
            }

            database[key] = values;
        }
        file.close();
    }

public:
    FileStorageBPT() {
        load_from_file();
    }

    ~FileStorageBPT() {
        save_to_file();
    }

    void insert(const std::string& key, int value) {
        database[key].insert(value);
    }

    std::vector<int> find(const std::string& key) {
        std::vector<int> result;
        auto it = database.find(key);
        if (it != database.end()) {
            result.assign(it->second.begin(), it->second.end());
        }
        return result;
    }

    void remove(const std::string& key, int value) {
        auto it = database.find(key);
        if (it != database.end()) {
            it->second.erase(value);
            if (it->second.empty()) {
                database.erase(it);
            }
        }
    }
};

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    FileStorageBPT storage;

    int n;
    std::cin >> n;
    std::cin.ignore();

    for (int i = 0; i < n; i++) {
        std::string command;
        std::getline(std::cin, command);

        if (command.substr(0, 6) == "insert") {
            size_t space1 = command.find(' ', 7);
            std::string key = command.substr(7, space1 - 7);
            int value = std::stoi(command.substr(space1 + 1));
            storage.insert(key, value);
        } else if (command.substr(0, 6) == "delete") {
            size_t space1 = command.find(' ', 7);
            std::string key = command.substr(7, space1 - 7);
            int value = std::stoi(command.substr(space1 + 1));
            storage.remove(key, value);
        } else if (command.substr(0, 4) == "find") {
            std::string key = command.substr(5);
            std::vector<int> result = storage.find(key);

            if (result.empty()) {
                std::cout << "null" << std::endl;
            } else {
                for (size_t j = 0; j < result.size(); j++) {
                    if (j > 0) std::cout << " ";
                    std::cout << result[j];
                }
                std::cout << std::endl;
            }
        }
    }

    return 0;
}