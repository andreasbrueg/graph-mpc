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
#include "add.h"
#include "add_weights.h"
#include "bit2a.h"
#include "compaction.h"
#include "construct_payload.h"
#include "deduplication.h"
#include "equals_zero.h"
#include "flip.h"
#include "function.h"
#include "gather.h"
#include "merged_shuffle.h"
#include "permute.h"
#include "propagate.h"
#include "reveal.h"
#include "shuffle.h"
#include "shuffle_repeat.h"
#include "unshuffle.h"
#include "update.h"

using json = nlohmann::json;

struct MPContext {
    std::unordered_map<Party, std::vector<Ring>> preproc;
    std::vector<Ring> data_recv;
    std::vector<Ring> mult_vals;
    std::vector<Ring> and_vals;
    std::vector<Ring> shuffle_vals;
    std::vector<Ring> reveal_vals;

    std::vector<Ring> vtx_order;
    std::vector<Ring> src_order;
    std::vector<Ring> dst_order;
    std::vector<Ring> clear_shuffled_vtx_order;
    std::vector<Ring> clear_shuffled_src_order;
    std::vector<Ring> clear_shuffled_dst_order;

    ShufflePre vtx_order_shuffle;
    ShufflePre src_order_shuffle;
    ShufflePre dst_order_shuffle;
    ShufflePre vtx_src_merge;
    ShufflePre src_dst_merge;
    ShufflePre dst_vtx_merge;
};

struct Wires {
    std::vector<std::vector<Ring>> src_order_bits;
    std::vector<std::vector<Ring>> dst_order_bits;
    std::vector<Ring> isV;
    std::vector<Ring> data;

