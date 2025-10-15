#pragma once

#include "../utils/graph.h"
#include "function.h"

class DeduplicationSub : public Function {
   public:
    DeduplicationSub(size_t f_id, ProtocolConfig *conf, std::vector<Ring> vec_p, std::vector<Ring> vec_dupl) : Function(f_id, conf, {}, {}, vec_p, vec_dupl) {}

    void preprocess() override {}

    void evaluate_send() override {}

    void evaluate_recv() override {
        for (size_t i = 1; i < size; ++i) {
            output[i - 1] = input[i] - input[i - 1];
        }
    }
};

class Insert : public Function {
   public:
    Insert(size_t f_id, ProtocolConfig *conf, std::vector<Ring> input) : Function(f_id, conf, {}, {}, input, {}) {}

    void preprocess() override {}

    void evaluate_send() override {}

    void evaluate_recv() override {
        output = input;
        output.insert(output.begin(), 0);
    }
};

class PushBack : public Function {
   public:
    PushBack(size_t f_id, ProtocolConfig *conf, std::vector<std::vector<Ring>> *keys, std::vector<Ring> input)
        : Function(f_id, conf, {}, {}, input, {}, {}), keys(keys) {}
    void preprocess() override {}

    void evaluate_send() override {}

    void evaluate_recv() override { keys->at(keys->size() - 1) = {input.begin(), input.end()}; }

   private:
    std::vector<std::vector<Ring>> *keys;
};