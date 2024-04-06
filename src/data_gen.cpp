#include "Common.h"
#include <string>
#include <random>

namespace dm {

std::random_device r;

std::default_random_engine e1(r());

std::uniform_int_distribution<int> len_uniform_dist(10, 60);
std::uniform_int_distribution<int> char_uniform_dist(0, 25);

std::string genString()
{
    int len = len_uniform_dist(e1);
    std::string ss(len, 'c');

    for (u32 i = 0; i < len; i++) {
        ss[i] = 'a' + char_uniform_dist(e1);
    }

    return ss;
}




};