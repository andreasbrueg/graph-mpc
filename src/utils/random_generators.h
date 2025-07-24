#pragma once

#include <emp-tool/emp-tool.h>

#include <cstdint>
#include <random>

#include "types.h"

class RandomGenerators {
    emp::PRG _rng_01;
    emp::PRG _rng_D0;
    emp::PRG _rng_D0_unshuffle;
    emp::PRG _rng_D0_comp;
    emp::PRG _rng_D1;
    emp::PRG _rng_D1_unshuffle;
    emp::PRG _rng_D1_comp;
    emp::PRG _rng_D;
    emp::PRG _rng_self;

   public:
    RandomGenerators(uint64_t seeds_hi[9], uint64_t seeds_lo[9]) {
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

        seed_block = emp::makeBlock(seeds_hi[5], seeds_lo[5]);
        _rng_D0_unshuffle.reseed(&seed_block, 0);

        seed_block = emp::makeBlock(seeds_hi[6], seeds_lo[6]);
        _rng_D1_unshuffle.reseed(&seed_block, 0);

        seed_block = emp::makeBlock(seeds_hi[7], seeds_lo[7]);
        _rng_D0_comp.reseed(&seed_block, 0);

        seed_block = emp::makeBlock(seeds_hi[8], seeds_lo[8]);
        _rng_D1_comp.reseed(&seed_block, 0);
    }

    emp::PRG &rng_01() { return _rng_01; }
    emp::PRG &rng_D0() { return _rng_D0; }
    emp::PRG &rng_D0_unshuffle() { return _rng_D0_unshuffle; }
    emp::PRG &rng_D0_comp() { return _rng_D0_comp; }
    emp::PRG &rng_D1() { return _rng_D1; }
    emp::PRG &rng_D1_unshuffle() { return _rng_D1_unshuffle; }
    emp::PRG &rng_D1_comp() { return _rng_D1_comp; }
    emp::PRG &rng_D() { return _rng_D; }
    emp::PRG &rng_self() { return _rng_self; }
};
