#pragma once

#include "mul.h"

class EQZ : public Mul {
   public:
    EQZ(size_t f_id, ProtocolConfig *conf, std::unordered_map<Party, std::vector<Ring>> *preproc_vals, std::vector<Ring> *online_vals, std::vector<Ring> input,
        std::vector<Ring> output, Party &recv, size_t size, size_t layer, FileWriter *preproc_disk, FileWriter *triples_disk)
        : Mul(f_id, conf, preproc_vals, online_vals, input, {}, output, recv, true, size, preproc_disk, triples_disk), layer(layer) {}

    void evaluate_send() override {
        if (ssd) {
            triples_a = triples_disk->read(size);
            triples_b = triples_disk->read(size);
            if (id == read)
                triples_c = preproc_disk->read(size);
            else
                triples_c = triples_disk->read(size);
        }

        /* x_0 == -x_1 <=> ~x_0 ^ -x_1 */
        if (id == P0 && layer == 0) {
#pragma omp parallel for if (size > 10000)
            for (auto &elem : input) elem = ~elem;
        }

        if (id == P1 && layer == 0) {
#pragma omp parallel for if (size > 10000)
            for (auto &elem : input) elem = -elem;
        }

        std::vector<Ring> shares_left(input);
        std::vector<Ring> shares_right(input);

        size_t width = 1 << (4 - layer);

#pragma omp parallel for if (size > 10000)
        for (auto &elem : shares_left) elem >>= width;

        size_t old = online_vals->size();
        online_vals->resize(old + 2 * size);
        auto send_ptr = online_vals->data() + old;

#pragma omp parallel for if (size > 10000)
        for (size_t i = 0; i < size; ++i) {
            Ring a = triples_a[i];
            Ring b = triples_b[i];
            auto xa = shares_left[i] ^ a;
            auto yb = shares_right[i] ^ b;
            send_ptr[2 * i] = xa;
            send_ptr[2 * i + 1] = yb;
        }
    }

    void evaluate_recv() override {
        std::vector<Ring> data_recv = read_online(2 * size);

#pragma omp parallel for if (size > 10000)
        for (size_t i = 0; i < size; ++i) {
            Ring a = triples_a[i];
            Ring b = triples_b[i];
            Ring c = triples_c[i];

            auto xa = data_recv[2 * i];
            auto yb = data_recv[2 * i + 1];

            output[i] = (xa & yb) * (id) ^ xa & b ^ yb & a ^ c;
        }
        if (layer == 4) {
#pragma omp parallel for if (size > 10000)
            for (auto &elem : output) {
                elem <<= 31;
                elem >>= 31;
            }
        }
    }

   private:
    size_t layer;
};