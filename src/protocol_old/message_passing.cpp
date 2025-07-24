#include "message_passing.h"

/* ----- Helper Functions ----- */
void mp::init(Party id, SecretSharedGraph &g) {
    /* Generate vector containing { 1-isV, src[0], src[1], ..., src[n_bits - 1] } */
    std::vector<std::vector<Ring>> src_order_bits(g.src_bits.size() + 1);

    std::vector<Ring> inverted_isV(g.isV.size());
    for (size_t i = 0; i < inverted_isV.size(); ++i) {
        inverted_isV[i] = -g.isV[i];
        if (id == P0) inverted_isV[i] += 1;
    }

    std::copy(g.src_bits.begin(), g.src_bits.end(), src_order_bits.begin() + 1);
    src_order_bits[0] = inverted_isV;

    /* Generate vector containing { isV, dst[0], dst[1], ..., dst[n_bits - 1] } */
    std::vector<std::vector<Ring>> dst_order_bits(g.dst_bits.size() + 1);

    std::copy(g.dst_bits.begin(), g.dst_bits.end(), dst_order_bits.begin() + 1);
    dst_order_bits[0] = g.isV;

    g.src_order_bits = src_order_bits;
    g.dst_order_bits = dst_order_bits;
    g.inverted_isV = inverted_isV;
}

/* Input vector needs to be in vertex order */
std::vector<Ring> mp::propagate_1(std::vector<Ring> &input_vector, size_t n_vertices) {
    std::vector<Ring> data(input_vector.size());
    for (size_t i = n_vertices - 1; i > 0; --i) {
        data[i] = input_vector[i] - input_vector[i - 1];
    }
    data[0] = input_vector[0];
    for (size_t i = n_vertices; i < data.size(); ++i) {
        data[i] = input_vector[i];
    }
    return data;
}

/* Input vector needs to be in source order */
std::vector<Ring> mp::propagate_2(std::vector<Ring> &input_vector, std::vector<Ring> &correction_vector) {
    std::vector<Ring> data(input_vector.size());
    Ring sum = 0;
    for (size_t i = 0; i < data.size(); ++i) {
        sum += input_vector[i];
        data[i] = sum - correction_vector[i];
    }
    return data;
}

/* Input vector needs to be in destination order */
std::vector<Ring> mp::gather_1(std::vector<Ring> &input_vector) {
    std::vector<Ring> data(input_vector.size());
    Ring sum = 0;
    for (size_t i = 0; i < data.size(); ++i) {
        sum += input_vector[i];
        data[i] = sum;
    }
    return data;
}

/* Input vector needs to be in vertex order */
std::vector<Ring> mp::gather_2(std::vector<Ring> &input_vector, size_t n_vertices) {
    std::vector<Ring> data(input_vector.size());
    Ring sum = 0;

    for (size_t i = 0; i < n_vertices; ++i) {
        data[i] = input_vector[i] - sum;
        sum += data[i];
    }
#pragma omp_parallel for if (data.size() - num_v > 1000)
    for (size_t i = n_vertices; i < data.size(); ++i) {
        data[i] = 0;
    }
    return data;
}

/* ----- Preprocessing ----- */
MPPreprocessing mp::preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, size_t n_bits, size_t n_iterations,
                               F_pre_mp_preprocess f_preprocess, F_post_mp_preprocess f_postprocess) {
    MPPreprocessing preproc;

    f_preprocess(id, rngs, network, n, n_bits, preproc);
    auto sort_pre_start = std::chrono::system_clock::now();

    if (preproc.deduplication_pre.is_null()) {
        preproc.src_order_pre = sort::get_sort_preprocess(id, rngs, network, n, n_bits + 1);
        preproc.dst_order_pre = sort::get_sort_preprocess(id, rngs, network, n, n_bits + 1);
    } else {
        preproc.src_order_pre = sort::get_sort_preprocess(id, rngs, network, n, n_bits + 2);
        preproc.dst_order_sort_iteration_pre = sort::sort_iteration_preprocess(id, rngs, network, n);
    }
    preproc.vtx_order_pre = sort::sort_iteration_preprocess(id, rngs, network, n);

    auto sort_pre_end = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(sort_pre_end - sort_pre_start);
    std::cout << "Sort Preprocessing took: " << std::to_string(elapsed.count()) << " s" << std::endl;

    auto sw_pre_start = std::chrono::system_clock::now();

    permute::switch_perm_preprocess(id, rngs, network, n, preproc);

    auto sw_pre_end = std::chrono::system_clock::now();
    elapsed = std::chrono::duration_cast<std::chrono::seconds>(sw_pre_end - sw_pre_start);
    std::cout << "Switch Perm Preprocessing took: " << std::to_string(elapsed.count()) << " s" << std::endl;

    f_postprocess(id, rngs, network, n, preproc);

    return preproc;
}

