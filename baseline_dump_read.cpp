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

 struct LightHashV2
{
    inline uint32_t operator()(std::string sv) const noexcept
    {
        uint32_t hash = 1;
        for (char c : sv) {
            hash ^= c;
            hash *= FNV32PRIME;
        }
        return hash;
    }
};
 
template<typename Hash = LightHash>
struct FastUnorderedMap {
    struct Bucket {
        std::string key;
        int value;
        bool used;
    };
 
    std::vector<Bucket> table;
    uint32_t numElements;
    uint32_t capacity;
    float maxLoadFactor;
    Hash hashFn;

    void rehash() { // 0.015 sec
        uint32_t newCapacity = capacity * 2;
        std::vector<Bucket> newTable(newCapacity);
        for (uint32_t i = 0; i < capacity; i++) {
            if (table[i].used) {
                uint32_t idx = hashFn(table[i].key) % newCapacity;
                while (newTable[idx].used) {
                    idx = (idx + 1) % newCapacity;
                }
                newTable[idx] = {table[i].key, table[i].value, true};
            }
        }
        table.swap(newTable);
        capacity = newCapacity;
    }

 public:
    FastUnorderedMap(uint32_t initCapacity = 16, float loadFactor = 0.5f)
       : numElements(0), capacity(initCapacity), maxLoadFactor(loadFactor) {
       table.resize(capacity);
       table.reserve(initCapacity);
    }

    void insert(const std::string & key) {
        if (numElements > capacity * maxLoadFactor) {
            rehash();
        }
        uint32_t idx = hashFn(key) % capacity;
        while (table[idx].used) {
            if (table[idx].key == key) {
                ++table[idx].value;
                return;
            }
            idx = (idx + 1) % capacity;
        }
        table[idx] = {key, 1, true};
        ++numElements;
        return;
    }
 };


template<typename Hash = LightHash
>
struct FastUnorderedMapV2 {
    struct Bucket {
        std::size_t hashValue;
        std::size_t keyIndex;
        int value;
        bool used;
    };
 
    std::vector<Bucket> table;
    std::vector<std::string> keyStorage;
 
    std::uint32_t numElements = 0;
    std::uint32_t capacity    = 0;
    float maxLoadFactor;
    Hash hashFn;
 
    FastUnorderedMapV2(std::uint32_t initCapacity = 16, float loadFactor = 0.5f)
        : capacity(initCapacity), maxLoadFactor(loadFactor) {
        table.resize(capacity);
        table.reserve(initCapacity);
    }
 
    void rehash() {
        std::uint32_t newCapacity = capacity * 2;
        std::vector<Bucket> newTable(newCapacity);

        for (auto &b : table) {
            if (b.used) {
                std::size_t idx = b.hashValue % newCapacity;
                while (newTable[idx].used) {
                    idx = (idx + 1) % newCapacity;
                }
                newTable[idx] = b;
            }
        }

        table.swap(newTable);
        capacity = newCapacity;
    }
 
    void insert(std::string const &key) {
        if (numElements > capacity * maxLoadFactor) {
            rehash();
        }

        std::size_t h = hashFn(key);
        std::uint32_t idx = static_cast<std::uint32_t>(h % capacity);
        while (table[idx].used) {
            if (table[idx].hashValue == h) {
                if (keyStorage[table[idx].keyIndex] == key) {
                    ++table[idx].value;
                    return;
                }
            }
            idx = (idx + 1) % capacity;
        }

        table[idx] = {h, keyStorage.size(), 1, true};
        keyStorage.emplace_back(key);
        ++numElements;
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
    FastUnorderedMapV2<> fastWordCount;
    char buffer[BUF_SIZE];

    ssize_t bytesRead = 0;
    bool state = false;
    int lastAlpha = -1;
    uint16_t prevSize = 0;
    char prevWord[32];

    while ((bytesRead = read(fd, buffer, BUF_SIZE)) > 0) { // 0.05 sec
        for (size_t idx = 0; idx < bytesRead; ++idx) {
            if ((buffer[idx]>= 'A' && buffer[idx] <= 'Z') || (buffer[idx] >= 'a' && buffer[idx] <= 'z')) {
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
                    if (useFastMap) {
                        fastWordCount.insert(std::string(prevWord, prevSize + idx));
                    } else {
                        ++wordCount[std::string(prevWord, prevSize + idx)];
                    }
                    prevSize = 0;
                } else if (state) {
                    if (useFastMap) {
                        fastWordCount.insert(std::string(buffer + lastAlpha, idx - lastAlpha));
                    } else {
                        ++wordCount[std::string(buffer + lastAlpha, idx - lastAlpha)];
                    }
                }
                state = false;
            }
        }
        if (state) {
            prevSize = bytesRead - lastAlpha;
            std::memcpy(prevWord, buffer + lastAlpha, prevSize);
        }
    }

    std::vector<std::pair<int, std::string>> result;
    result.reserve(fastWordCount.numElements);
    if (useFastMap) { // 0.02 sec
        for (const auto& pair : fastWordCount.table) {
            if (pair.value > 0) {
                result.push_back({pair.value, ""});
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