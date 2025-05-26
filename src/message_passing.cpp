#include "message_passing.h"

std::vector<Row> mp::propagate(ProtocolConfig &conf, std::vector<Row> &input_vector) { return input_vector; }

std::vector<Row> mp::gather(ProtocolConfig &conf, std::vector<Row> &input_vector) { return input_vector; }

std::vector<Row> mp::apply(ProtocolConfig &conf, std::vector<Row> &in1, std::vector<Row> &in2) { return in1; }

void mp::run(ProtocolConfig &conf, SecretSharedGraph &graph, size_t n_iterations) {
    auto pid = conf.pid;

    /* Generate vector containing { 1-isV, src[0], src[1], ..., src[n_bits - 1] } */
    std::vector<std::vector<Row>> src_order_bits(graph.src_bits.size() + 1);

    std::vector<Row> inverted_isV(graph.isV_bits.size());
    for (size_t i = 0; i < inverted_isV.size(); ++i) {
        inverted_isV[i] = -graph.isV_bits[i];
        if (pid == P0) inverted_isV[i] += 1;
    }

    std::copy(graph.src_bits.begin(), graph.src_bits.end(), src_order_bits.begin() + 1);
    src_order_bits[0] = inverted_isV;

    /* Generate vector containing { isV, dst[0], dst[1], ..., dst[n_bits - 1] } */
    std::vector<std::vector<Row>> dst_order_bits(graph.dst_bits.size() + 1);

    std::copy(graph.dst_bits.begin(), graph.dst_bits.end(), dst_order_bits.begin() + 1);
    dst_order_bits[0] = graph.isV_bits;

    /* Compute the three permutations */
    Permutation src_order = sort::get_sort(conf, src_order_bits);
    Permutation dst_order = sort::get_sort(conf, dst_order_bits);
    Permutation vtx_order = sort::sort_iteration(conf, src_order, inverted_isV);

    /* Bring payload into vertex order */
    auto payload_v = graph.from_bits(graph.payload_bits);
    payload_v = sort::apply_perm(conf, vtx_order, payload_v);

    for (size_t i = 0; i < n_iterations; ++i) {
        auto payload_p = propagate(conf, payload_v);
        auto payload_src = sort::switch_perm(conf, vtx_order, src_order, payload_p);
        payload_p = propagate(conf, payload_src);
        auto payload_dst = sort::switch_perm(conf, src_order, dst_order, payload_p);
        payload_p = gather(conf, payload_dst);
        payload_p = sort::switch_perm(conf, src_order, dst_order, payload_p);
        auto update = gather(conf, payload_p);
        payload_v = apply(conf, payload_v, update);
    }
}
