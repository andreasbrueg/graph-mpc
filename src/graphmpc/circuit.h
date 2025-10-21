#pragma once

#include <memory>

#include "../utils/structs.h"
#include "function.h"

class Circuit {
   public:
    Circuit(ProtocolConfig &conf)
        : n_shuffles(0), n_unshuffles(0), n_mults(0), n_wires(0), size(conf.size), bits(conf.bits), depth(conf.depth), shuffle_idx(0), weights(conf.weights) {}

    std::vector<std::vector<std::shared_ptr<Function>>> get() { return circ; }

    void build();

    void level_order();

    virtual void pre_mp() = 0;
    virtual size_t apply(size_t &data_vtx) = 0;
    virtual size_t post_mp(size_t &data) = 0;
    virtual void compute_sorts();  // Can be overwritten

    size_t n_shuffles;
    size_t n_unshuffles;
    size_t n_mults;

    size_t n_wires;

   protected:
    std::vector<std::vector<std::shared_ptr<Function>>> circ;
    std::vector<std::shared_ptr<Function>> f_queue;

    MPContext ctx;
    Inputs in;

    size_t size;
    size_t bits;
    size_t depth;
    size_t shuffle_idx;
    std::vector<Ring> weights;

    void set_inputs();

    void prepare_shuffles();

    size_t message_passing(size_t &data);

    size_t sort(std::vector<size_t> &bit_keys, size_t bits);

    size_t sort_iteration(size_t &perm, size_t &keys);

    /* ----- Single Functions ----- */

    size_t input();

    void output(size_t &input);

    size_t propagate_1(size_t &input);

    size_t propagate_2(size_t &input1, size_t &input2);

    size_t gather_1(size_t &input);

    size_t gather_2(size_t &input);

    size_t shuffle(size_t &input, size_t shuffle_idx);

    size_t unshuffle(size_t &input, size_t shuffle_idx);

    size_t merged_shuffle(size_t &input, size_t shuffle_idx, size_t pi_idx, size_t omega_idx);

    size_t compaction(size_t &input);

    size_t reveal(size_t &input);

    size_t permute(size_t &input, size_t &perm);

    size_t reverse_permute(size_t &input, size_t &perm);

    size_t equals_zero(size_t &input, size_t size, size_t layer);

    size_t bit2A(size_t &input, size_t size);

    size_t deduplication_sub(size_t &input1);

    size_t deduplication_insert(size_t &input1);

    size_t mul(size_t &x, size_t &y, bool binary = false);

    size_t mul(size_t &x, size_t &y, size_t size, bool binary = false);

    size_t flip(size_t &input);

    size_t add(size_t &input1, size_t &input2);

    size_t add_const(size_t &data, Ring val);

    // void deduplication() {
    // ctx.dst_order = add_sort(g.dst_order_bits, bits + 1);
    // auto deduplication_perm = ctx.dst_order;

    // for (size_t i = 0; i < bits; ++i) {  // Was g.src_bits().size()
    // deduplication_perm = add_sort_iteration(deduplication_perm, g.src_bits[i]);
    //}

    // deduplication_perm = shuffle(deduplication_perm, shuffle_idx);
    // deduplication_perm = reveal(deduplication_perm);

    // auto deduplication_src = shuffle(g.src, shuffle_idx);
    // auto deduplication_dst = shuffle(g.dst, shuffle_idx);

    // deduplication_perm = permute(deduplication_src, deduplication_src);
    // deduplication_perm = permute(deduplication_dst, deduplication_dst);

    // auto deduplication_src_dupl = deduplication_sub(deduplication_src);
    // auto deduplication_dst_dupl = deduplication_sub(deduplication_dst);

    // deduplication_src_dupl = equals_zero(deduplication_src_dupl, size - 1, 0);
    // deduplication_dst_dupl = equals_zero(deduplication_dst_dupl, size - 1, 0);
    // deduplication_src_dupl = equals_zero(deduplication_src_dupl, size - 1, 1);
    // deduplication_dst_dupl = equals_zero(deduplication_dst_dupl, size - 1, 1);
    // deduplication_src_dupl = equals_zero(deduplication_src_dupl, size - 1, 2);
    // deduplication_dst_dupl = equals_zero(deduplication_dst_dupl, size - 1, 2);
    // deduplication_src_dupl = equals_zero(deduplication_src_dupl, size - 1, 3);
    // deduplication_dst_dupl = equals_zero(deduplication_dst_dupl, size - 1, 3);
    // deduplication_src_dupl = equals_zero(deduplication_src_dupl, size - 1, 4);
    // deduplication_dst_dupl = equals_zero(deduplication_dst_dupl, size - 1, 4);

    // auto deduplication_duplicates = mul(deduplication_src_dupl, deduplication_dst_dupl, size - 1, true);
    // deduplication_duplicates = bit2A(deduplication_duplicates, size - 1);
    // deduplication_duplicates = insert(deduplication_duplicates);

    // deduplication_duplicates = permute(deduplication_duplicates, deduplication_perm, true);
    // deduplication_duplicates = unshuffle(deduplication_duplicates, shuffle_idx);
    //// push_back(g.src_order_bits, deduplication_duplicates);
    //// push_back(g.dst_order_bits, deduplication_duplicates);
    // shuffle_idx++;
    //}

    // void add_clip() {
    //  for (size_t i = 0; i < w.mp_data_parallel.size(); ++i) {
    //  std::vector<Ring> &wire = w.data;
    //  if (w.mp_data_parallel.size() == 0) {
    //  wire = equals_zero(wire, size, 0);
    //  wire = equals_zero(wire, size, 1);
    //  wire = equals_zero(wire, size, 2);
    //  wire = equals_zero(wire, size, 3);
    //  wire = equals_zero(wire, size, 4);
    //  wire = bit2A(wire, size);
    //  wire = flip(wire);
    // } else {
    //  w.mp_data_parallel[i] = equals_zero(w.mp_data_parallel[i], size, 0);
    //  w.mp_data_parallel[i] = equals_zero(w.mp_data_parallel[i], size, 1);
    //  w.mp_data_parallel[i] = equals_zero(w.mp_data_parallel[i], size, 2);
    //  w.mp_data_parallel[i] = equals_zero(w.mp_data_parallel[i], size, 3);
    //  w.mp_data_parallel[i] = equals_zero(w.mp_data_parallel[i], size, 4);
    //  w.mp_data_parallel[i] = bit2A(w.mp_data_parallel[i], size);
    //  w.mp_data_parallel[i] = flip(w.mp_data_parallel[i]);
    // }
    // }
    //}
};