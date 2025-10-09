#pragma once

#include "mul.h"

class EQZ : public Mul {
   public:
    EQZ(ProtocolConfig *conf, std::unordered_map<Party, std::vector<Ring>> *preproc_vals, std::vector<Ring> *online_vals, std::vector<Ring> *input,
        std::vector<Ring> *output, Party &recv, size_t size, size_t layer)
        : Mul(conf, preproc_vals, online_vals, input, nullptr, output, recv, true, size), layer(layer) {}

    void evaluate_send() override {
        std::vector<Ring> result(*input);

        /* x_0 == -x_1 <=> ~x_0 ^ -x_1 */
        if (id == P0 && layer == 0) {
#pragma omp parallel for if (size > 10000)
            for (auto &elem : result) elem = ~elem;
        }

        if (id == P1 && layer == 0) {
#pragma omp parallel for if (size > 10000)
            for (auto &elem : result) elem = -elem;
        }

        std::vector<Ring> shares_left(result);
        std::vector<Ring> shares_right(result);

        size_t width = 1 << (4 - layer);

#pragma omp parallel for if (size > 10000)
        for (auto &elem : shares_left) elem >>= width;

        // #pragma omp parallel for if (size > 10000)
        for (size_t i = 0; i < size; ++i) {
            auto [a, b, _] = triples[i];
            auto xa = shares_left[i] ^ a;
            auto yb = shares_right[i] ^ b;
            online_vals->push_back(xa);
            online_vals->push_back(yb);
        }
    }

    void evaluate_recv() override {
        std::vector<Ring> result(size);
        std::vector<Ring> data_recv = read_online(2 * size);

        // #pragma omp parallel for if (size > 10000)
        for (size_t i = 0; i < size; ++i) {
            auto [a, b, mul] = triples[i];

            auto xa = data_recv[2 * i];
            auto yb = data_recv[2 * i + 1];

            result[i] = (xa & yb) * (id) ^ xa & b ^ yb & a ^ mul;
        }
        if (layer == 4) {
#pragma omp parallel for if (size > 10000)
            for (auto &elem : result) {
                elem <<= 31;
                elem >>= 31;
            }
        }
        *output = result;
    }

   private:
    size_t layer;
};