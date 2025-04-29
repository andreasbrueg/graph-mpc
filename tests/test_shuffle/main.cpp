#include <cstring>

#include "../src/perm.h"
#include "../src/random_generators.h"
#include "../src/shuffle.h"
#include "../src/utils/types.h"

void test_single_shuffle() {
    const size_t n_rows = 100;
    const size_t n_rounds = 1;

    Party pid = P1;

    /* Dummy seed */
    uint64_t seeds[4] = {123456789, 123456789, 123456789, 123456789};
    RandomGenerators rngs(seeds, seeds);

    char *ip[] = {};
    std::string certificate_path = "";
    std::string private_key_path = "";
    std::string trusted_cert_path = "";
    bool localhost = true;

    io::NetIOMP network(1, 3, 8000, ip, certificate_path, private_key_path, trusted_cert_path, localhost);
    Shuffle shuffle(pid, n_rows, n_rounds, rngs, std::shared_ptr<io::NetIOMP>(&network));
}

int main(int argc, char **argv) {
    test_single_shuffle();
    return 0;
}