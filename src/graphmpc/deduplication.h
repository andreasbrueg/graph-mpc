#pragma once

#include "../utils/graph.h"
#include "function.h"

class DeduplicationSub : public Function {
   public:
    DeduplicationSub(ProtocolConfig *conf, std::vector<Ring> *vec_p, std::vector<Ring> *vec_dupl) : Function(conf, {}, {}, vec_p, vec_dupl) {}

    void preprocess() override {}

    void evaluate_send() override {}

    void evaluate_recv() override {
        std::vector<Ring> result(output->size());
        for (size_t i = 1; i < size; ++i) {
            result[i - 1] = input->at(i) - input->at(i - 1);
        }
        *output = result;
    }
};

class Insert : public Function {
   public:
    Insert(ProtocolConfig *conf, std::vector<Ring> *input) : Function(conf, {}, {}, input, {}) {}

    void preprocess() override {}

    void evaluate_send() override {}

    void evaluate_recv() override { input->insert(input->begin(), 0); }
};

class PushBack : public Function {
   public:
    PushBack(ProtocolConfig *conf, std::vector<std::vector<Ring>> *keys, std::vector<Ring> *input) : Function(conf, {}, {}, input, {}, {}), keys(keys) {}
    void preprocess() override {}

    void evaluate_send() override {}

    void evaluate_recv() override { keys->at(keys->size() - 1) = {input->begin(), input->end()}; }

   private:
    std::vector<std::vector<Ring>> *keys;
};