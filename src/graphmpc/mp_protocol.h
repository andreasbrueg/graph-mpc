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
#include "add_const.h"
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
#include "unshuffle.h"

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

    std::shared_ptr<ShufflePre> vtx_order_shuffle;
    std::shared_ptr<ShufflePre> src_order_shuffle;
    std::shared_ptr<ShufflePre> dst_order_shuffle;
    std::shared_ptr<ShufflePre> vtx_src_merge;
    std::shared_ptr<ShufflePre> src_dst_merge;
    std::shared_ptr<ShufflePre> dst_vtx_merge;
};

struct Wires {
    std::vector<std::vector<Ring>> src_order_bits;
    std::vector<std::vector<Ring>> dst_order_bits;
    std::vector<Ring> isV_inv;
    std::vector<Ring> data;
    std::vector<std::vector<Ring>> mp_data_parallel;
    std::vector<Ring> buf;
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
        w.src_order_bits.resize(bits + 1);
        w.dst_order_bits.resize(bits + 1);
        w.isV_inv.resize(size);
        w.data.resize(size);

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
            w.isV_inv[i] = num_wires;
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

    virtual void compute_sorts();  // Can be overwritten

    void prepare_shuffles();

    json details() {
        size_t rounds = circ.size();
        json output;
        output["details"] = {
            {"party", id}, {"size", size}, {"nodes", nodes}, {"depth", depth}, {"bits", bits}, {"ssd", ssd}, {"Communication rounds: ", rounds}};
        return output;
    }

    void build() {
        pre_mp();
        compute_sorts();
        prepare_shuffles();
        if (w.mp_data_parallel.size() > 0) {  // Only the case for pi_r
            for (size_t i = 0; i < w.mp_data_parallel.size(); ++i) {
                w.buf = w.mp_data_parallel[i];
                w.data = w.mp_data_parallel[i];
                build_message_passing();
                w.mp_data_parallel[i] = w.data;
            }
        } else {
            build_message_passing();
        }
        post_mp();
        g.data = w.data;
        build_circuit();
        init_inputs();
    }

    void build_message_passing();

    void preprocess();

    void evaluate();

    void set_input(Graph &graph) { g = graph; }

    Graph get_output() { return g; }

    void init_inputs() {
        w.src_order_bits = g.src_order_bits;
        w.dst_order_bits = g.dst_order_bits;
        w.isV_inv = g.isV_inv;
        w.data = g.data;
    }

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
        std::cout << "Communication rounds: " << circ.size() << std::endl;
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

        ctx.vtx_order_shuffle = std::make_shared<ShufflePre>();
        ctx.src_order_shuffle = std::make_shared<ShufflePre>();
        ctx.dst_order_shuffle = std::make_shared<ShufflePre>();
        ctx.vtx_src_merge = std::make_shared<ShufflePre>();
        ctx.src_dst_merge = std::make_shared<ShufflePre>();
        ctx.dst_vtx_merge = std::make_shared<ShufflePre>();
        ctx.vtx_order_shuffle->initialize(id, size);
        ctx.src_order_shuffle->initialize(id, size);
        ctx.dst_order_shuffle->initialize(id, size);
        ctx.vtx_src_merge->initialize(id, size);
        ctx.src_dst_merge->initialize(id, size);
        ctx.dst_vtx_merge->initialize(id, size);
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
    std::vector<Ring> data;

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

    std::vector<Ring> add_sort(std::vector<std::vector<Ring>> &bit_keys, size_t bits);

    std::vector<Ring> add_sort_iteration(std::vector<Ring> &perm, std::vector<Ring> &keys);

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

            if (f->interactive) max_depth++;

            function_level[f->f_id] = max_depth;
            for (size_t i = 0; i < f->output.size(); ++i) {
                wire_level[f->output[i]] = max_depth;
            }

