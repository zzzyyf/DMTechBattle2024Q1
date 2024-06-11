#include "Common.h"
#include "zlib/zlib.h"

#include <cassert>
#include <chrono>
#include <fstream>
#include <sstream>

#include "MyHashMap.h"

#include <thread>
#include <vector>

namespace dm {

constexpr u32 data_size = 10'000'000; // 10'000'000;
std::string st_data[data_size];
u32 st_crc_data[data_size];

constexpr u32 parallel = 24;

};

using namespace dm;

using Map = MyHashMap;
// using Map = std::unordered_map<std::string, u32>;
// using Map = std::unordered_map<std::string, u32, Hash1>;

template <class M>
void dispatch(const u32 n, const u32 parallel, M &map);

template <class M>
void insert(M &map, const u32 id, const u32 start, const u32 end_ex);

template <class M>
void search(M &map, const u32 id, const u32 start, const u32 end_ex);

u64 getMemUsage();

int main(int argc, char *argv[])
{
    // initHash1();

    for (u32 i = 0; i < data_size; i++)
    {
        st_data[i] = genString();

        u32 crc = crc32(0l, Z_NULL, 0);
        const unsigned char *data = reinterpret_cast<unsigned char *>(st_data[i].data());
        st_crc_data[i] = crc32(crc, data, st_data[i].size());
    }

    std::cout << "init mem usage kb: " << getMemUsage() << std::endl;

    Map map;
    dispatch(data_size, parallel, map);

    return 0;
}


template <class M>
void dispatch(const u32 n, const u32 parallel, M &map)
{
    auto t1 = std::chrono::steady_clock::now();

    u32 slice_size = n / parallel;

    u32 start = 0;
    u32 remainder = n % parallel;
    u32 end_ex = start;

    std::vector<std::thread> threads(parallel);

    for (u32 i = 0; i < parallel; i++)
    {
        end_ex += slice_size + (i < remainder ? 1 : 0);
        threads[i] = std::thread([&, i, start, end_ex](){ insert(map, i, start, end_ex); });
        start = end_ex;
    }

    for (auto &th : threads) {
        th.join();
    }

    auto t2 = std::chrono::steady_clock::now();

    std::chrono::duration<double, std::milli> diff = t2 - t1;
    std::cout << "insert cost " << diff.count() << "ms" << std::endl;

    std::cout << "map size: " << map.size() << std::endl;
    std::cout << "mem usage kb after insert: " << getMemUsage() << std::endl;

    t1 = std::chrono::steady_clock::now();

    start = 0;
    end_ex = start;
    for (u32 i = 0; i < parallel; i++)
    {
        end_ex += slice_size + (i < remainder ? 1 : 0);
        threads[i] = std::thread([&, i, start, end_ex](){ search(map, i, start, end_ex); });
        start = end_ex;
    }

    for (auto &th : threads) {
        th.join();
    }

    t2 = std::chrono::steady_clock::now();
    diff = t2 - t1;
    std::cout << "search cost " << diff.count() << "ms" << std::endl;
}

template <class M>
void insert(M &map, const u32 id, const u32 start, const u32 end_ex)
{
    for (u32 i = start; i < end_ex; i++)
    {
        map.emplace(id, st_data[i], st_crc_data[i]);
    }
}

template <class M>
void search(M &map, const u32 id, const u32 start, const u32 end_ex)
{
    for (u32 i = start; i < end_ex; i++)
    {
        auto itr = map.find(id, st_data[i]);

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