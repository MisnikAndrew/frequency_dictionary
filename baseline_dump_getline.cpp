#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <locale>
#include <chrono>

#pragma GCC optimize unroll-loops

#define FNV32INIT 0x811C9DC5
#define FNV32PRIME 0x01000193

struct LightHash {
    uint32_t operator()(const std::stringview &str) const noexcept {
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

    void rehash() {
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
    }

    bool insert(const Key & key, const Value & value) {
        if (num_elements + 1 > capacity * max_load_factor) {
            rehash();
        }
        uint32_t idx = probe(key);
        while (table[idx].occupied) {
            if (table[idx].key == key) {
                table[idx].value = value;
                return false;
            }
            idx = (idx + 1) % capacity;
        }
        table[idx].occupied = true;
        table[idx].key = key;
        table[idx].value = value;
        ++num_elements;
        return true;
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
    std::ifstream file(argv[1]);
    file.imbue(std::locale(file.getloc()));

    
    std::chrono::duration<double> totalWordAllocTime(0);
    std::chrono::duration<double> totalLineAllocTime(0);

    if (!file.is_open()) {
        std::cerr << "Failed to open file!" << std::endl;
        return 1;
    }

    std::unordered_map<std::string, int> wordCount;
    FastUnorderedMap<std::string, int> newWordCount;
    // char line[1024];
    std::string line;
    std::string word;

    int lineAlloc = 0;
    int wordAlloc = 0;

    // while (file.getline(line, sizeof(line))) {
    auto lineAllocStart = std::chrono::high_resolution_clock::now();
    while (std::getline(file, line)) {
        totalLineAllocTime += (std::chrono::high_resolution_clock::now() - lineAllocStart);
        ++lineAlloc;
        /* for (char& ch : line) {
            if (ch >= 'A' && ch <= 'Z') {
                ch += 32;
            } else if (ch < 'a' || ch > 'z') {
                ch = 32;
            }
        }
        */

        for (char& ch : line) {
            if (ch < 'A' || ch > 'z' || (ch > 'Z' && ch < 'a')) {
                ch = 32;
            } else if (ch >= 'A' && ch <= 'Z') {
                ch += 32;
            }
        }
        auto wordAllocStart = std::chrono::high_resolution_clock::now();
        std::stringstream wss(line);
        totalWordAllocTime += (std::chrono::high_resolution_clock::now() - wordAllocStart);
        
        while (wss >> word) {
            ++wordAlloc;
            //start = std::chrono::high_resolution_clock::now();
            if (useFastMap) {
                ++newWordCount[word];
            } else {
                ++wordCount[word];
            }
            // totalIncrementsTime += (std::chrono::high_resolution_clock::now() - start1);
        }
        lineAllocStart = std::chrono::high_resolution_clock::now();
    }

    file.close();

    std::vector<std::pair<int, std::string>> result;
    std::cerr << newWordCount.num_elements << '\n';
    result.reserve(newWordCount.num_elements);
    if (useFastMap) {
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

    std::cerr << "lineAlloc: " << lineAlloc << '\n';
    std::cerr << "wordAlloc: " << wordAlloc << '\n';
    std::cout << "Total time spent on totalWordAllocTime: " << totalWordAllocTime.count() << " seconds" << std::endl;
    std::cout << "Total time spent on totalLineAllocTime: " << totalLineAllocTime.count() << " seconds" << std::endl;

    return 0;
}