#pragma once

#include "mul.h"

class Compaction : public Mul {
   public:
    Compaction(ProtocolConfig *conf, std::unordered_map<Party, std::vector<Ring>> *preproc_vals, std::vector<Ring> *online_vals, std::vector<Ring> *input,
               std::vector<Ring> *output, Party &recv)
        : Mul(conf, preproc_vals, online_vals, input, {}, output, recv, false) {}

    void evaluate_send() override {
        if (id == D) return;

        std::vector<Ring> f_0;

        // set f_0 to 1 - input and f_1 to input, we just immediately use input instead of f_1
        for (size_t i = 0; i < size; ++i) {
            f_0.push_back(-input->at(i));
            if (id == P0) {
                f_0[i] += 1;  // 1 as constant only to one share
            }
        }

        std::vector<Ring> s_0, s_1;
        Ring s = 0;
        // Set s_0 to prefix sum of f_0 and s_1 to prefix sum of f_1/input continuing from the prior final value
        for (size_t i = 0; i < size; ++i) {
            s += f_0[i];
            s_0.push_back(s);
        }

        for (size_t i = 0; i < size; ++i) {
            s += input->at(i);
            s_1.push_back(s - s_0[i]);
        }

        // #pragma omp parallel for if (size > 10000)
        for (size_t i = 0; i < size; ++i) {
            auto [a, b, _] = triples[i];
            auto xa = input->at(i) + a;
            auto yb = s_1[i] + b;
            online_vals->push_back(xa);
            online_vals->push_back(yb);
        }
        return;
    }

    void evaluate_recv() override {
        std::vector<Ring> result(size);
        std::vector<Ring> f_0;
        // set f_0 to 1 - input and f_1 to input, we just immediately use input instead of f_1
        for (size_t i = 0; i < size; ++i) {
            f_0.push_back(-input->at(i));
            if (id == P0) {
                f_0[i] += 1;  // 1 as constant only to one share
            }
        }

        std::vector<Ring> s_0;
        Ring s = 0;
        // Set s_0 to prefix sum of f_0 and s_1 to prefix sum of f_1/input continuing from the prior final value
        for (size_t i = 0; i < size; ++i) {
            s += f_0[i];
            s_0.push_back(s);
        }

        auto data_recv = read_online(2 * size);

        // #pragma omp parallel for if (size > 10000)
        for (size_t i = 0; i < size; ++i) {
            auto [a, b, mul] = triples[i];

            auto xa = data_recv[2 * i];
            auto yb = data_recv[2 * i + 1];

            result[i] = (xa * yb * (id)) - (xa * b) - (yb * a) + mul;
        }

        // #pragma omp parallel for if (size > 10000)
        for (size_t i = 0; i < output->size(); ++i) {
            result[i] += s_0[i];
            if (id == P0) result[i]--;
        }
        *output = result;
    }
};