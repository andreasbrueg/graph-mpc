#pragma once

#include "../utils/permutation.h"
#include "types.h"

class ShufflePre {
   public:
    ShufflePre() = default;

    ShufflePre(Permutation &pi_0, Permutation &pi_1, Permutation &pi_0_p, Permutation &pi_1_p, std::vector<Ring> &B, std::vector<Ring> &R)
        : pi_0(pi_0), pi_1(pi_1), pi_0_p(pi_0_p), pi_1_p(pi_1_p), B(B), R(R) {};

    Permutation pi_0;
    Permutation pi_1;
    Permutation pi_0_p;
    Permutation pi_1_p;
    std::vector<Ring> B;
    std::vector<Ring> R;

    bool preprocessed = false;

    bool has_pi_0 = false;
    bool has_pi_1 = false;
    bool has_pi_0_p = false;
    bool has_pi_1_p = false;
    bool has_B = false;
    bool has_R = false;

    void clear(Party id) {
        if (id == P0) {
            pi_0.perm_vec.clear();
            pi_0_p.perm_vec.clear();
            B.clear();
            R.clear();
        }
        if (id == P1) {
            pi_1.perm_vec.clear();
            pi_1_p.perm_vec.clear();
            B.clear();
            R.clear();
        }
        if (id == D) {
            pi_0.perm_vec.clear();
            pi_1.perm_vec.clear();
        }
        preprocessed = false;
        has_pi_0 = false;
        has_pi_1 = false;
        has_pi_0_p = false;
        has_pi_1_p = false;
        has_B = false;
        has_R = false;
    }
};