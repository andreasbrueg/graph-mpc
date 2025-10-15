#pragma once

#include "function.h"

class Propagate_1 : public Function {
   public:
    Propagate_1(size_t f_id, ProtocolConfig *conf, std::vector<Ring> input, std::vector<Ring> output)
        : Function(f_id, conf, {}, {}, input, output), nodes(conf->nodes) {}

    void preprocess() override {}

    void evaluate_send() override {}

    void evaluate_recv() override {
        for (size_t i = nodes - 1; i > 0; --i) {
            output[i] = input[i] - input[i - 1];
        }
        output[0] = input[0];
        for (size_t i = nodes; i < size; ++i) {
            output[i] = input[i];
        }
    }

   private:
    size_t nodes;
};

class Propagate_2 : public Function {
   public:
    Propagate_2(size_t f_id, ProtocolConfig *conf, std::vector<Ring> input1, std::vector<Ring> input2, std::vector<Ring> output)
        : Function(f_id, conf, {}, {}, input1, input2, output) {}

    void preprocess() override {}

    void evaluate_send() override {}

    void evaluate_recv() override {
        Ring sum = 0;
        for (size_t i = 0; i < size; ++i) {
            sum += input[i];
            output[i] = sum - input2[i];
        }
    }
};