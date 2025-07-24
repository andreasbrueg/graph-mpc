#pragma once

#include <functional>

#include "graphmpc/message_passing.h"
#include "graphmpc/shuffle.h"
#include "io/netmp.h"
#include "utils/random_generators.h"
#include "utils/types.h"

class FunctionQueue {
   public:
    FunctionQueue(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc)
        : id(id), rngs(rngs), network(network), n(n), preproc(preproc) {};

    void add_shuffle(std::vector<Ring> &input) {
        preprocessing.push_back([=]() { shuffle::get_shuffle(id, rngs, network, n, preproc); });
        evaluation.push_back([=, &input]() { shuffle::shuffle(id, rngs, network, n, preproc, input); });
    }

    void add_unshuffle(ShufflePre &shuffle_pre, std::vector<Ring> &input) {
        preprocessing.push_back([=, &shuffle_pre]() { shuffle::get_unshuffle(id, rngs, network, n, shuffle_pre, preproc); });
        evaluation.push_back([=, &shuffle_pre, &input]() { shuffle::unshuffle(id, rngs, network, n, preproc, shuffle_pre, input); });
    }

    void add_multiplication(std::vector<Ring> &x, std::vector<Ring> &y) {
        preprocessing.push_back([=]() { mul::preprocess(id, rngs, network, n, preproc); });
        evaluation.push_back([=, &x, &y]() { mul::evaluate(id, network, n, preproc, x, y); });
    }

    void add_sort(std::vector<std::vector<Ring>> &bits) {
        size_t n_bits = bits.size();
        preprocessing.push_back([=]() { sort::get_sort_preprocess(id, rngs, network, n, n_bits, preproc); });
        evaluation.push_back([=, &bits]() { sort::get_sort_evaluate(id, rngs, network, n, preproc, bits); });
    }

    void run() {
        for (auto &func : preprocessing) func();
        for (auto &func : evaluation) func();
    }

   private:
    Party id;
    RandomGenerators &rngs;
    std::shared_ptr<io::NetIOMP> network;
    size_t n;

    MPPreprocessing &preproc;

    std::vector<std::function<void()>> preprocessing;
    std::vector<std::function<void()>> evaluation;
};
