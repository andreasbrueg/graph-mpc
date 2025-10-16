#pragma once

#include <algorithm>
#include <cmath>   // std::log2, std::ceil
#include <memory>  // std::unique_ptr, std::make_unique
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <vector>

#include "../io/netmp.h"
#include "../setup/configs.h"
#include "../utils/graph.h"
#include "../utils/structs.h"
#include "add.h"
#include "add_const.h"
#include "bit2a.h"
#include "compaction.h"
#include "construct_payload.h"
#include "deduplication.h"
#include "equals_zero.h"
#include "flip.h"
#include "function.h"
#include "gather.h"
#include "input.h"
#include "merged_shuffle.h"
#include "permute.h"
#include "propagate.h"
#include "reveal.h"
#include "shuffle.h"
#include "unshuffle.h"

using json = nlohmann::json;

class MPProtocol {
   public:
    MPProtocol(ProtocolConfig &conf, std::shared_ptr<io::NetIOMP> &network)
        : conf(conf),
          id(conf.id),
          size(conf.size),
          nodes(conf.nodes),
          depth(conf.depth),
          bits(static_cast<size_t>(std::ceil(std::log2(static_cast<double>(nodes) + 2.0)))),
          rngs(conf.rngs),
          weights(conf.weights),
          network(network),
          ssd(conf.ssd) {
        w.src_order_bits.resize(bits + 1);
        w.dst_order_bits.resize(bits + 1);
        w.isV_inv.resize(size);
        w.data.resize(size);
        reset();
    }

    virtual void pre_mp() = 0;
    virtual void apply() = 0;
    virtual void post_mp() = 0;
    virtual void compute_sorts();  // Can be overwritten

    void build();
    void preprocess();
    void evaluate();

    void benchmark_graph();
    void set_input(Graph &graph);
    Graph get_output();

    void print();
    json details();

   protected:
    ProtocolConfig conf;
    Party id;
    size_t size, nodes, depth, bits;
    RandomGenerators rngs;
    std::vector<Ring> weights;
    std::shared_ptr<io::NetIOMP> network;
    bool ssd;

    Graph g;
    std::vector<Ring> data;

    std::vector<Ring> input_to_val;

    std::vector<std::vector<std::shared_ptr<Function>>> circ;
    std::vector<std::shared_ptr<Function>> f_queue;
    std::vector<std::shared_ptr<ShufflePre>> shuffles;

    size_t shuffle_idx = 0;
    size_t current_layer = 0;
    size_t num_wires = 0;

    MPContext ctx;
    Wires w;

    FileWriter preproc_disk;
    FileWriter triples_disk;

    Party recv_shuffle = P0;
    Party recv_mul = P0;

    long long comm_ms = 0;
    long long sr_ms = 0;

    void online_communication();

    void prepare_shuffles();

    void message_passing();

    void init_inputs();

    void reset();

    std::vector<Ring> add_sort(std::vector<std::vector<Ring>> &bit_keys, size_t bits);

    std::vector<Ring> add_sort_iteration(std::vector<Ring> &perm, std::vector<Ring> &keys);

    void build_circuit();

    void add_function(std::shared_ptr<Function> func);

    std::vector<Ring> add_input();

    std::vector<Ring> propagate_1(std::vector<Ring> &input);

    std::vector<Ring> propagate_2(std::vector<Ring> &input1, std::vector<Ring> &input2);

    std::vector<Ring> gather_1(std::vector<Ring> &input);

    std::vector<Ring> gather_2(std::vector<Ring> &input);

    std::vector<Ring> shuffle(std::vector<Ring> &input, size_t shuffle_idx);

    std::vector<Ring> unshuffle(std::vector<Ring> &input, size_t shuffle_idx);

    std::vector<Ring> merged_shuffle(std::vector<Ring> &input, size_t pi_idx, size_t omega_idx, size_t shuffle_idx);

    std::vector<Ring> compaction(std::vector<Ring> &input);

    std::vector<Ring> reveal(std::vector<Ring> &input);

    std::vector<Ring> permute(std::vector<Ring> &input, std::vector<Ring> &perm, bool inverse = false);

    std::vector<Ring> equals_zero(std::vector<Ring> &input, size_t size, size_t layer);

    std::vector<Ring> bit2A(std::vector<Ring> &input, size_t size);

    std::vector<Ring> insert(std::vector<Ring> input);

    std::vector<Ring> deduplication_sub(std::vector<Ring> &vec_p);

    std::vector<Ring> mul(std::vector<Ring> &x, std::vector<Ring> &y, bool binary = false);

    std::vector<Ring> mul(std::vector<Ring> &x, std::vector<Ring> &y, size_t size, bool binary = false);

    std::vector<Ring> push_back(std::vector<std::vector<Ring>> &keys, std::vector<Ring> &vec);

    std::vector<Ring> flip(std::vector<Ring> &input);

    std::vector<Ring> add(std::vector<Ring> &input1, std::vector<Ring> &input2);

    std::vector<Ring> add_const(std::vector<Ring> &data, Ring val);

    std::vector<Ring> construct_payload(std::vector<std::vector<Ring>> &payloads);

    void add_deduplication();

    void add_clip();
};