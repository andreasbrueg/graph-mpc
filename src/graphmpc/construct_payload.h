#pragma once

#include "function.h"

class ConstructPayload : public Function {
   public:
    ConstructPayload(size_t f_id, ProtocolConfig *conf, std::vector<std::vector<Ring>> *payloads, std::vector<Ring> output)
        : Function(f_id, conf, {}, {}, {}, {}, output, false), payloads(payloads), nodes(conf->nodes) {}

    void preprocess() override {}

    void evaluate_send() override {}

    void evaluate_recv() override {
        for (size_t i = 0; i < payloads->size(); ++i) {
            auto sum = payloads->at(i)[0];
            for (size_t j = 1; j < nodes; ++j) {
                sum += payloads->at(i)[j];
            }
            output[i] = sum;
        }
    }

   private:
    std::vector<std::vector<Ring>> *payloads;
    size_t nodes;
};