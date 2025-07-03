#include "message_passing.h"

/* ----- Helper Functions ----- */
std::tuple<std::vector<std::vector<Ring>>, std::vector<std::vector<Ring>>, std::vector<Ring>> mp::init(Party id, SecretSharedGraph &g) {
    /* Generate vector containing { 1-isV, src[0], src[1], ..., src[n_bits - 1] } */
    std::vector<std::vector<Ring>> src_order_bits(g.src_bits.size() + 1);

    std::vector<Ring> inverted_isV(g.isV_bits.size());
    for (size_t i = 0; i < inverted_isV.size(); ++i) {
        inverted_isV[i] = -g.isV_bits[i];
        if (id == P0) inverted_isV[i] += 1;
    }

    std::copy(g.src_bits.begin(), g.src_bits.end(), src_order_bits.begin() + 1);
    src_order_bits[0] = inverted_isV;

    /* Generate vector containing { isV, dst[0], dst[1], ..., dst[n_bits - 1] } */
    std::vector<std::vector<Ring>> dst_order_bits(g.dst_bits.size() + 1);

    std::copy(g.dst_bits.begin(), g.dst_bits.end(), dst_order_bits.begin() + 1);
    dst_order_bits[0] = g.isV_bits;

    return {src_order_bits, dst_order_bits, inverted_isV};
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

    f_preprocess(id, rngs, network, n, preproc);
    auto sort_pre_start = std::chrono::system_clock::now();

    preproc.src_order_pre = sort::get_sort_preprocess(id, rngs, network, n, n_bits + 1);
    preproc.dst_order_pre = sort::get_sort_preprocess(id, rngs, network, n, n_bits + 1);
    preproc.vtx_order_pre = sort::sort_iteration_preprocess(id, rngs, network, n);

    auto sort_pre_end = std::chrono::system_clock::now();

    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(sort_pre_end - sort_pre_start);
    std::cout << "Sort Preprocessing took: " << std::to_string(elapsed.count()) << " s" << std::endl;

    auto apply_pre_start = std::chrono::system_clock::now();
    preproc.apply_perm_share = shuffle::get_shuffle(id, rngs, network, n, true);
    auto apply_pre_end = std::chrono::system_clock::now();

    elapsed = std::chrono::duration_cast<std::chrono::seconds>(apply_pre_end - apply_pre_start);
    std::cout << "Apply Preprocessing took: " << std::to_string(elapsed.count()) << " s" << std::endl;

    auto sw_pre_start = std::chrono::system_clock::now();
    for (size_t j = 0; j < 3; ++j) {
        preproc.sw_perm_pre.push_back(permute::switch_perm_preprocess(id, rngs, network, n));
    }
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
    /* Preprocess graph before message passing */
    f_preprocess(id, rngs, network, n, preproc, g);

    /* Get the bit vectors for performing get_sort */
    auto [src_order_bits, dst_order_bits, inverted_isV] = init(id, g);

    /* Compute the three permutations */
    auto sort_eval_start = std::chrono::system_clock::now();
    Permutation src_order = sort::get_sort_evaluate(id, rngs, network, n, src_order_bits, preproc.src_order_pre);
    Permutation dst_order = sort::get_sort_evaluate(id, rngs, network, n, dst_order_bits, preproc.dst_order_pre);
    Permutation vtx_order = sort::sort_iteration_evaluate(id, rngs, network, n, src_order, inverted_isV, preproc.vtx_order_pre);
    auto sort_eval_end = std::chrono::system_clock::now();

    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(sort_eval_end - sort_eval_start);
    std::cout << "Sort Evaluation took: " << std::to_string(elapsed.count()) << " s" << std::endl;
    /* Bring payload into vertex order */
    // auto payload_v = from_bits(g.payload_bits, g.size);
    auto payload_v = g.payload;

    auto apply_eval_start = std::chrono::system_clock::now();
    payload_v = permute::apply_perm_evaluate(id, rngs, network, n, vtx_order, preproc.apply_perm_share, payload_v);
    auto apply_eval_end = std::chrono::system_clock::now();
    elapsed = std::chrono::duration_cast<std::chrono::seconds>(apply_eval_end - apply_eval_start);
    std::cout << "Apply Evaluation took: " << std::to_string(elapsed.count()) << " s" << std::endl;

    for (size_t i = 0; i < n_iterations; ++i) {
        /* Add current weight only to vertices */
        if (id == P0) {
#pragma omp parallel for if (n_vertices > 10000)
            for (size_t j = 0; j < n_vertices; ++j) payload_v[j] += weights[weights.size() - 1 - i];
        }

        /* Propagate-1 */
        auto propagate_1_start = std::chrono::system_clock::now();
        auto payload_p = propagate_1(payload_v, n_vertices);
        auto propagate_1_end = std::chrono::system_clock::now();
        elapsed = std::chrono::duration_cast<std::chrono::seconds>(propagate_1_end - propagate_1_start);
        std::cout << "Propagate 1 took: " << std::to_string(elapsed.count()) << " s" << std::endl;

        /* Switch Perm to src order */
        auto sw_eval_start = std::chrono::system_clock::now();
        auto payload_src = permute::switch_perm_evaluate(id, rngs, network, n, vtx_order, src_order, preproc.sw_perm_pre[0], payload_p);
        auto payload_corr = permute::switch_perm_evaluate(id, rngs, network, n, vtx_order, src_order, preproc.sw_perm_pre[0], payload_v);
        auto sw_eval_end = std::chrono::system_clock::now();
        elapsed = std::chrono::duration_cast<std::chrono::seconds>(sw_eval_end - sw_eval_start);
        std::cout << "Switch Perm Evaluation took: " << std::to_string(elapsed.count()) << " s" << std::endl;

        /* Propagate-2 */
        auto propagate_2_start = std::chrono::system_clock::now();
        payload_p = propagate_2(payload_src, payload_corr);
        auto propagate_2_end = std::chrono::system_clock::now();
        elapsed = std::chrono::duration_cast<std::chrono::seconds>(propagate_2_end - propagate_2_start);
        std::cout << "Propagate 2 took: " << std::to_string(elapsed.count()) << " s" << std::endl;

        /* Switch Perm to dst order*/
        sw_eval_start = std::chrono::system_clock::now();
        auto payload_dst = permute::switch_perm_evaluate(id, rngs, network, n, src_order, dst_order, preproc.sw_perm_pre[1], payload_p);
        sw_eval_end = std::chrono::system_clock::now();
        elapsed = std::chrono::duration_cast<std::chrono::seconds>(sw_eval_end - sw_eval_start);
        std::cout << "Switch Perm Evaluation took: " << std::to_string(elapsed.count()) << " s" << std::endl;

        /* Gather-1*/
        auto gather_1_start = std::chrono::system_clock::now();
        payload_p = gather_1(payload_dst);
        auto gather_1_end = std::chrono::system_clock::now();
        elapsed = std::chrono::duration_cast<std::chrono::seconds>(gather_1_end - gather_1_start);
        std::cout << "Gather 1 Perm Evaluation took: " << std::to_string(elapsed.count()) << " s" << std::endl;

        /* Switch Perm to vtx order */
        sw_eval_start = std::chrono::system_clock::now();
        payload_p = permute::switch_perm_evaluate(id, rngs, network, n, dst_order, vtx_order, preproc.sw_perm_pre[2], payload_p);
        sw_eval_end = std::chrono::system_clock::now();
        elapsed = std::chrono::duration_cast<std::chrono::seconds>(sw_eval_end - sw_eval_start);
        std::cout << "Switch Perm Evaluation took: " << std::to_string(elapsed.count()) << " s" << std::endl;

        /* Gather-2 */
        auto gather_2_start = std::chrono::system_clock::now();
        auto update = gather_2(payload_p, n_vertices);
        auto gather_2_end = std::chrono::system_clock::now();
        elapsed = std::chrono::duration_cast<std::chrono::seconds>(gather_2_end - gather_2_start);
        std::cout << "Gather 2 Evaluation took: " << std::to_string(elapsed.count()) << " s" << std::endl;

        /* Apply */
        auto apply_start = std::chrono::system_clock::now();
        payload_v = f_apply(payload_v, update);
        auto apply_end = std::chrono::system_clock::now();
        elapsed = std::chrono::duration_cast<std::chrono::seconds>(apply_end - apply_start);
        std::cout << "Apply Evaluation took: " << std::to_string(elapsed.count()) << " s" << std::endl;
    }

    auto postprocess_start = std::chrono::system_clock::now();
    f_postprocess(id, rngs, network, n, g, preproc, payload_v);
    auto postprocess_end = std::chrono::system_clock::now();
    elapsed = std::chrono::duration_cast<std::chrono::seconds>(postprocess_end - postprocess_start);
    std::cout << "Postprocess Evaluation took: " << std::to_string(elapsed.count()) << " s" << std::endl;
}

