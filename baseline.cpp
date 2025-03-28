#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <locale>
#include <chrono>

struct LightHash {
    std::size_t operator()(const std::string& s) const {
        std::size_t hash = 1;
        for (const auto& c : s) {
            hash = ((hash << 5) + hash) ^ c;
        }

        hash ^= hash >> 16;
        hash *= 0x85ebca6b;
        hash ^= hash >> 13;
        hash *= 0xc2b2ae35;
        hash ^= hash >> 16;
        return hash;
    }

 };
 
 template<typename Key, 
          typename Value, 
          typename Hash = std::hash<Key> >
 struct FastUnorderedMap {
    struct Bucket {
       bool occupied;
       Key key;
       Value value;
       Bucket() : occupied(false) {}
    };
 
    std::vector<Bucket> table;
    std::size_t num_elements;
    std::size_t capacity;
    float max_load_factor;
    Hash hashFn;

    void rehash() {
       std::size_t new_capacity = capacity * 2;
       std::vector<Bucket> new_table(new_capacity);
       for (std::size_t i = 0; i < capacity; i++) {
          if (table[i].occupied) {
             std::size_t idx = hashFn(table[i].key) % new_capacity;
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
 
    inline std::size_t probe(const Key & key) const {
       return hashFn(key) % capacity;
    }
 
 public:
    FastUnorderedMap(std::size_t init_capacity = 16, float load_factor = 0.7f)
       : num_elements(0), capacity(init_capacity), max_load_factor(load_factor) {
       table.resize(capacity);
    }

    bool insert(const Key & key, const Value & value) {
       if (num_elements + 1 > capacity * max_load_factor) {
          rehash();
       }
       std::size_t idx = probe(key);
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
       std::size_t idx = probe(key);
       std::size_t start = idx;
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
       std::size_t idx = probe(key);
       std::size_t start = idx;
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
        std::string option = argv[2];
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

    std::chrono::duration<double> totalTransformTime(0);
    std::chrono::duration<double> totalToLowerTime(0);
    std::chrono::duration<double> totalIncrementsTime(0);

    if (!file.is_open()) {
        std::cerr << "Failed to open file!" << std::endl;
        return 1;
    }

    std::unordered_map<std::string, int> wordCount;
    FastUnorderedMap<std::string, int> newWordCount;
    std::string line;
    std::string word;

    while (std::getline(file, line)) {
        // auto start = std::chrono::high_resolution_clock::now();
        for (char& ch : line) {
            if (!std::isalpha(static_cast<unsigned char>(ch))) {
                ch = ' ';
            }
        }
        // totalTransformTime += (std::chrono::high_resolution_clock::now() - start);


        std::stringstream wss(line);
        
        while (wss >> word) {
            // auto start = std::chrono::high_resolution_clock::now();
            for (char& ch : word) {
                ch = std::tolower(static_cast<unsigned char>(ch));
            }
            // totalToLowerTime += (std::chrono::high_resolution_clock::now() - start);
            // start = std::chrono::high_resolution_clock::now();
            if (useFastMap) {
                ++newWordCount[word];
            } else {
                ++wordCount[word];
            }
            // totalIncrementsTime += (std::chrono::high_resolution_clock::now() - start);
        }
    }

    file.close();

    std::vector<std::pair<int, std::string>> result;
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
    // std::cout << "Total time spent on transforming characters: " << totalTransformTime.count() << " seconds" << std::endl;
    // std::cout << "Total time spent on lowering characters: " << totalToLowerTime.count() << " seconds" << std::endl;
    // std::cout << "Total time spent on incrementing values: " << totalIncrementsTime.count() << " seconds" << std::endl;

    return 0;
}