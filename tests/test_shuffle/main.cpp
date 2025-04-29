#include "../src/perm.h"
#include "../src/random_generators.h"
#include "../src/shuffle.h"
#include "../src/utils/types.h"

void test_correctness() {
    const size_t n_rows = 100;
    const size_t n_rounds = 1;

    /* Dummy seed */
    uint64_t seeds[4] = {123456789, 123456789, 123456789, 123456789};
    RandomGenerators _rngs(seeds, seeds);
}

int main(int argc, char **argv) {
    test_correctness();
    return 0;
}