/* ----- Ad-Hoc Preprocessing ----- */

void mp::run(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, size_t n_iterations, size_t n_vertices,
             SecretSharedGraph &g, std::vector<Ring> &weights, F_apply f_apply, F_pre_mp f_preprocess, F_post_mp f_postprocess) {
    /* Preprocess graph before message passing */
    f_preprocess(id, rngs, network, n, g);

    auto [src_order_bits, dst_order_bits, inverted_isV] = init(id, g);

    /* Compute the three permutations */
    Permutation src_order = sort::get_sort(id, rngs, network, n, src_order_bits);
    Permutation dst_order = sort::get_sort(id, rngs, network, n, dst_order_bits);
    Permutation vtx_order = sort::sort_iteration(id, rngs, network, n, src_order, inverted_isV);

    /* Bring payload into vertex order */
    auto payload_v = from_bits(g.payload_bits, g.size);
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

        /* Propagate-2 */
        payload_p = propagate_2(payload_src, payload_corr);

        /* Switch Perm to dst order*/
        auto payload_dst = permute::switch_perm(id, rngs, network, n, src_order, dst_order, payload_p);

        /* Gather-1*/
        payload_p = gather_1(payload_dst);

        /* Switch Perm to vtx order */
        payload_p = permute::switch_perm(id, rngs, network, n, dst_order, vtx_order, payload_p);

        /* Gather-2 */
        auto update = gather_2(payload_p, n_vertices);

        /* Apply */
        payload_v = f_apply(payload_v, update);
    }

    f_postprocess(id, rngs, network, n, g, payload_v);
}
