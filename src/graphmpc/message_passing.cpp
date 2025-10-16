#include "mp_protocol.h"

/* Case deduplication */
void MPProtocol::compute_sorts() {
    ctx.src_order = add_sort(w.src_order_bits, bits + 1);
    ctx.dst_order = add_sort(w.dst_order_bits, bits + 1);
    ctx.vtx_order = add_sort_iteration(ctx.src_order, w.isV_inv);  // TODO: Invert isV!
}

void MPProtocol::prepare_shuffles() {
    /* Prepare vtx order */
    ctx.vtx_order = shuffle(ctx.vtx_order, shuffle_idx);
    ctx.src_order = shuffle(ctx.src_order, shuffle_idx++);
    ctx.dst_order = shuffle(ctx.dst_order, shuffle_idx++);

    ctx.clear_shuffled_vtx_order = reveal(ctx.vtx_order);
    ctx.clear_shuffled_src_order = reveal(ctx.src_order);
    ctx.clear_shuffled_dst_order = reveal(ctx.dst_order);
}

void MPProtocol::build_message_passing() {
    data = shuffle(w.data, 0);
    data = permute(data, ctx.clear_shuffled_vtx_order);

    size_t vtx_src_idx = shuffle_idx;
    size_t src_dst_idx = shuffle_idx++;
    size_t dst_vtx_idx = shuffle_idx++;

    for (size_t i = 0; i < depth; ++i) {
        add_const(data, weights[weights.size() - 1 - i]);

        auto data_corr = data;

        /* Propagate-1 */
        propagate_1(data);

        /* Switch Perm from vtx to src order */
        data = permute(data, ctx.clear_shuffled_vtx_order, true);
        data = merged_shuffle(data, ctx.vtx_order_shuffle, ctx.src_order_shuffle, vtx_src_idx);
        data = permute(data, ctx.clear_shuffled_src_order);

        data_corr = permute(data_corr, ctx.clear_shuffled_vtx_order, true);
        data_corr = merged_shuffle(data_corr, ctx.vtx_order_shuffle, ctx.src_order_shuffle, vtx_src_idx);
        data_corr = permute(data_corr, ctx.clear_shuffled_src_order);

        /* Propagate-2 */
        data = propagate_2(data, data_corr);

        /* Switch Perm from src to dst order */
        data = permute(data, ctx.clear_shuffled_src_order, true);
        data = merged_shuffle(data, ctx.src_order_shuffle, ctx.dst_order_shuffle, src_dst_idx);
        data = permute(data, ctx.clear_shuffled_dst_order);

        /* Gather-1 */
        data = gather_1(data);

        /* Switch Perm from dst to vtx order */
        data = permute(data, ctx.clear_shuffled_dst_order, true);
        data = merged_shuffle(data, ctx.dst_order_shuffle, ctx.vtx_order_shuffle, dst_vtx_idx);
        data = permute(data, ctx.clear_shuffled_vtx_order);

        /* Gather-2 */
        data = gather_2(data);

        apply();
    }
}
