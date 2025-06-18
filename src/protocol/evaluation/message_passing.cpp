#include "../message_passing.h"

void mp::evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, SecretSharedGraph &g,
                  size_t n_iterations, size_t n_vertices, MPPreprocessing &preproc, F_apply f_apply) {
    auto [src_order_bits, dst_order_bits, inverted_isV] = init(id, g);
    /* Compute the three permutations */
    Permutation src_order = sort::get_sort_evaluate(id, rngs, network, n, BLOCK_SIZE, src_order_bits, preproc.src_order_pre);
    Permutation dst_order = sort::get_sort_evaluate(id, rngs, network, n, BLOCK_SIZE, dst_order_bits, preproc.dst_order_pre);
    Permutation vtx_order = sort::sort_iteration_evaluate(id, rngs, network, n, BLOCK_SIZE, src_order, inverted_isV, preproc.vtx_order_pre);

    /* Bring payload into vertex order */
    auto payload_v = SecretSharedGraph::from_bits(g.payload_bits, g.size);
    payload_v = sort::apply_perm_evaluate(id, rngs, network, n, BLOCK_SIZE, vtx_order, preproc.apply_perm_pre, payload_v);

    for (size_t i = 0; i < n_iterations; ++i) {
        /* Propagate-1 */
        auto payload_p = propagate_1(payload_v, n_vertices);

        /* Switch Perm to src order */
        auto [pi, omega, merged] = preproc.sw_perm_pre[0];
        auto payload_src = sort::switch_perm_evaluate(id, rngs, network, n, BLOCK_SIZE, vtx_order, src_order, pi, omega, merged, payload_p);
        auto [pi_1, omega_1, merged_1] = preproc.sw_perm_pre[1];
        auto payload_corr = sort::switch_perm_evaluate(id, rngs, network, n, BLOCK_SIZE, vtx_order, src_order, pi_1, omega_1, merged_1, payload_v);

        /* Propagate-2 */
        payload_p = propagate_2(payload_src, payload_corr);

        /* Switch Perm to dst order*/
        auto [pi_2, omega_2, merged_2] = preproc.sw_perm_pre[2];
        auto payload_dst = sort::switch_perm_evaluate(id, rngs, network, n, BLOCK_SIZE, src_order, dst_order, pi_2, omega_2, merged_2, payload_p);

        /* Gather-1*/
        payload_p = gather_1(payload_dst);

        /* Switch Perm to vtx order */
        auto [pi_3, omega_3, merged_3] = preproc.sw_perm_pre[3];
        payload_p = sort::switch_perm_evaluate(id, rngs, network, n, BLOCK_SIZE, dst_order, vtx_order, pi_3, omega_3, merged_3, payload_p);

        preproc.sw_perm_pre.erase(preproc.sw_perm_pre.begin(), preproc.sw_perm_pre.begin() + 4);

        /* Gather-2 */
        auto update = gather_2(payload_p, n_vertices);

        /* Apply */
        payload_v = f_apply(payload_v, update);
    }

    std::vector<Ring> payload_v_eqz = clip::equals_zero_evaluate(id, rngs, network, BLOCK_SIZE, preproc.eqz_triples, payload_v);
    std::vector<Ring> payload_v_B2A = clip::B2A_evaluate(id, rngs, network, BLOCK_SIZE, preproc.B2A_triples, payload_v_eqz);

    auto payload_v_flip = clip::flip(id, payload_v_B2A);

    auto payload_bits_new = SecretSharedGraph::to_bits(payload_v_flip, sizeof(Ring) * 8);
    g.payload_bits = payload_bits_new;
}
