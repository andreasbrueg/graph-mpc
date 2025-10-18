#pragma once

#include "../io/disk.h"
#include "../utils/sharing.h"
#include "../utils/types.h"

class Function {
   public:
    Function(FType type, size_t f_id, std::vector<Ring> output) : type(type), f_id(f_id), output(output) {}

    Function(FType type, size_t f_id, Ring in1, Ring in2, Ring output) : type(type), f_id(f_id), _in1(std::move(in1)), _in2(std::move(in2)), _output(output) {}

    Function(FType type, size_t f_id, std::vector<Ring> in1, std::vector<Ring> output) : type(type), f_id(f_id), in1(std::move(in1)), output(output) {}

    Function(FType type, size_t f_id, std::vector<Ring> in1, std::vector<Ring> output, bool inverse)
        : type(type), f_id(f_id), in1(std::move(in1)), output(output), inverse(inverse) {}

    Function(FType type, size_t f_id, std::vector<Ring> in1, std::vector<Ring> output, size_t idx)
        : type(type), f_id(f_id), in1(std::move(in1)), output(output) {
        if (type == Shuffle || type == Unshuffle || type == MergedShuffle) {
            shuffle_idx = idx;
        } else if (type == Bit2A || type == Compaction) {
            triples_idx = idx;
            binary = false;
        }
    }

    Function(FType type, size_t f_id, std::vector<Ring> in1, std::vector<Ring> output, size_t n1, size_t n2, size_t n3)
        : type(type), f_id(f_id), in1(std::move(in1)), output(output) {
        if (type == MergedShuffle) {
            shuffle_idx = n1;
            pi_idx = n2;
            omega_idx = n3;
        } else if (type == EQZ) {
            size = n1;
            layer = n2;
            triples_idx = n3;
            binary = true;
        }
    }

    Function(FType type, size_t f_id, std::vector<Ring> in1, Ring in2, std::vector<Ring> output)
        : type(type), f_id(f_id), in1(std::move(in1)), _in2(std::move(in2)), output(output) {}

    Function(FType type, size_t f_id, std::vector<Ring> in1, std::vector<Ring> in2, std::vector<Ring> output)
        : type(type), f_id(f_id), in1(std::move(in1)), in2(std::move(in2)), output(output) {}

    Function(FType type, size_t f_id, std::vector<Ring> in1, std::vector<Ring> in2, std::vector<Ring> output, size_t triples_idx, bool binary)
        : type(type), f_id(f_id), in1(std::move(in1)), in2(std::move(in2)), output(output), triples_idx(triples_idx), binary(binary) {}

    Function(FType type, size_t f_id, std::vector<Ring> in1, std::vector<Ring> in2, std::vector<Ring> output, size_t size, size_t triples_idx, bool binary)
        : type(type), f_id(f_id), in1(std::move(in1)), in2(std::move(in2)), output(output), size(size), triples_idx(triples_idx), binary(binary) {}

    virtual ~Function() = default;

    inline bool interactive() {
        return type == Shuffle || type == MergedShuffle || type == Unshuffle || type == Compaction || type == Mul || type == EQZ || type == Bit2A ||
               type == Reveal;
    }

    FType type;
    size_t f_id;
    std::vector<Ring> in1;
    std::vector<Ring> in2;
    Ring _in1;
    Ring _in2;
    std::vector<Ring> output;
    Ring _output;

    size_t size;
    size_t layer;

    size_t shuffle_idx;
    size_t pi_idx;
    size_t omega_idx;
    size_t triples_idx;

    bool inverse;
    bool binary;
};
