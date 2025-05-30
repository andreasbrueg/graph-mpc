#pragma once

#include <emp-tool/emp-tool.h>

#include <cstdint>
#include <random>

#include "types.h"

class RandomGenerators {
    emp::PRG _rng_01;
    emp::PRG _rng_D0;
    emp::PRG _rng_D0_unshuffle;
    emp::PRG _rng_D1;
    emp::PRG _rng_D1_unshuffle;
    emp::PRG _rng_D;
    emp::PRG _rng_self;

   public:
    RandomGenerators(uint64_t seeds_hi[7], uint64_t seeds_lo[7]);
    emp::PRG &rng_01();
    emp::PRG &rng_D0();
    emp::PRG &rng_D0_unshuffle();
    emp::PRG &rng_D1();
    emp::PRG &rng_D1_unshuffle();
    emp::PRG &rng_D();
    emp::PRG &rng_self();
};