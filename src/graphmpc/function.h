#pragma once

#include "../io/disk.h"
#include "../utils/sharing.h"
#include "../utils/types.h"

class Function {
   public:
    /* Used by Input */
    Function(FType type, size_t f_id, size_t out_idx);

    /* Used by ConstructData */
    Function(FType type, size_t f_id, size_t in1_idx, size_t out_idx, std::vector<size_t> &data_parallel);

    /* Used by Output, Propagate-1, Gather-1, Gather-2, Reveal, Sub */
    Function(FType type, size_t f_id, size_t in1_idx, size_t out_idx);

    /* Used by AddConst */
    Function(FType type, size_t f_id, size_t in1_idx, Ring val, size_t out_idx);

    Function(FType type, size_t f_id, size_t in1_idx, size_t out_idx, bool inverse);

    /* Used by Propagate-2, Shuffle, Unshuffle, Bit2A, Compaction, Add, Sub */
    Function(FType type, size_t f_id, size_t param1, size_t param2, size_t param3);

    /* Used by Bit2A */
    Function(FType type, size_t f_id, size_t param1, size_t param2, size_t param3, size_t param4);

    /* Used by MergedShuffle and EQZ */
    Function(FType type, size_t f_id, size_t in1_idx, size_t out_idx, size_t n1, size_t n2, size_t n3);

    /* Used by Mul */
    Function(FType type, size_t f_id, size_t in1_idx, size_t in2_idx, size_t out_idx, size_t size, size_t mult_idx, bool binary);

    bool interactive();

    FType type;
    size_t f_id;
    size_t in1_idx;
    size_t in2_idx;
    Ring val;
    size_t out_idx;

    size_t size;
    size_t layer;

    size_t shuffle_idx;
    size_t pi_idx;
    size_t omega_idx;
    size_t mult_idx;

    bool inverse;
    bool binary;

    std::vector<size_t> data_parallel;
};