    std::vector<std::vector<Ring>> mp_data_parallel;
    std::vector<Ring> mp_data_vtx;
    std::vector<Ring> mp_data;
    std::vector<Ring> mp_data_corr;
    std::vector<Ring> mp_buf;
    std::vector<Ring> sort_perm;
    std::vector<Ring> sort_bits;
    std::vector<Ring> sort_next_perm;
    std::vector<Ring> deduplication_perm;
    std::vector<Ring> deduplication_src;
    std::vector<Ring> deduplication_dst;
    std::vector<Ring> deduplication_src_dupl;
    std::vector<Ring> deduplication_dst_dupl;
    std::vector<Ring> deduplication_duplicates;
};

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
        w.src_order_bits.resize(bits);
        w.dst_order_bits.resize(bits);
        for (size_t i = 0; i < bits; ++i) {
            w.src_order_bits[i].resize(size);
            w.dst_order_bits[i].resize(size);
        }

        for (size_t i = 0; i < bits; ++i) {
            for (size_t j = 0; j < size; ++j) {
                w.src_order_bits[i][j] = num_wires;
                num_wires++;
            }
        }
        for (size_t i = 0; i < bits; ++i) {
            for (size_t j = 0; j < size; ++j) {
                w.dst_order_bits[i][j] = num_wires;
                num_wires++;
            }
        }
        for (size_t i = 0; i < size; ++i) {
            w.isV[i] = num_wires;
            num_wires++;
        }
        for (size_t i = 0; i < size; ++i) {
            w.data[i] = num_wires;
            num_wires++;
        }

        reset();
    }

    virtual void pre_mp() = 0;
    virtual void apply() = 0;
    virtual void post_mp() = 0;

    virtual void add_compute_sorts();  // Can be overwritten

    json details() {
        size_t rounds = f_queue.size();
        json output;
        output["details"] = {
            {"party", id}, {"size", size}, {"nodes", nodes}, {"depth", depth}, {"bits", bits}, {"ssd", ssd}, {"Communication rounds: ", rounds}};
        return output;
    }

    void build() {
        pre_mp();
        build_initialization();
        if (w.mp_data_parallel.size() > 0) {  // Only the case for pi_r
            for (size_t i = 0; i < w.mp_data_parallel.size(); ++i) {
                w.mp_buf = add_update(w.mp_data_parallel[i]);
                w.mp_data = add_update(w.mp_data_parallel[i]);
                build_message_passing();
                w.mp_data_parallel[i] = add_update(w.mp_data);
            }
        } else {
            w.mp_data = add_update(w.mp_data_vtx);
            build_message_passing();
        }
        post_mp();
        g.data = add_update(w.mp_data);
    }

    void build_message_passing();

    void preprocess();

    void evaluate();

    void set_input(Graph &graph) { g = graph; }

    Graph get_output() { return g; }

    void benchmark_graph() {
        if (id == P0) {
            for (size_t i = 0; i < nodes / 2; i++) g.add_list_entry(i + 1, i + 1, 1);
            for (size_t i = 0; i < (size - nodes) / 2; i++) g.add_list_entry(1, 2, 0);
        }
        if (id == P1) {
            for (size_t i = nodes / 2; i < nodes; i++) g.add_list_entry(i + 1, i + 1, 1);
            for (size_t i = (size - nodes) / 2; i < size - nodes; i++) g.add_list_entry(1, 2, 0);
        }
        g.share_subgraphs(id, rngs, network, bits);
        g.init_mp(id);
    }

    void print() {
        std::cout << "----- Protocol Configuration -----" << std::endl;
        std::cout << "Party: " << id << std::endl;
        std::cout << "Size: " << size << std::endl;
        std::cout << "Nodes: " << nodes << std::endl;
        std::cout << "Depth: " << depth << std::endl;
        std::cout << "Bits: " << bits << std::endl;
        std::cout << "SSD utilization: " << ssd << std::endl;
        std::cout << "Communication rounds: " << f_queue.size() << std::endl;
        std::cout << std::endl;
    }

    void reset() {
        std::filesystem::remove("preproc_" + std::to_string(id) + ".bin");
        std::filesystem::remove("triples_" + std::to_string(id) + ".bin");
        preproc_disk = FileWriter(id, "preproc_" + std::to_string(id) + ".bin");
        triples_disk = FileWriter(id, "triples_" + std::to_string(id) + ".bin");

        rngs.reseed();
        recv_shuffle = P0;
        recv_mul = P0;

        current_layer = 0;
        comm_ms = 0;
        sr_ms = 0;

        ctx.preproc[P0].clear();
        ctx.preproc[P1].clear();

        ctx.mult_vals.clear();
        ctx.and_vals.clear();
        ctx.shuffle_vals.clear();
        ctx.reveal_vals.clear();
        ctx.data_recv.clear();

        ctx.vtx_order_shuffle.initialize(id, size);
        ctx.src_order_shuffle.initialize(id, size);
        ctx.dst_order_shuffle.initialize(id, size);
        ctx.vtx_src_merge.initialize(id, size);
        ctx.src_dst_merge.initialize(id, size);
        ctx.dst_vtx_merge.initialize(id, size);
    }

   protected:
    ProtocolConfig conf;
    Party id;
    size_t size, nodes, depth, bits;
    RandomGenerators rngs;
    std::vector<Ring> weights;
    std::shared_ptr<io::NetIOMP> network;
    bool ssd;

    Graph g;

    std::vector<std::vector<std::unique_ptr<Function>>> circ;
    std::vector<std::unique_ptr<Function>> f_queue;
    ShufflePre *current_shuffle;
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

    std::vector<Ring> add_sort(std::vector<std::vector<Ring>> &bit_keys, size_t bits);

    std::vector<Ring> add_sort_iteration(std::vector<Ring> &perm, std::vector<Ring> &keys);

    void build_initialization();

    void build_circuit() {
        std::vector<size_t> wire_level(num_wires, 0);
        std::vector<size_t> function_level(f_queue.size(), 0);
        size_t depth = 0;

        for (auto &f : f_queue) {
            size_t max_depth = 0;
            for (size_t i = 0; i < f->input.size(); ++i) {
                auto wire_depth = wire_level[f->input[i]];
                max_depth = std::max(max_depth, wire_depth);
            }
            for (size_t i = 0; i < f->input2.size(); ++i) {
                auto wire_depth = wire_level[f->input2[i]];
                max_depth = std::max(max_depth, wire_depth);
            }

            max_depth++;

            function_level[f->f_id] = max_depth;
            for (size_t i = 0; i < f->output.size(); ++i) {
                wire_level[f->output[i]] = max_depth;
            }
            depth = std::max(depth, max_depth);
        }

        circ.resize(depth);

        for (auto &f : f_queue) {
            circ[function_level[f->f_id]].push_back(std::move(f));
        }
    }

    void add_function(std::unique_ptr<Function> func) { f_queue.push_back(std::move(func)); }

    std::vector<Ring> add_propagate_1(std::vector<Ring> &input) {
        std::vector<Ring> output(size);
        std::iota(output.begin(), output.end(), num_wires);
        num_wires += size;

        f_queue.emplace_back(std::make_unique<Propagate_1>(f_queue.size(), &conf, input, output));
        return output;
    }

    std::vector<Ring> add_propagate_2(std::vector<Ring> &input1, std::vector<Ring> &input2) {
        std::vector<Ring> output(size);
        std::iota(output.begin(), output.end(), num_wires);
        num_wires += size;

        f_queue.emplace_back(std::make_unique<Propagate_2>(f_queue.size(), &conf, input1, input2, output));
        return output;
    }

    std::vector<Ring> add_gather_1(std::vector<Ring> &input) {
        std::vector<Ring> output(size);
        std::iota(output.begin(), output.end(), num_wires);
        num_wires += size;

        f_queue.emplace_back(std::make_unique<Gather_1>(f_queue.size(), &conf, input, output));
        return output;
    }

    std::vector<Ring> add_gather_2(std::vector<Ring> &input) {
        std::vector<Ring> output(size);
        std::iota(output.begin(), output.end(), num_wires);
        num_wires += size;

        f_queue.emplace_back(std::make_unique<Gather_2>(f_queue.size(), &conf, input, output));
        return output;
    }

    std::vector<Ring> add_shuffle(std::vector<Ring> &input) {
        std::vector<Ring> output(size);
        std::iota(output.begin(), output.end(), num_wires);
        num_wires += size;

        auto shuffle_ptr = std::make_unique<Shuffle>(f_queue.size(), &conf, &ctx.preproc, &ctx.shuffle_vals, input, output, recv_shuffle, &preproc_disk);
        current_shuffle = shuffle_ptr->perm_share;
        add_function(std::move(shuffle_ptr));
        return output;
    }

    std::vector<Ring> add_shuffle(std::vector<Ring> &input, ShufflePre *perm_share) {
        std::vector<Ring> output(size);
        std::iota(output.begin(), output.end(), num_wires);
        num_wires += size;

        add_function(std::make_unique<Shuffle>(f_queue.size(), &conf, &ctx.preproc, &ctx.shuffle_vals, input, output, recv_shuffle, perm_share, &preproc_disk));
        return output;
    }

    std::vector<Ring> repeat_shuffle(std::vector<Ring> &input) {
        std::vector<Ring> output(size);
        std::iota(output.begin(), output.end(), num_wires);
        num_wires += size;

        add_function(std::make_unique<ShuffleRepeat>(f_queue.size(), &conf, &ctx.preproc, &ctx.shuffle_vals, input, output, current_shuffle, recv_shuffle,
                                                     &preproc_disk));
        return output;
    }

    std::vector<Ring> repeat_shuffle(std::vector<Ring> &input, ShufflePre &perm_share) {
        std::vector<Ring> output(size);
        std::iota(output.begin(), output.end(), num_wires);
        num_wires += size;

        add_function(
            std::make_unique<ShuffleRepeat>(f_queue.size(), &conf, &ctx.preproc, &ctx.shuffle_vals, input, output, &perm_share, recv_shuffle, &preproc_disk));
        return output;
    }

    std::vector<Ring> add_unshuffle(std::vector<Ring> &input) {
        std::vector<Ring> output(size);
        std::iota(output.begin(), output.end(), num_wires);
        num_wires += size;

        add_function(
            std::make_unique<Unshuffle>(f_queue.size(), &conf, &ctx.preproc, &ctx.shuffle_vals, input, output, current_shuffle, recv_shuffle, &preproc_disk));
        return output;
    }

    std::vector<Ring> add_merged_shuffle(std::vector<Ring> &input, ShufflePre &perm_share, ShufflePre &pi_share, ShufflePre &omega_share) {
        std::vector<Ring> output(size);
        std::iota(output.begin(), output.end(), num_wires);
        num_wires += size;

        add_function(std::make_unique<MergedShuffle>(f_queue.size(), &conf, &ctx.preproc, &ctx.shuffle_vals, input, output, recv_shuffle, &perm_share,
                                                     &pi_share, &omega_share, &preproc_disk));
        return output;
    }

    std::vector<Ring> add_compaction(std::vector<Ring> &input) {
        std::vector<Ring> output(size);
        std::iota(output.begin(), output.end(), num_wires);
        num_wires += size;

        add_function(std::make_unique<Compaction>(f_queue.size(), &conf, &ctx.preproc, &ctx.mult_vals, input, output, recv_mul, &preproc_disk, &triples_disk));
        return output;
    }

    std::vector<Ring> add_reveal(std::vector<Ring> &input) {
        std::vector<Ring> output(size);
        std::iota(output.begin(), output.end(), num_wires);
        num_wires += size;

        add_function(std::make_unique<Reveal>(f_queue.size(), &conf, &ctx.reveal_vals, input, output));
        return output;
    }

    std::vector<Ring> add_permute(std::vector<Ring> &input, std::vector<Ring> &perm, bool inverse = false) {
        std::vector<Ring> output(size);
        std::iota(output.begin(), output.end(), num_wires);
        num_wires += size;

        f_queue.emplace_back(std::make_unique<Permute>(f_queue.size(), &conf, input, output, &perm, inverse));
        return output;
    }

    std::vector<Ring> add_update(std::vector<Ring> &input) {
        std::vector<Ring> output(size);
        std::iota(output.begin(), output.end(), num_wires);
        num_wires += size;

        f_queue.emplace_back(std::make_unique<Update>(f_queue.size(), &conf, input, output));
        return output;
    }

    std::vector<Ring> add_equals_zero(std::vector<Ring> &input, size_t size, size_t layer) {
        std::vector<Ring> output(size);
        std::iota(output.begin(), output.end(), num_wires);
        num_wires += size;

        add_function(
            std::make_unique<EQZ>(f_queue.size(), &conf, &ctx.preproc, &ctx.and_vals, input, output, recv_mul, size, layer, &preproc_disk, &triples_disk));
        return output;
    }

    std::vector<Ring> add_Bit2A(std::vector<Ring> &input, size_t size) {
        std::vector<Ring> output(size);
        std::iota(output.begin(), output.end(), num_wires);
        num_wires += size;

        add_function(std::make_unique<Bit2A>(f_queue.size(), &conf, &ctx.preproc, &ctx.mult_vals, input, output, recv_mul, size, &preproc_disk, &triples_disk));
        return output;
    }

    /* TODO: Connect output in Insert */
    std::vector<Ring> add_insert(std::vector<Ring> input) {
        std::vector<Ring> output(size);
        std::iota(output.begin(), output.end(), num_wires);
        num_wires += size;

        f_queue.emplace_back(std::make_unique<Insert>(f_queue.size(), &conf, &input));
        return output;
    }

    std::vector<Ring> add_deduplication_sub(std::vector<Ring> &vec_p) {
        std::vector<Ring> vec_dupl(size - 1);
        std::iota(vec_dupl.begin(), vec_dupl.end(), num_wires);
        num_wires += (size - 1);

        f_queue.emplace_back(std::make_unique<DeduplicationSub>(f_queue.size(), &conf, vec_p, vec_dupl));
        return vec_dupl;
    }

    std::vector<Ring> add_mul(std::vector<Ring> &x, std::vector<Ring> &y, bool binary = false) {
        std::vector<Ring> output(size);
        std::iota(output.begin(), output.end(), num_wires);
        num_wires += size;

        if (binary) {
            add_function(
                std::make_unique<Mul>(f_queue.size(), &conf, &ctx.preproc, &ctx.and_vals, x, y, output, recv_mul, binary, &preproc_disk, &triples_disk));
        } else {
            add_function(
                std::make_unique<Mul>(f_queue.size(), &conf, &ctx.preproc, &ctx.mult_vals, x, y, output, recv_mul, binary, &preproc_disk, &triples_disk));
        }
        return output;
    }

    std::vector<Ring> add_mul(std::vector<Ring> &x, std::vector<Ring> &y, size_t size, bool binary = false) {
        std::vector<Ring> output(size);
        std::iota(output.begin(), output.end(), num_wires);
        num_wires += size;

        if (binary) {
            add_function(
                std::make_unique<Mul>(f_queue.size(), &conf, &ctx.preproc, &ctx.and_vals, x, y, output, recv_mul, binary, size, &preproc_disk, &triples_disk));
        } else {
            add_function(
                std::make_unique<Mul>(f_queue.size(), &conf, &ctx.preproc, &ctx.mult_vals, x, y, output, recv_mul, binary, size, &preproc_disk, &triples_disk));
        }
        return output;
    }

    /* TODO: Connect output in PushBack */
    std::vector<Ring> add_push_back(std::vector<std::vector<Ring>> &keys, std::vector<Ring> &vec) {
        std::vector<Ring> output(size);
        std::iota(output.begin(), output.end(), num_wires);
        num_wires += size;

        f_queue.emplace_back(std::make_unique<PushBack>(f_queue.size(), &conf, &keys, vec));
        return output;
    }

    std::vector<Ring> add_flip(std::vector<Ring> &input) {
        std::vector<Ring> output(size);
        std::iota(output.begin(), output.end(), num_wires);
        num_wires += size;

        f_queue.emplace_back(std::make_unique<Flip>(f_queue.size(), &conf, input, output));
        return output;
    }

    std::vector<Ring> add_add(std::vector<Ring> &input1, std::vector<Ring> &input2) {
        std::vector<Ring> output(size);
        std::iota(output.begin(), output.end(), num_wires);
        num_wires += size;

        f_queue.emplace_back(std::make_unique<Add>(f_queue.size(), &conf, input1, input2, output));
        return output;
    }

    std::vector<Ring> add_construct_payload(std::vector<std::vector<Ring>> &payloads) {
        std::vector<Ring> output(size);
        std::iota(output.begin(), output.end(), num_wires);
        num_wires += size;

        f_queue.emplace_back(std::make_unique<ConstructPayload>(f_queue.size(), &conf, &payloads, output));
        return output;
    }

    void add_deduplication() {
        ctx.dst_order = add_sort(g.dst_order_bits, bits + 1);
        w.deduplication_perm = ctx.dst_order;

        for (size_t i = 0; i < bits; ++i) {  // Was g.src_bits().size()
            w.deduplication_perm = add_sort_iteration(w.deduplication_perm, g.src_bits[i]);
        }

        w.deduplication_perm = add_shuffle(w.deduplication_perm);
        w.deduplication_perm = add_reveal(w.deduplication_perm);
        w.deduplication_src = repeat_shuffle(g.src);
        w.deduplication_dst = repeat_shuffle(g.dst);
        w.deduplication_perm = add_permute(w.deduplication_src, w.deduplication_src);
        w.deduplication_perm = add_permute(w.deduplication_dst, w.deduplication_dst);

        w.deduplication_src_dupl = add_deduplication_sub(w.deduplication_src);
        w.deduplication_dst_dupl = add_deduplication_sub(w.deduplication_dst);

        w.deduplication_src_dupl = add_equals_zero(w.deduplication_src_dupl, size - 1, 0);
        w.deduplication_dst_dupl = add_equals_zero(w.deduplication_dst_dupl, size - 1, 0);
        w.deduplication_src_dupl = add_equals_zero(w.deduplication_src_dupl, size - 1, 1);
        w.deduplication_dst_dupl = add_equals_zero(w.deduplication_dst_dupl, size - 1, 1);
        w.deduplication_src_dupl = add_equals_zero(w.deduplication_src_dupl, size - 1, 2);
        w.deduplication_dst_dupl = add_equals_zero(w.deduplication_dst_dupl, size - 1, 2);
        w.deduplication_src_dupl = add_equals_zero(w.deduplication_src_dupl, size - 1, 3);
        w.deduplication_dst_dupl = add_equals_zero(w.deduplication_dst_dupl, size - 1, 3);
        w.deduplication_src_dupl = add_equals_zero(w.deduplication_src_dupl, size - 1, 4);
        w.deduplication_dst_dupl = add_equals_zero(w.deduplication_dst_dupl, size - 1, 4);

        w.deduplication_duplicates = add_mul(w.deduplication_src_dupl, w.deduplication_dst_dupl, size - 1, true);
        w.deduplication_duplicates = add_Bit2A(w.deduplication_duplicates, size - 1);
        w.deduplication_duplicates = add_insert(w.deduplication_duplicates);

        w.deduplication_duplicates = add_permute(w.deduplication_duplicates, w.deduplication_perm, true);
        w.deduplication_duplicates = add_unshuffle(w.deduplication_duplicates);
        add_push_back(g.src_order_bits, w.deduplication_duplicates);
        add_push_back(g.dst_order_bits, w.deduplication_duplicates);
    }

    void add_clip() {
        for (size_t i = 0; i < w.mp_data_parallel.size(); ++i) {
            std::vector<Ring> &wire = w.mp_data;
            if (w.mp_data_parallel.size() == 0) {
                wire = add_equals_zero(wire, size, 0);
                wire = add_equals_zero(wire, size, 1);
                wire = add_equals_zero(wire, size, 2);
                wire = add_equals_zero(wire, size, 3);
                wire = add_equals_zero(wire, size, 4);
                wire = add_Bit2A(wire, size);
                wire = add_flip(wire);
            } else {
                w.mp_data_parallel[i] = add_equals_zero(w.mp_data_parallel[i], size, 0);
                w.mp_data_parallel[i] = add_equals_zero(w.mp_data_parallel[i], size, 1);
                w.mp_data_parallel[i] = add_equals_zero(w.mp_data_parallel[i], size, 2);
                w.mp_data_parallel[i] = add_equals_zero(w.mp_data_parallel[i], size, 3);
                w.mp_data_parallel[i] = add_equals_zero(w.mp_data_parallel[i], size, 4);
                w.mp_data_parallel[i] = add_Bit2A(w.mp_data_parallel[i], size);
                w.mp_data_parallel[i] = add_flip(w.mp_data_parallel[i]);
            }
        }
    }
};