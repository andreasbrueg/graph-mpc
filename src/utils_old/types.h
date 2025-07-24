#pragma once

#include <cstdint>
#include <vector>

enum Party { P0 = 0, P1 = 1, D = 2 };

typedef uint32_t Ring;

// void print_vec(std::vector<Ring> &vec) {
// for (size_t i = 0; i < vec.size(); ++i) {
// if (i != vec.size() - 1) {
// std::cout << vec[i] << ", ";
//} else {
// std::cout << vec[i] << std::endl;
//}
//}
//}