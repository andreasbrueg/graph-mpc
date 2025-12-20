#pragma once

#include "../io/disk.h"
#include "../utils/sharing.h"
#include "../utils/types.h"

enum GType {
    Input,
    Output,
    Shuffle,
    Unshuffle,
    MergedShuffle,
    Compaction,
    Reveal,
    Permute,
    ReversePermute,
    AddConst,
    AddConstSIMD,
    MulConst,
    MulConstSIMD,
    Add,
    AddSIMD,
    Sub,
    SubSIMD,
    Mul,
    MulSIMD,
    And,
    AndSIMD,
    Xor,
    XorSIMD,
    DeduplicationSub,
    Bit2A,
    EQZ,
    Gather1,
    Gather2,
    Propagate1,
    Propagate2,
    Flip,
    Insert,
    ConstructData
};

class Gate {
   public:
    /* Used by Input */
    Gate(GType type, size_t g_id, wire_id out_idx);

    /* Used by ConstructData */
    Gate(GType type, size_t g_id, wire_id in1_idx, wire_id out_idx, std::vector<wire_id> &data_parallel);

    /* Used by Output, Propagate-1, Gather-1, Gather-2, Reveal, Sub */
    Gate(GType type, size_t g_id, wire_id in1_idx, wire_id out_idx);

    /* Used by AddConst, AddConstSIMD, MulConst, MulConstSIMD */
    Gate(GType type, size_t g_id, wire_id in1_idx, Ring val, wire_id out_idx);

    /* Used by Propagate-2, Shuffle, Unshuffle, Bit2A, Compaction, Add, Sub */
    Gate(GType type, size_t g_id, size_t param1, size_t param2, size_t param3);

    /* Used by Bit2A */
    Gate(GType type, size_t g_id, size_t param1, size_t param2, size_t param3, size_t param4);

    /* Used by MergedShuffle and EQZ */
    Gate(GType type, size_t g_id, wire_id in1_idx, wire_id out_idx, size_t n1, size_t n2, size_t n3);

    /* Used by Mul */
    Gate(GType type, size_t g_id, wire_id in1_idx, wire_id in2_idx, wire_id out_idx, size_t size, size_t mult_idx, bool binary);

    bool interactive();

    GType type;
    size_t g_id;
    wire_id in1_idx;
    wire_id in2_idx;
    Ring val;
    wire_id out_idx;

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
