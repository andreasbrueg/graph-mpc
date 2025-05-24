#include <cassert>
#include <iostream>
#include <numeric>

#include "../src/perm.h"
#include "../src/random_generators.h"
#include "../src/utils/types.h"

RandomGenerators rngs((uint64_t[4]){123456789, 123456789, 123456789, 123456789}, (uint64_t[4]){123456789, 123456789, 123456789, 123456789});

bool contains_duplicates(Permutation p) {
    for (int i = 0; i < p.size(); ++i) {
        for (int j = 0; j < p.size(); ++j) {
            if (i != j) {
                if (p[i] == p[j]) {
                    return true;
                }
            }
        }
    }
    return false;
}

void test_plausibility() {
    Permutation swap_4_5(std::vector<Row>({0, 1, 2, 3, 5, 4, 6, 7, 8, 9}));
    Permutation swap_0_9(std::vector<Row>({9, 1, 2, 3, 4, 5, 6, 7, 8, 0}));
    std::vector<Row> test_vec = std::vector<Row>({0, 0, 0, 0, 0, 1, 1, 1, 1, 1});

    std::vector<Row> res1 = swap_4_5(test_vec);
    assert((res1 == std::vector<Row>({0, 0, 0, 0, 1, 0, 1, 1, 1, 1})));

    std::vector<Row> res2 = swap_0_9(res1);
    assert((res2 == std::vector<Row>({1, 0, 0, 0, 1, 0, 1, 1, 1, 0})));
}

void test_associativity() {
    const int n_elems = 100;
    Permutation pi_0 = Permutation::random(n_elems, rngs.rng_D0());
    Permutation pi_1 = Permutation::random(n_elems, rngs.rng_D1());
    Permutation pi_2 = Permutation::random(n_elems, rngs.rng_D1());

    std::vector<Row> test_table = std::vector<Row>(n_elems);
    std::iota(test_table.begin(), test_table.end(), 0);

    std::vector<Row> res1 = ((pi_0 * pi_1) * pi_2)(test_table);
    std::vector<Row> res2 = (pi_0 * (pi_1 * pi_2))(test_table);

    assert((res1 == res2));
}

void test_inverse() {
    const int n_elems = 100;
    Permutation perm = Permutation::random(n_elems, rngs.rng_D());
    Permutation inv = perm.inverse();

    std::vector<Row> test_vec = std::vector<Row>(n_elems);
    std::iota(test_vec.begin(), test_vec.end(), 0);

    assert(((perm * inv)(test_vec) == test_vec));
}

void test_pi_1_p() {
    const int n_elems = 100;
    Permutation pi_0 = Permutation::random(n_elems, rngs.rng_D0());
    Permutation pi_1 = Permutation::random(n_elems, rngs.rng_D1());
    Permutation pi_0_p = Permutation::random(n_elems, rngs.rng_D0());

    Permutation pi_1_p = pi_0_p.inverse() * (pi_0 * pi_1);
    assert((pi_0_p * pi_1_p == pi_0 * pi_1));
}

void test_fact_2_3() {
    const int n_elems = 100;
    Permutation pi = Permutation::random(n_elems, rngs.rng_D());
    Permutation sigma = Permutation::random(n_elems, rngs.rng_D());

    std::vector<Row> a = std::vector<Row>(n_elems);
    std::iota(a.begin(), a.end(), 0); /* 0, 1, 2, 3, ... */

    auto left = (pi * sigma).inverse()(a);
    auto right = (sigma.inverse() * pi.inverse())(a);

    assert(left == right);

    auto left_two = pi(pi.inverse()(a));
    auto right_two = pi.inverse()(pi(a));

    assert(left_two == right_two);
}

void test_observation_2_4() {
    const int n_elems = 100;
    Permutation pi = Permutation::random(n_elems, rngs.rng_D());
    Permutation sigma = Permutation::random(n_elems, rngs.rng_D());

    std::vector<Row> a = std::vector<Row>(n_elems);
    std::iota(a.begin(), a.end(), 0); /* 0, 1, 2, 3, ... */

    auto left = Permutation(pi(sigma.get_perm_vec()));
    auto right = (sigma * pi.inverse());

    assert(left == right);
}

void custom() {
    Permutation pi = Permutation({4, 6, 0, 3, 2, 1, 7, 5, 9, 8});
    Permutation pi_inv = Permutation({2, 5, 4, 3, 0, 7, 1, 6, 9, 8});
    Permutation p1 = Permutation({5, 8, 2, 6, 0, 1, 4, 7, 3, 9});
    Permutation p2 = Permutation({4, 0, 6, 2, 9, 8, 5, 3, 7, 1});
    Permutation omega = Permutation({3, 8, 7, 4, 0, 1, 9, 5, 6, 2});
    Permutation omega_inv = Permutation({4, 5, 9, 0, 3, 7, 8, 2, 1, 6});

    std::vector<Row> input_vector = std::vector<Row>({0, 2, 3, 3, 4, 7, 8, 8, 9, 9});

    auto reverse_sorted_vector = p2(omega_inv(omega(pi_inv(pi(p1.inverse()(input_vector))))));

    std::cout << "res: ";
    for (const auto &elem : reverse_sorted_vector) {
        std::cout << elem << ", ";
    }
    std::cout << std::endl;
}

int main(int argc, char **argv) {
    test_plausibility();
    test_associativity();
    test_inverse();
    test_pi_1_p();
    test_fact_2_3();
    test_observation_2_4();
    custom();
    return 0;
}