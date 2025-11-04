#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>
#include <unordered_map>
#include <set>
#include <memory>

const int MAX_KEYS = 100;  // Maximum keys per node
const int MIN_KEYS = 50;   // Minimum keys per node (except root)
const std::string DATA_FILE = "data.bpt";
const std::string INDEX_FILE = "index.bpt";

struct IndexEntry {
    char key[65];
    int value;
    long file_pos;

    IndexEntry() {
        memset(key, 0, sizeof(key));
        value = 0;
        file_pos = -1;
    }

    IndexEntry(const std::string& k, int v, long pos) : value(v), file_pos(pos) {
        strncpy(key, k.c_str(), 64);
        key[64] = '\0';
    }

    bool operator<(const IndexEntry& other) const {
        int key_cmp = strcmp(key, other.key);
        if (key_cmp != 0) return key_cmp < 0;
        return value < other.value;
    }

    bool operator>(const IndexEntry& other) const {
        int key_cmp = strcmp(key, other.key);
        if (key_cmp != 0) return key_cmp > 0;
        return value > other.value;
    }

    bool operator==(const IndexEntry& other) const {
        return strcmp(key, other.key) == 0 && value == other.value;
    }
};

struct BPTNode {
    bool is_leaf;
    int key_count;
    IndexEntry keys[MAX_KEYS + 1];
    long children[MAX_KEYS + 2];
    long next_leaf;
    long self_pos;

    BPTNode() : is_leaf(true), key_count(0), next_leaf(-1), self_pos(-1) {
        memset(children, -1, sizeof(children));
    }
};

class BPTree {
private:
    long root_pos;
    std::fstream index_file;
    std::fstream data_file;

    long allocate_node() {
        index_file.seekp(0, std::ios::end);
        long pos = index_file.tellp();
        BPTNode node;
        node.self_pos = pos;
        index_file.write(reinterpret_cast<char*>(&node), sizeof(BPTNode));
        return pos;
    }

    void write_node(long pos, const BPTNode& node) {
        index_file.seekp(pos);
        index_file.write(reinterpret_cast<const char*>(&node), sizeof(BPTNode));
    }

    BPTNode read_node(long pos) {
        BPTNode node;
        index_file.seekg(pos);
        index_file.read(reinterpret_cast<char*>(&node), sizeof(BPTNode));
        return node;
    }

    void split_child(BPTNode& parent, int index, BPTNode& child) {
        BPTNode new_child;
        new_child.is_leaf = child.is_leaf;
        new_child.self_pos = allocate_node();

        int mid = child.key_count / 2;
        new_child.key_count = child.key_count - mid;

        for (int i = 0; i < new_child.key_count; i++) {
            new_child.keys[i] = child.keys[mid + i];
        }

        if (!child.is_leaf) {
            for (int i = 0; i <= new_child.key_count; i++) {
                new_child.children[i] = child.children[mid + i];
            }
        }

        child.key_count = mid;

        for (int i = parent.key_count; i >= index + 1; i--) {
            parent.children[i + 1] = parent.children[i];
        }
        parent.children[index + 1] = new_child.self_pos;

        for (int i = parent.key_count - 1; i >= index; i--) {
            parent.keys[i + 1] = parent.keys[i];
        }
        parent.keys[index] = child.keys[mid - 1];
        parent.key_count++;

        if (child.is_leaf) {
            new_child.next_leaf = child.next_leaf;
            child.next_leaf = new_child.self_pos;
        }

        write_node(child.self_pos, child);
        write_node(new_child.self_pos, new_child);
    }