/* ----- Evaluation ----- */
void mp::evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, size_t n_bits, size_t n_iterations, size_t n_vertices,
                  SecretSharedGraph &g, std::vector<Ring> &weights, F_apply f_apply, F_pre_mp_evaluate f_preprocess, F_post_mp_evaluate f_postprocess,
                  MPPreprocessing &preproc) {
    /* Get the bit vectors for performing get_sort */
    init(id, g);

    /* Preprocess graph before message passing */
    f_preprocess(id, rngs, network, n, preproc, g);

    /* Compute the three permutations */
    std::cout << "Start generating sorts..." << std::endl;
    auto sort_eval_start = std::chrono::system_clock::now();
    preproc.src_order = sort::get_sort_evaluate(id, rngs, network, n, g.src_order_bits, preproc.src_order_pre);
    if (preproc.deduplication_pre.is_null()) {
        preproc.dst_order = sort::get_sort_evaluate(id, rngs, network, n, g.dst_order_bits, preproc.dst_order_pre);
    } else {
        /* One more sort iteration to get dst_order */
        preproc.dst_order =
            sort::sort_iteration_evaluate(id, rngs, network, n, preproc.dst_order, g.dst_order_bits[n_bits + 1], preproc.dst_order_sort_iteration_pre);
    }
    preproc.vtx_order = sort::sort_iteration_evaluate(id, rngs, network, n, preproc.src_order, g.inverted_isV, preproc.vtx_order_pre);
    auto sort_eval_end = std::chrono::system_clock::now();

    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(sort_eval_end - sort_eval_start);
    std::cout << "Sort Evaluation took: " << std::to_string(elapsed.count()) << " s" << std::endl << std::endl;

    /* Bring payload into vertex order */
    auto payload_v = g.payload;

    payload_v = permute::apply_perm_evaluate(id, rngs, network, n, preproc.vtx_order, preproc, payload_v);

    for (size_t i = 0; i < n_iterations; ++i) {
        /* Add current weight only to vertices */
        if (id == P0) {
#pragma omp parallel for if (n_vertices > 10000)
            for (size_t j = 0; j < n_vertices; ++j) payload_v[j] += weights[weights.size() - 1 - i];
        }

        /* Propagate-1 */
        auto payload_p = propagate_1(payload_v, n_vertices);

        /* Switch Perm to src order */
        auto payload_src = permute::switch_perm_evaluate(id, rngs, network, n, preproc.vtx_order, preproc.src_order, preproc, payload_p);
        auto payload_corr = permute::switch_perm_evaluate(id, rngs, network, n, preproc.vtx_order, preproc.src_order, preproc, payload_v);
        auto src_rev = share::reveal_vec(id, network, payload_src);
        std::cout << "Source order: ";
        for (size_t i = 0; i < src_rev.size(); ++i) {
            if (i != src_rev.size() - 1) {
                std::cout << src_rev[i] << ", ";
            } else {
                std::cout << src_rev[i] << std::endl;
            }
        }
        /* Propagate-2 */
        payload_p = propagate_2(payload_src, payload_corr);

        /* Switch Perm to dst order*/
        auto payload_dst = permute::switch_perm_evaluate(id, rngs, network, n, preproc.src_order, preproc.dst_order, preproc, payload_p);
        auto dst_rev = share::reveal_vec(id, network, payload_dst);
        std::cout << "Destination order: ";
        for (size_t i = 0; i < dst_rev.size(); ++i) {
            if (i != dst_rev.size() - 1) {
                std::cout << dst_rev[i] << ", ";
            } else {
                std::cout << dst_rev[i] << std::endl;
            }
        }

        /* Gather-1*/
        payload_p = gather_1(payload_dst);

        /* Switch Perm to vtx order */
        payload_p = permute::switch_perm_evaluate(id, rngs, network, n, preproc.dst_order, preproc.vtx_order, preproc, payload_p);

        /* Gather-2 */
        auto update = gather_2(payload_p, n_vertices);

        /* Apply */
        payload_v = f_apply(payload_v, update);
        auto vtx_rev = share::reveal_vec(id, network, payload_v);
        std::cout << "Vertex order: ";
        for (size_t i = 0; i < vtx_rev.size(); ++i) {
            if (i != vtx_rev.size() - 1) {
                std::cout << vtx_rev[i] << ", ";
            } else {
                std::cout << vtx_rev[i] << std::endl;
            }
        }
    }

    auto postprocess_start = std::chrono::system_clock::now();
    f_postprocess(id, rngs, network, n, g, preproc, payload_v);
    auto postprocess_end = std::chrono::system_clock::now();
    elapsed = std::chrono::duration_cast<std::chrono::seconds>(postprocess_end - postprocess_start);
    std::cout << "Postprocess Evaluation took: " << std::to_string(elapsed.count()) << " s" << std::endl << std::endl;
}

