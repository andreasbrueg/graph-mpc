#include "mp_protocol.h"

/* Case deduplication */
void MPProtocol::add_compute_sorts() {
    ctx.src_order = add_sort(w.src_order_bits, bits + 1);
    ctx.dst_order = add_sort(w.dst_order_bits, bits + 1);
    ctx.vtx_order = add_sort_iteration(ctx.src_order, w.isV);  // TODO: Invert isV!
}

void MPProtocol::build_initialization() {
    add_compute_sorts();
    /* Prepare vtx order */
    ctx.vtx_order = add_shuffle(ctx.vtx_order, &ctx.vtx_order_shuffle);
    ctx.src_order = add_shuffle(ctx.src_order, &ctx.src_order_shuffle);
    ctx.dst_order = add_shuffle(ctx.dst_order, &ctx.dst_order_shuffle);

    ctx.clear_shuffled_vtx_order = add_reveal(ctx.vtx_order);
    ctx.clear_shuffled_src_order = add_reveal(ctx.src_order);
    ctx.clear_shuffled_dst_order = add_reveal(ctx.dst_order);

    w.data = add_shuffle(w.data, &ctx.vtx_order_shuffle);
    w.mp_data_vtx = add_permute(w.data, ctx.clear_shuffled_vtx_order);
}

void MPProtocol::build_message_passing() {
    for (size_t i = 0; i < depth; ++i) {
        f_queue.emplace_back(std::make_unique<AddWeights>(&conf, &w.mp_data, &weights, i));

        w.mp_data_corr = w.mp_data;

        /* Propagate-1 */
        add_propagate_1(w.mp_data);

        /* Switch Perm from vtx to src order */
        w.mp_data = add_permute(w.mp_data, ctx.clear_shuffled_vtx_order, true);
        w.mp_data = add_merged_shuffle(w.mp_data, ctx.vtx_src_merge, ctx.vtx_order_shuffle, ctx.src_order_shuffle);
        w.mp_data = add_permute(w.mp_data, ctx.clear_shuffled_src_order);

        w.mp_data_corr = add_permute(w.mp_data_corr, ctx.clear_shuffled_vtx_order, true);
        w.mp_data_corr = add_merged_shuffle(w.mp_data_corr, ctx.vtx_src_merge, ctx.vtx_order_shuffle, ctx.src_order_shuffle);
        w.mp_data_corr = add_permute(w.mp_data_corr, ctx.clear_shuffled_src_order);

        /* Propagate-2 */
        w.mp_data = add_propagate_2(w.mp_data, w.mp_data_corr);

        /* Switch Perm from src to dst order */
        w.mp_data = add_permute(w.mp_data, ctx.clear_shuffled_src_order, true);
        w.mp_data = add_merged_shuffle(w.mp_data, ctx.src_dst_merge, ctx.src_order_shuffle, ctx.dst_order_shuffle);
        w.mp_data = add_permute(w.mp_data, ctx.clear_shuffled_dst_order);

        /* Gather-1 */
        w.mp_data = add_gather_1(w.mp_data);

        /* Switch Perm from dst to vtx order */
        w.mp_data = add_permute(w.mp_data, ctx.clear_shuffled_dst_order, true);
        w.mp_data = add_merged_shuffle(w.mp_data, ctx.dst_vtx_merge, ctx.dst_order_shuffle, ctx.vtx_order_shuffle);
        w.mp_data = add_permute(w.mp_data, ctx.clear_shuffled_vtx_order);

        /* Gather-2 */
        w.mp_data = add_gather_2(w.mp_data);

        apply();
    }
}
