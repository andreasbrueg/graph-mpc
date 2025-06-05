#pragma once

#include <memory>
#include <queue>

#include "io/comm.h"
#include "io/netmp.h"
#include "protocol/clip.h"
#include "protocol/compaction.h"
#include "protocol/message_passing.h"
#include "protocol/shuffle.h"
#include "protocol/sorting.h"
#include "utils/graph.h"
#include "utils/random_generators.h"
#include "utils/sharing.h"
#include "utils/types.h"

enum FunctionToken { COMPACTION, SHUFFLE, UNSHUFFLE, SORT, SORT_ITERATION, SWITCH_PERM, APPLY_PERM };

struct Preprocessing {
    std::vector<std::vector<std::tuple<Ring, Ring, Ring>>> compaction_triples;
    std::vector<ShufflePre> shuffle_shares;
    std::vector<std::vector<Ring>> unshuffle_Bs;
    std::vector<SortPreprocessing_Dealer> sort_preproc_1;
    std::vector<SortPreprocessing> sort_preproc_2;
    std::vector<SwitchPermPreprocessing> sw_perm_preproc;
};

class Processor {
   public:
    Processor(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, const size_t BLOCK_SIZE)
        : id(id), rngs(rngs), network(network), n(n), BLOCK_SIZE(BLOCK_SIZE) {}

    void add(FunctionToken function);
    void set_graph(SecretSharedGraph &graph) { g = graph; }
    void set_wire(std::vector<Ring> &input) { wire = input; }
    void set_sort_wire(std::vector<std::vector<Ring>> &input) { sort_wire = input; }

    void run_mp_preprocessing(size_t n_iterations);
    void run_mp_evaluation(size_t n_iterations, size_t n_vertices);

    Preprocessing get_preprocessing() { return preproc; }
    std::vector<Ring> get_wire() { return wire; }
    std::vector<Ring> get_wire_clear() { return share::reveal_vec(id, network, BLOCK_SIZE, wire); }
    SecretSharedGraph get_graph() { return g; }

   private:
    Party id;
    RandomGenerators rngs;
    std::shared_ptr<io::NetIOMP> network;
    size_t n;
    const size_t BLOCK_SIZE;

    SecretSharedGraph g;
    std::vector<Ring> wire;
    std::vector<std::vector<Ring>> sort_wire;

    std::vector<FunctionToken> queue;
    Preprocessing preproc;
    MPPreprocessing mp_preproc;

    ShufflePre last_shuffle;
    bool pre_completed = false;

    size_t preprocessing_data_size(Party id);
    void run_preprocessing_dealer();
    void run_preprocessing_parties();

    /*
     void run_evaluation_1(std::vector<Ring> &send);
     void run_evaluation_2(std::vector<Ring> &vals);
     size_t evaluation_data_size();
     std::vector<Ring> add_vals(std::vector<Ring> &sent, std::vector<Ring> &received);
     */
};