/* ----- Ad-Hoc Preprocessing ----- */

void mp::run(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, size_t n_iterations, size_t n_vertices,
             SecretSharedGraph &g, std::vector<Ring> &weights, F_apply f_apply, F_pre_mp f_preprocess, F_post_mp f_postprocess) {
    /* Preprocess graph before message passing */
    f_preprocess(id, rngs, network, n, g);

    init(id, g);

    /* Compute the three permutations */
    Permutation src_order = sort::get_sort(id, rngs, network, n, g.src_order_bits);
    Permutation dst_order = sort::get_sort(id, rngs, network, n, g.dst_order_bits);
    Permutation vtx_order = sort::sort_iteration(id, rngs, network, n, src_order, g.inverted_isV);

    /* Bring payload into vertex order */
    auto payload_v = g.payload;
    payload_v = permute::apply_perm(id, rngs, network, n, vtx_order, payload_v);

    for (size_t i = 0; i < n_iterations; ++i) {
        /* Add current weight */
        if (id == P0) {
            for (size_t j = 0; j < n_vertices; ++j) payload_v[j] += weights[weights.size() - 1 - i];
        }

        /* Propagate-1 */
        auto payload_p = propagate_1(payload_v, n_vertices);

        /* Switch Perm to src order */
        auto payload_src = permute::switch_perm(id, rngs, network, n, vtx_order, src_order, payload_p);
        auto payload_corr = permute::switch_perm(id, rngs, network, n, vtx_order, src_order, payload_v);

        auto payload_src_rev = share::reveal_vec(id, network, payload_src);

        /* Propagate-2 */
        payload_p = propagate_2(payload_src, payload_corr);

        /* Switch Perm to dst order*/
        auto payload_dst = permute::switch_perm(id, rngs, network, n, src_order, dst_order, payload_p);

        auto payload_dst_rev = share::reveal_vec(id, network, payload_dst);

        /* Gather-1*/
        payload_p = gather_1(payload_dst);

        /* Switch Perm to vtx order */
        payload_p = permute::switch_perm(id, rngs, network, n, dst_order, vtx_order, payload_p);

        auto payload_vtx_rev = share::reveal_vec(id, network, payload_p);

        /* Gather-2 */
        auto update = gather_2(payload_p, n_vertices);

        /* Apply */
        payload_v = f_apply(payload_v, update);
    }

    f_postprocess(id, rngs, network, n, g, payload_v);
}
