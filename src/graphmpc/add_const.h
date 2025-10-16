#pragma once

#include <cassert>

#include "function.h"

class AddConst : public Function {
   public:
    AddConst(size_t f_id, ProtocolConfig *conf, std::vector<Ring> data_v, Ring val, std::vector<Ring> output)
        : Function(f_id, conf, {}, {}, data_v, {}, output, false), nodes(conf->nodes), val(val) {}

    void preprocess() override {}

    void evaluate_send() override {}

    void evaluate_recv() override {
        if (id == P0) {
#pragma omp parallel for if (nodes > 10000)
            for (size_t j = 0; j < nodes; ++j) output[j] = input[j] + val;
        }
    }

   private:
    size_t nodes;
    Ring val;
};