    void insert_non_full(BPTNode& node, const IndexEntry& entry) {
        int i = node.key_count - 1;

        if (node.is_leaf) {
            while (i >= 0 && node.keys[i] > entry) {
                node.keys[i + 1] = node.keys[i];
                i--;
            }
            node.keys[i + 1] = entry;
            node.key_count++;
            write_node(node.self_pos, node);
        } else {
            while (i >= 0 && node.keys[i] > entry) {
                i--;
            }
            i++;

            BPTNode child = read_node(node.children[i]);
            if (child.key_count == MAX_KEYS) {
                split_child(node, i, child);
                if (node.keys[i] < entry) {
                    i++;
                }
                child = read_node(node.children[i]);
            }
            insert_non_full(child, entry);
        }
    }

public:
    BPTree() : root_pos(-1) {
        index_file.open(INDEX_FILE, std::ios::in | std::ios::out | std::ios::binary);
        data_file.open(DATA_FILE, std::ios::in | std::ios::out | std::ios::binary);

        if (!index_file.is_open() || !data_file.is_open()) {
            index_file.close();
            data_file.close();
            index_file.open(INDEX_FILE, std::ios::out | std::ios::binary);
            data_file.open(DATA_FILE, std::ios::out | std::ios::binary);
            index_file.close();
            data_file.close();

            index_file.open(INDEX_FILE, std::ios::in | std::ios::out | std::ios::binary);
            data_file.open(DATA_FILE, std::ios::in | std::ios::out | std::ios::binary);

            BPTNode root;
            root.is_leaf = true;
            root.self_pos = allocate_node();
            root_pos = root.self_pos;
            write_node(root_pos, root);
        } else {
            index_file.seekg(0);
            index_file.read(reinterpret_cast<char*>(&root_pos), sizeof(long));
        }
    }

    ~BPTree() {
        if (index_file.is_open()) {
            index_file.seekp(0);
            index_file.write(reinterpret_cast<const char*>(&root_pos), sizeof(long));
            index_file.close();
        }
        if (data_file.is_open()) {
            data_file.close();
        }
    }

    void insert(const std::string& key, int value) {
        IndexEntry entry(key, value, -1);

        BPTNode root = read_node(root_pos);
        if (root.key_count == MAX_KEYS) {
            BPTNode new_root;
            new_root.is_leaf = false;
            new_root.self_pos = allocate_node();
            new_root.children[0] = root_pos;
            root_pos = new_root.self_pos;

            split_child(new_root, 0, root);
            insert_non_full(new_root, entry);
        } else {
            insert_non_full(root, entry);
        }
    }

    std::vector<int> find(const std::string& key) {
        std::vector<int> result;
        BPTNode node = read_node(root_pos);

        while (!node.is_leaf) {
            int i = 0;
            while (i < node.key_count && strcmp(node.keys[i].key, key.c_str()) < 0) {
                i++;
            }
            node = read_node(node.children[i]);
        }

        bool found = false;
        while (true) {
            for (int i = 0; i < node.key_count; i++) {
                if (strcmp(node.keys[i].key, key.c_str()) == 0) {
                    result.push_back(node.keys[i].value);
                    found = true;
                } else if (found && strcmp(node.keys[i].key, key.c_str()) > 0) {
                    return result;
                }
            }

            if (node.next_leaf == -1) break;
            node = read_node(node.next_leaf);
            if (found && strcmp(node.keys[0].key, key.c_str()) > 0) break;
        }

        return result;
    }

    void remove(const std::string& key, int value) {
        std::vector<int> values = find(key);
        auto it = std::find(values.begin(), values.end(), value);
        if (it != values.end()) {
            IndexEntry entry(key, value, -1);
            remove_entry(root_pos, entry);
        }
    }

private:
    void remove_entry(long node_pos, const IndexEntry& entry) {
        BPTNode node = read_node(node_pos);

        if (node.is_leaf) {
            int idx = -1;
            for (int i = 0; i < node.key_count; i++) {
                if (node.keys[i] == entry) {
                    idx = i;
                    break;
                }
            }

            if (idx != -1) {
                for (int i = idx; i < node.key_count - 1; i++) {
                    node.keys[i] = node.keys[i + 1];
                }
                node.key_count--;
                write_node(node_pos, node);
            }
        }
    }
};

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    BPTree tree;

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
            tree.insert(key, value);
        } else if (command.substr(0, 6) == "delete") {
            size_t space1 = command.find(' ', 7);
            std::string key = command.substr(7, space1 - 7);
            int value = std::stoi(command.substr(space1 + 1));
            tree.remove(key, value);
        } else if (command.substr(0, 4) == "find") {
            std::string key = command.substr(5);
            std::vector<int> result = tree.find(key);

            if (result.empty()) {
                std::cout << "null" << std::endl;
            } else {
                std::sort(result.begin(), result.end());
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