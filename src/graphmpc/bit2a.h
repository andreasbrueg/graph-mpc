#pragma once

#include "mul.h"

class Bit2A : public Mul {
   public:
    Bit2A(ProtocolConfig *conf, std::unordered_map<Party, std::vector<Ring>> *preproc_vals, std::vector<Ring> *online_vals, std::vector<Ring> *input,
          std::vector<Ring> *output, Party &recv, size_t size)
        : Mul(conf, preproc_vals, online_vals, input, nullptr, output, recv, false, size) {}

    void evaluate_send() override {
        size_t old = online_vals->size();
        online_vals->resize(old + 2 * size);
        auto send_ptr = online_vals->data() + old;

#pragma omp parallel for if (size > 10000)
        for (size_t i = 0; i < size; ++i) {
            Ring a = triples_a[i];
            Ring b = triples_b[i];
            auto xa = (id == P0 ? 1 : 0) * (input->at(i) & 1) + a;
            auto yb = (id == P0 ? 0 : 1) * (input->at(i) & 1) + b;
            send_ptr[2 * i] = xa;
            send_ptr[2 * i + 1] = yb;
        }
    }

    void evaluate_recv() override {
        std::vector<Ring> data_recv = read_online(2 * size);
        auto outptr = output->data();
#pragma omp parallel for if (size > 10000)
        for (size_t i = 0; i < size; ++i) {
            Ring a = triples_a[i];
            Ring b = triples_b[i];
            Ring c = triples_c[i];
            auto xa = data_recv[2 * i];
            auto yb = data_recv[2 * i + 1];

            auto mul_result = (xa * yb * (id)) - (xa * b) - (yb * a) + c;
            outptr[i] = (input->at(i) & 1) - 2 * mul_result;
        }
    }
};