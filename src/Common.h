#pragma once

#include <cstdint>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>

namespace dm {

using u8 = uint8_t;
using u16 = uint16_t;
using i32 = int32_t;
using u32 = uint32_t;
using i64 = int64_t;
using u64 = uint64_t;

using String = std::string;
using StringRef = std::basic_string_view<char>;

template<class T>
using Ptr = std::shared_ptr<T>;

inline char *find(const char *start, const u32 length, const StringRef &token)
{
    if (length == 0)
        return nullptr;

    StringRef haystack(start, length);
    auto pos = haystack.find(token);
    if (pos == haystack.npos)
    {
        return nullptr;
    }

    return const_cast<char *>(start) + pos;
}

std::string genString();

}   // end of namespace dm;
