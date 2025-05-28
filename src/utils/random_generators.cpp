#include "random_generators.h"

emp::PRG &RandomGenerators::rng_01() {
    return _rng_01;
}
emp::PRG &RandomGenerators::rng_D0() {
    return _rng_D0;
}
emp::PRG &RandomGenerators::rng_D1() {
    return _rng_D1;
}
emp::PRG &RandomGenerators::rng_D() {
    return _rng_D;
}
emp::PRG &RandomGenerators::rng_self() {
    return _rng_self;
}

RandomGenerators::RandomGenerators(uint64_t seeds_hi[5], uint64_t seeds_lo[5]) {
    auto seed_block = emp::makeBlock(seeds_hi[0], seeds_lo[0]);
    _rng_self.reseed(&seed_block, 0);

    seed_block = emp::makeBlock(seeds_hi[1], seeds_lo[1]);
    _rng_D.reseed(&seed_block, 0);

    seed_block = emp::makeBlock(seeds_hi[2], seeds_lo[2]);
    _rng_D0.reseed(&seed_block, 0);

    seed_block = emp::makeBlock(seeds_hi[3], seeds_lo[3]);
    _rng_D1.reseed(&seed_block, 0);

    seed_block = emp::makeBlock(seeds_hi[4], seeds_lo[4]);
    _rng_01.reseed(&seed_block, 0);
}
