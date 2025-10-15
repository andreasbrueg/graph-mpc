#pragma once

#include "function.h"

class Gather_1 : public Function {
   public:
    Gather_1(size_t f_id, ProtocolConfig *conf, std::vector<Ring> input, std::vector<Ring> output) : Function(f_id, conf, {}, {}, input, output) {}

    void preprocess() override {}

    void evaluate_send() override {}

    void evaluate_recv() override {
        Ring sum = 0;
        for (size_t i = 0; i < size; ++i) {
            sum += input[i];
            output[i] = sum;
        }
    }
};

class Gather_2 : public Function {
   public:
    Gather_2(size_t f_id, ProtocolConfig *conf, std::vector<Ring> input, std::vector<Ring> output)
        : Function(f_id, conf, {}, {}, input, output), nodes(conf->nodes) {}

    void preprocess() override {}

    void evaluate_send() override {}

    void evaluate_recv() override {
        Ring sum = 0;

        for (size_t i = 0; i < nodes; ++i) {
            output[i] = input[i] - sum;
            sum += output[i];
        }
#pragma omp_parallel for if (size - nodes > 10000)
        for (size_t i = nodes; i < size; ++i) {
            output[i] = 0;
        }
    }

   private:
    size_t nodes;
};