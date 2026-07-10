#pragma once

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wvla-cxx-extension"
#include <emp-tool/emp-tool.h>
#pragma GCC diagnostic pop

#include <cstdint>
#include <random>

#include "types.h"

class RandomGenerators {
    // For online parties:
    emp::PRG _rng_01;
    emp::PRG _rng_with_D_prep;
    emp::PRG _rng_with_D_send;
    emp::PRG _rng_with_D_recv;
    // For dealer:
    emp::PRG _rng_with_P0_prep;
    emp::PRG _rng_with_P0_send;
    emp::PRG _rng_with_P0_recv;
    emp::PRG _rng_with_P1_prep;
    emp::PRG _rng_with_P1_send;
    emp::PRG _rng_with_P1_recv;
    // For any party/dealer:
    emp::PRG _rng_self;

    // Just for assertions to check that no RNGs are accessed before proper initialization and
    // that they are not initialized multiple times.
    bool i_am_D = false;
    bool i_am_P0 = false;
    bool i_am_P1 = false;

   public:

    RandomGenerators() = default;

    std::tuple<std::vector<uint64_t>, std::vector<uint64_t>, std::vector<uint64_t>, std::vector<uint64_t>> dealer_create_rngs() {
        assert(!i_am_D && !i_am_P0 && !i_am_P1);
        i_am_D = true;

        // _rng_self runs on cryptographically secure random seed sampled by its constructor

        std::vector<uint64_t> seeds_hi_P0(3);
        std::vector<uint64_t> seeds_lo_P0(3);
        std::vector<uint64_t> seeds_hi_P1(3);
        std::vector<uint64_t> seeds_lo_P1(3);

        _rng_self.random_data(seeds_hi_P0.data(), 3 * sizeof(uint64_t));
        _rng_self.random_data(seeds_lo_P0.data(), 3 * sizeof(uint64_t));
        _rng_self.random_data(seeds_hi_P1.data(), 3 * sizeof(uint64_t));
        _rng_self.random_data(seeds_lo_P1.data(), 3 * sizeof(uint64_t));

        auto seed_block = emp::makeBlock(seeds_hi_P0[0], seeds_lo_P0[0]);
        _rng_with_P0_prep.reseed(&seed_block, 0);
        seed_block = emp::makeBlock(seeds_hi_P0[1], seeds_lo_P0[1]);
        _rng_with_P0_send.reseed(&seed_block, 0);
        seed_block = emp::makeBlock(seeds_hi_P0[2], seeds_lo_P0[2]);
        _rng_with_P0_recv.reseed(&seed_block, 0);

        seed_block = emp::makeBlock(seeds_hi_P1[0], seeds_lo_P1[0]);
        _rng_with_P1_prep.reseed(&seed_block, 0);
        seed_block = emp::makeBlock(seeds_hi_P1[1], seeds_lo_P1[1]);
        _rng_with_P1_send.reseed(&seed_block, 0);
        seed_block = emp::makeBlock(seeds_hi_P1[2], seeds_lo_P1[2]);
        _rng_with_P1_recv.reseed(&seed_block, 0);

        return std::make_tuple(seeds_hi_P0, seeds_lo_P0, seeds_hi_P1, seeds_lo_P1);
    }

    void party_create_rngs(std::vector<uint64_t> seeds_hi, std::vector<uint64_t> seeds_lo) {
        assert(!i_am_D && !i_am_P0 && !i_am_P1);

        // _rng_self runs on cryptographically secure random seed sampled by its constructor

        auto seed_block = emp::makeBlock(seeds_hi[0], seeds_lo[0]);
        _rng_with_D_prep.reseed(&seed_block, 0);
        seed_block = emp::makeBlock(seeds_hi[1], seeds_lo[1]);
        _rng_with_D_send.reseed(&seed_block, 0);
        seed_block = emp::makeBlock(seeds_hi[2], seeds_lo[2]);
        _rng_with_D_recv.reseed(&seed_block, 0);
    }

    std::tuple<uint64_t, uint64_t> p0_create_rng_01() {
        assert(!i_am_D && !i_am_P0 && !i_am_P1);
        i_am_P0 = true;

        std::vector<uint64_t> seed(2);
        _rng_self.random_data(seed.data(), 2 * sizeof(uint64_t));
        auto seed_block = emp::makeBlock(seed[0], seed[1]);
        _rng_01.reseed(&seed_block, 0);
        return std::make_tuple(seed[0], seed[1]);
    }

    void p1_create_rng01(uint64_t seed_hi, uint64_t seed_lo) {
        assert(!i_am_D && !i_am_P0 && !i_am_P1);
        i_am_P1 = true;

        auto seed_block = emp::makeBlock(seed_hi, seed_lo);
        _rng_01.reseed(&seed_block, 0);
    }

    emp::PRG &rng_01() {
        assert(i_am_P0 || i_am_P1);
        return _rng_01;
    }

    emp::PRG &rng_with_D_prep() {
        assert(i_am_P0 || i_am_P1);
        return _rng_with_D_prep;
    }

    emp::PRG &rng_with_D_send() {
        assert(i_am_P0 || i_am_P1);
        return _rng_with_D_send;
    }

    emp::PRG &rng_with_D_recv() {
        assert(i_am_P0 || i_am_P1);
        return _rng_with_D_recv;
    }

    emp::PRG &rng_with_P0_prep() {
        assert(i_am_D);
        return _rng_with_P0_prep;
    }

    emp::PRG &rng_with_P0_send() {
        assert(i_am_D);
        return _rng_with_P0_send;
    }

    emp::PRG &rng_with_P0_recv() {
        assert(i_am_D);
        return _rng_with_P0_recv;
    }

    emp::PRG &rng_with_P1_prep() {
        assert(i_am_D);
        return _rng_with_P1_prep;
    }

    emp::PRG &rng_with_P1_send() {
        assert(i_am_D);
        return _rng_with_P1_send;
    }

    emp::PRG &rng_with_P1_recv() {
        assert(i_am_D);
        return _rng_with_P1_recv;
    }

    emp::PRG &rng_self() {
        return _rng_self;
    }

    void reset() {
        i_am_D = false;
        i_am_P0 = false;
        i_am_P1 = false;
    }
};