            depth = std::max(depth, max_depth);
        }

        circ.resize(depth + 1);

        for (size_t i = 0; i < f_queue.size(); ++i) {
            auto &f = f_queue[i];
            if (f == nullptr) {
                std::cout << "nullptr function at index " << i << std::endl;
                continue;
            }
            circ[function_level[f->f_id]].push_back(f);
        }
    }

    void add_function(std::shared_ptr<Function> func) { f_queue.push_back(std::move(func)); }

    std::vector<Ring> propagate_1(std::vector<Ring> &input) {
        std::vector<Ring> output(size);
        std::iota(output.begin(), output.end(), num_wires);
        num_wires += size;

        f_queue.emplace_back(std::make_shared<Propagate_1>(f_queue.size(), &conf, input, output));
        return output;
    }

    std::vector<Ring> propagate_2(std::vector<Ring> &input1, std::vector<Ring> &input2) {
        std::vector<Ring> output(size);
        std::iota(output.begin(), output.end(), num_wires);
        num_wires += size;

        f_queue.emplace_back(std::make_shared<Propagate_2>(f_queue.size(), &conf, input1, input2, output));
        return output;
    }

    std::vector<Ring> gather_1(std::vector<Ring> &input) {
        std::vector<Ring> output(size);
        std::iota(output.begin(), output.end(), num_wires);
        num_wires += size;

        f_queue.emplace_back(std::make_shared<Gather_1>(f_queue.size(), &conf, input, output));
        return output;
    }

    std::vector<Ring> gather_2(std::vector<Ring> &input) {
        std::vector<Ring> output(size);
        std::iota(output.begin(), output.end(), num_wires);
        num_wires += size;

        f_queue.emplace_back(std::make_shared<Gather_2>(f_queue.size(), &conf, input, output));
        return output;
    }

    std::vector<Ring> shuffle(std::vector<Ring> &input, size_t shuffle_idx) {
        std::vector<Ring> output(size);
        std::iota(output.begin(), output.end(), num_wires);
        num_wires += size;

        add_function(std::make_shared<Shuffle>(f_queue.size(), &conf, &ctx.preproc, &ctx.shuffle_vals, &shuffles, input, output, recv_shuffle, &preproc_disk,
                                               shuffle_idx));
        return output;
    }

    // std::vector<Ring> repeat_shuffle(std::vector<Ring> &input, ShufflePre &perm_share) {
    // std::vector<Ring> output(size);
    // std::iota(output.begin(), output.end(), num_wires);
    // num_wires += size;

    // auto f =
    // std::make_unique<ShuffleRepeat>(f_queue.size(), &conf, &ctx.preproc, &ctx.shuffle_vals, input, output, &perm_share, recv_shuffle, &preproc_disk);
    // f_queue.push_back(std::move(f));

    // return output;
    //}

    std::vector<Ring> unshuffle(std::vector<Ring> &input, size_t shuffle_idx) {
        std::vector<Ring> output(size);
        std::iota(output.begin(), output.end(), num_wires);
        num_wires += size;

        auto perm_share = shuffles[shuffles.size() - 1];  // last shuffle
        add_function(std::make_shared<Unshuffle>(f_queue.size(), &conf, &ctx.preproc, &ctx.shuffle_vals, &shuffles, input, output, recv_shuffle, &preproc_disk,
                                                 shuffle_idx));
        return output;
    }

    std::vector<Ring> merged_shuffle(std::vector<Ring> &input, std::shared_ptr<ShufflePre> pi_share, std::shared_ptr<ShufflePre> omega_share,
                                     size_t shuffle_idx) {
        std::vector<Ring> output(size);
        std::iota(output.begin(), output.end(), num_wires);
        num_wires += size;

        add_function(std::make_shared<MergedShuffle>(f_queue.size(), &conf, &ctx.preproc, &ctx.shuffle_vals, &shuffles, input, output, recv_shuffle, pi_share,
                                                     omega_share, &preproc_disk, shuffle_idx));
        return output;
    }

    std::vector<Ring> compaction(std::vector<Ring> &input) {
        std::vector<Ring> output(size);
        std::iota(output.begin(), output.end(), num_wires);
        num_wires += size;

        add_function(std::make_shared<Compaction>(f_queue.size(), &conf, &ctx.preproc, &ctx.mult_vals, input, output, recv_mul, &preproc_disk, &triples_disk));
        return output;
    }

    std::vector<Ring> reveal(std::vector<Ring> &input) {
        std::vector<Ring> output(size);
        std::iota(output.begin(), output.end(), num_wires);
        num_wires += size;

        add_function(std::make_shared<Reveal>(f_queue.size(), &conf, &ctx.reveal_vals, input, output));
        return output;
    }

    std::vector<Ring> permute(std::vector<Ring> &input, std::vector<Ring> &perm, bool inverse = false) {
        std::vector<Ring> output(size);
        std::iota(output.begin(), output.end(), num_wires);
        num_wires += size;

        f_queue.emplace_back(std::make_shared<Permute>(f_queue.size(), &conf, input, perm, output, inverse));
        return output;
    }

    std::vector<Ring> equals_zero(std::vector<Ring> &input, size_t size, size_t layer) {
        std::vector<Ring> output(size);
        std::iota(output.begin(), output.end(), num_wires);
        num_wires += size;

        add_function(
            std::make_shared<EQZ>(f_queue.size(), &conf, &ctx.preproc, &ctx.and_vals, input, output, recv_mul, size, layer, &preproc_disk, &triples_disk));
        return output;
    }

    std::vector<Ring> bit2A(std::vector<Ring> &input, size_t size) {
        std::vector<Ring> output(size);
        std::iota(output.begin(), output.end(), num_wires);
        num_wires += size;

        add_function(std::make_shared<Bit2A>(f_queue.size(), &conf, &ctx.preproc, &ctx.mult_vals, input, output, recv_mul, size, &preproc_disk, &triples_disk));
        return output;
    }

    /* TODO: Connect output in Insert */
    std::vector<Ring> insert(std::vector<Ring> input) {
        std::vector<Ring> output(size);
        std::iota(output.begin(), output.end(), num_wires);
        num_wires += size;

        f_queue.emplace_back(std::make_shared<Insert>(f_queue.size(), &conf, input, output));
        return output;
    }

    std::vector<Ring> deduplication_sub(std::vector<Ring> &vec_p) {
        std::vector<Ring> vec_dupl(size - 1);
        std::iota(vec_dupl.begin(), vec_dupl.end(), num_wires);
        num_wires += (size - 1);

        f_queue.emplace_back(std::make_shared<DeduplicationSub>(f_queue.size(), &conf, vec_p, vec_dupl));
        return vec_dupl;
    }

    std::vector<Ring> mul(std::vector<Ring> &x, std::vector<Ring> &y, bool binary = false) {
        std::vector<Ring> output(size);
        std::iota(output.begin(), output.end(), num_wires);
        num_wires += size;

        if (binary) {
            add_function(
                std::make_shared<Mul>(f_queue.size(), &conf, &ctx.preproc, &ctx.and_vals, x, y, output, recv_mul, binary, &preproc_disk, &triples_disk));
        } else {
            add_function(
                std::make_shared<Mul>(f_queue.size(), &conf, &ctx.preproc, &ctx.mult_vals, x, y, output, recv_mul, binary, &preproc_disk, &triples_disk));
        }
        return output;
    }

    std::vector<Ring> mul(std::vector<Ring> &x, std::vector<Ring> &y, size_t size, bool binary = false) {
        std::vector<Ring> output(size);
        std::iota(output.begin(), output.end(), num_wires);
        num_wires += size;

        if (binary) {
            add_function(
                std::make_shared<Mul>(f_queue.size(), &conf, &ctx.preproc, &ctx.and_vals, x, y, output, recv_mul, binary, size, &preproc_disk, &triples_disk));
        } else {
            add_function(
                std::make_shared<Mul>(f_queue.size(), &conf, &ctx.preproc, &ctx.mult_vals, x, y, output, recv_mul, binary, size, &preproc_disk, &triples_disk));
        }
        return output;
    }

    /* TODO: Connect output in PushBack */
    std::vector<Ring> push_back(std::vector<std::vector<Ring>> &keys, std::vector<Ring> &vec) {
        std::vector<Ring> output(size);
        std::iota(output.begin(), output.end(), num_wires);
        num_wires += size;

        f_queue.emplace_back(std::make_shared<PushBack>(f_queue.size(), &conf, &keys, vec));
        return output;
    }

    std::vector<Ring> flip(std::vector<Ring> &input) {
        std::vector<Ring> output(size);
        std::iota(output.begin(), output.end(), num_wires);
        num_wires += size;

        f_queue.emplace_back(std::make_shared<Flip>(f_queue.size(), &conf, input, output));
        return output;
    }

    std::vector<Ring> add(std::vector<Ring> &input1, std::vector<Ring> &input2) {
        std::vector<Ring> output(size);
        std::iota(output.begin(), output.end(), num_wires);
        num_wires += size;

        f_queue.emplace_back(std::make_shared<Add>(f_queue.size(), &conf, input1, input2, output));
        return output;
    }

    std::vector<Ring> add_const(std::vector<Ring> &data, Ring val) {
        std::vector<Ring> output(size);
        std::iota(output.begin(), output.end(), num_wires);
        num_wires += size;

        f_queue.emplace_back(std::make_shared<AddConst>(f_queue.size(), &conf, data, val, output));
        return output;
    }

    std::vector<Ring> construct_payload(std::vector<std::vector<Ring>> &payloads) {
        std::vector<Ring> output(size);
        std::iota(output.begin(), output.end(), num_wires);
        num_wires += size;

        f_queue.emplace_back(std::make_shared<ConstructPayload>(f_queue.size(), &conf, &payloads, output));
        return output;
    }

    void add_deduplication() {
        ctx.dst_order = add_sort(g.dst_order_bits, bits + 1);
        auto deduplication_perm = ctx.dst_order;

        for (size_t i = 0; i < bits; ++i) {  // Was g.src_bits().size()
            deduplication_perm = add_sort_iteration(deduplication_perm, g.src_bits[i]);
        }

        deduplication_perm = shuffle(deduplication_perm, shuffle_idx);
        deduplication_perm = reveal(deduplication_perm);

        auto deduplication_src = shuffle(g.src, shuffle_idx);
        auto deduplication_dst = shuffle(g.dst, shuffle_idx);

        deduplication_perm = permute(deduplication_src, deduplication_src);
        deduplication_perm = permute(deduplication_dst, deduplication_dst);

        auto deduplication_src_dupl = deduplication_sub(deduplication_src);
        auto deduplication_dst_dupl = deduplication_sub(deduplication_dst);

        deduplication_src_dupl = equals_zero(deduplication_src_dupl, size - 1, 0);
        deduplication_dst_dupl = equals_zero(deduplication_dst_dupl, size - 1, 0);
        deduplication_src_dupl = equals_zero(deduplication_src_dupl, size - 1, 1);
        deduplication_dst_dupl = equals_zero(deduplication_dst_dupl, size - 1, 1);
        deduplication_src_dupl = equals_zero(deduplication_src_dupl, size - 1, 2);
        deduplication_dst_dupl = equals_zero(deduplication_dst_dupl, size - 1, 2);
        deduplication_src_dupl = equals_zero(deduplication_src_dupl, size - 1, 3);
        deduplication_dst_dupl = equals_zero(deduplication_dst_dupl, size - 1, 3);
        deduplication_src_dupl = equals_zero(deduplication_src_dupl, size - 1, 4);
        deduplication_dst_dupl = equals_zero(deduplication_dst_dupl, size - 1, 4);

        auto deduplication_duplicates = mul(deduplication_src_dupl, deduplication_dst_dupl, size - 1, true);
        deduplication_duplicates = bit2A(deduplication_duplicates, size - 1);
        deduplication_duplicates = insert(deduplication_duplicates);

        deduplication_duplicates = permute(deduplication_duplicates, deduplication_perm, true);
        deduplication_duplicates = unshuffle(deduplication_duplicates, shuffle_idx);
        push_back(g.src_order_bits, deduplication_duplicates);
        push_back(g.dst_order_bits, deduplication_duplicates);
        shuffle_idx++;
    }

    void add_clip() {
        for (size_t i = 0; i < w.mp_data_parallel.size(); ++i) {
            std::vector<Ring> &wire = w.data;
            if (w.mp_data_parallel.size() == 0) {
                wire = equals_zero(wire, size, 0);
                wire = equals_zero(wire, size, 1);
                wire = equals_zero(wire, size, 2);
                wire = equals_zero(wire, size, 3);
                wire = equals_zero(wire, size, 4);
                wire = bit2A(wire, size);
                wire = flip(wire);
            } else {
                w.mp_data_parallel[i] = equals_zero(w.mp_data_parallel[i], size, 0);
                w.mp_data_parallel[i] = equals_zero(w.mp_data_parallel[i], size, 1);
                w.mp_data_parallel[i] = equals_zero(w.mp_data_parallel[i], size, 2);
                w.mp_data_parallel[i] = equals_zero(w.mp_data_parallel[i], size, 3);
                w.mp_data_parallel[i] = equals_zero(w.mp_data_parallel[i], size, 4);
                w.mp_data_parallel[i] = bit2A(w.mp_data_parallel[i], size);
                w.mp_data_parallel[i] = flip(w.mp_data_parallel[i]);
            }
        }
    }
};