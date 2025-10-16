#pragma once

#include "function.h"

class Input : public Function {
   public:
    Input(size_t f_id, ProtocolConfig *conf, std::vector<Ring> *input_to_val, std::vector<Ring> output)
        : Function(f_id, conf, {}, {}, {}, output, false), input_to_val(input_to_val) {}

    void preprocess() override {}

    void evaluate_send() override {
        for (size_t i = 0; i < size; ++i) {
            output[i] = input_to_val->at(output[i]);
        }
    }

    void evaluate_recv() override {}

   private:
    std::vector<Ring> *input_to_val;
};