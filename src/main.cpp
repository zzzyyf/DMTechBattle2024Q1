#include "Common.h"
#include "zlib/zlib.h"

#include <cassert>
#include <chrono>
#include <fstream>
#include <sstream>

#include "hash1.h"

#include <thread>

namespace dm {

constexpr u32 data_size = 10'000'000; // 10'000'000;
std::string st_data[data_size];
u32 st_crc_data[data_size];

};

using namespace dm;

using Map = std::unordered_map<std::string, u32>;
// using Map = std::unordered_map<std::string, u32, Hash1>;

template <class M>
void insert(M &map);

template <class M>
void search(M &map);

u64 getMemUsage();

int main(int argc, char *argv[])
{
    initHash1();

    for (u32 i = 0; i < data_size; i++)
    {
        st_data[i] = genString();

        u32 crc = crc32(0l, Z_NULL, 0);
        const unsigned char *data = reinterpret_cast<unsigned char *>(st_data[i].data());
        st_crc_data[i] = crc32(crc, data, st_data[i].size());
    }

    std::cout << "init mem usage: " << getMemUsage() << std::endl;

    auto start = std::chrono::steady_clock::now();

    Map map;
    insert(map);
    auto end = std::chrono::steady_clock::now();
    
    std::chrono::duration<double, std::milli> diff = end - start;
    std::cout << "insert cost " << diff.count() << "ms" << std::endl;

    std::cout << "map size: " << map.size() << std::endl;
    std::cout << "mem usage after insert: " << getMemUsage() << std::endl;
    
    // for (auto itr : map)
    // {
    //     std::cout << itr.first << ": " << itr.second << std::endl;
    // }

    start = std::chrono::steady_clock::now();
    search(map);
    end = std::chrono::steady_clock::now();
    
    diff = end - start;
    std::cout << "search cost " << diff.count() << "ms" << std::endl;

    return 0;
}

template <class M>
void insert(M &map)
{
    for (u32 i = 0; i < data_size; i++)
    {
        map.emplace(st_data[i], st_crc_data[i]);
    }
}

template <class M>
void search(M &map)
{
    for (u32 i = 0; i < data_size; i++)
    {
        volatile auto itr = map.find(st_data[i]);
        // assert(itr != map.end());

        assert(st_crc_data[i] == itr->second);
    }
}

u64 getMemUsage()
{
    std::ifstream file("/proc/self/status");
    std::string line;
    long long memory_usage = 0;
    const std::string prefix = "VmRSS:";
    while (std::getline(file, line))
    {
        if (line.compare(0, prefix.size(), prefix) == 0)
        {
            std::istringstream iss(line.substr(prefix.size() + 1));
            iss >> memory_usage;
            break;
        }
    }
    return memory_usage;
}