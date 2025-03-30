#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <string.h>
#include <map>
#include <locale>
#include <chrono>
#include <unistd.h>
#include <fcntl.h>

#pragma GCC optimize unroll-loops

#define FNV32INIT 0x811C9DC5
#define FNV32PRIME 0x01000193

constexpr int BUF_SIZE = 4096 * 16;

struct LightHash {
    uint32_t operator()(const std::string& str) const noexcept {
        auto b = str.begin();
        uint32_t ans = 1;
        while (b != str.end()) {
            ans = (ans * FNV32PRIME) ^ *b++;
        }
        return ans;
    }
 };

 template<typename Key, 
          typename Value, 
          typename Hash = LightHash >
 struct FastUnorderedMap {
    struct Bucket {
       bool occupied;
       Key key;
       Value value;
       Bucket() : occupied(false) {}
    };
 
    std::vector<Bucket> table;
    uint32_t num_elements;
    uint32_t capacity;
    float max_load_factor;
    Hash hashFn;

    void rehash() { // 0.015 sec
        uint32_t new_capacity = capacity * 2;
        std::vector<Bucket> new_table(new_capacity);
        for (uint32_t i = 0; i < capacity; i++) {
            if (table[i].occupied) {
                uint32_t idx = hashFn(table[i].key) % new_capacity;
                while (new_table[idx].occupied) {
                    idx = (idx + 1) % new_capacity;
                }
                new_table[idx].occupied = true;
                new_table[idx].key = table[i].key;
                new_table[idx].value = table[i].value;
            }
        }
        table.swap(new_table);
        capacity = new_capacity;
    }
 
    inline uint32_t probe(const Key & key) const {
       return hashFn(key) % capacity;
    }
 
 public:
    FastUnorderedMap(uint32_t init_capacity = 16, float load_factor = 0.5f)
       : num_elements(0), capacity(init_capacity), max_load_factor(load_factor) {
       table.resize(capacity);
       table.reserve(init_capacity);
    }

    void insert(const Key & key) {
        if (num_elements > capacity * max_load_factor) {
            rehash();
        }
        uint32_t idx = probe(key);
        while (table[idx].occupied) {
            if (table[idx].key == key) {
                ++table[idx].value;
                return;
            }
            idx = (idx + 1) % capacity;
        }
        table[idx].occupied = true;
        table[idx].key = key;
        table[idx].value = 1;
        ++num_elements;
        return;
    }

    Value * find(const Key & key) {
        uint32_t idx = probe(key);
        uint32_t start = idx;
        while (table[idx].occupied) {
            if (table[idx].key == key) {
                return &table[idx].value;
            }
            idx = (idx + 1) % capacity;
            if (idx == start) break;
        }
        return nullptr;
    }
 
    Value & operator[](const Key & key) {
       if (num_elements + 1 > capacity * max_load_factor) {
          rehash();
       }
       uint32_t idx = probe(key);
       uint32_t start = idx;
       while (table[idx].occupied) {
          if (table[idx].key == key) {
             return table[idx].value;
          }
          idx = (idx + 1) % capacity;
          if (idx == start) break;
       }
       table[idx].occupied = true;
       table[idx].key = key;
       ++num_elements;
       return table[idx].value;
    }
 };

int main(int argc, char* argv[]) {
    if (argc < 2) {
        exit(1);
    }

    bool printResult = false;
    bool useFastMap = false;
    for (size_t it = 2; it < argc; ++it) {
        std::string option = argv[it];
        if (option == "--print-result") {
            printResult = true;
        }
        if (option == "--fast-map") {
            useFastMap = true;
        }
    }


    auto start = std::chrono::high_resolution_clock::now();
    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        std::cerr << "Failed to open file descriptor" << std::endl;
        exit(1);
    }

    std::unordered_map<std::string, int> wordCount;
    FastUnorderedMap<std::string, int> newWordCount;
    char buffer[BUF_SIZE];

    ssize_t bytes_read = 0;
    bool state = false;
    int lastAlpha = -1;
    uint16_t prevSize = 0;
    char prevWord[32];

    while ((bytes_read = read(fd, buffer, BUF_SIZE)) > 0) { // 0.05 sec
        for (size_t idx = 0; idx < bytes_read; ++idx) {
            if (std::isalpha(buffer[idx])) {
                if (buffer[idx] >= 'A' && buffer[idx] <= 'Z') {
                    buffer[idx] += 32;
                }
                if (!state) {
                    lastAlpha = idx;
                    state = true;
                }
            } else {
                if (prevSize) {
                    std::strncpy(prevWord + prevSize, buffer, idx);
                    newWordCount.insert(std::string(prevWord, prevSize + idx));
                    prevSize = 0;
                } else if (state) {
                    newWordCount.insert(std::string(buffer + lastAlpha, idx - lastAlpha));
                }
                state = false;
            }
        }
        if (state) {
            prevSize = bytes_read - lastAlpha;
            std::memcpy(prevWord, buffer + lastAlpha, prevSize);
        }
    }

    std::vector<std::pair<int, std::string>> result;
    result.reserve(newWordCount.num_elements);
    if (useFastMap) { // 0.02 sec
        for (const auto& pair : newWordCount.table) {
            if (pair.value > 0) {
                result.push_back({pair.value, pair.key});
            }
        }
    } else {
        for (const auto& pair : wordCount) {
            result.push_back({pair.second, pair.first});
        }
    }
    std::cout << "file_name = " << argv[1] << " | result size = " << result.size() << " | ";

    if (printResult) {
        for (const auto& pair : result) {
            printf("%d %s\n", pair.first, pair.second.c_str());
        }
    }

    {
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = now - start;
        std::cout << "Time elapsed: " << duration.count() << " seconds" << std::endl;
    }
    return 0;
}