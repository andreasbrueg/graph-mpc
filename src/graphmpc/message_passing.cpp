#include "message_passing.h"

MPPreprocessing mp::preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t n_bits, size_t n_iterations) {
    MPPreprocessing preproc;

    auto sort_pre_start = std::chrono::system_clock::now();

    /* Sort preprocessing */
    if (preproc.deduplication_pre.is_null()) {  // TODO: change branch condition
        sort::get_sort_preprocess(id, rngs, network, n, n_bits + 1, preproc);
        sort::get_sort_preprocess(id, rngs, network, n, n_bits + 1, preproc);
    } else {
        sort::get_sort_preprocess(id, rngs, network, n, n_bits + 2, preproc);  // One bit more, as deduplication appends a bit to the end
        sort::sort_iteration_preprocess(id, rngs, network, n, preproc);        // Only one iteration, as dst_sort has almost been completed during deduplication
    }
    sort::sort_iteration_preprocess(id, rngs, network, n, preproc);  // vtx_order sort iteration

    auto sort_pre_end = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(sort_pre_end - sort_pre_start);
    std::cout << "Sort Preprocessing took: " << std::to_string(elapsed.count()) << " s" << std::endl;

    /* Apply / Switch Perm Preprocessing */
    auto sw_pre_start = std::chrono::system_clock::now();

    preproc.vtx_order_shuffle = shuffle::get_shuffle(id, rngs, network, n);  // Shuffle for applying/switching to vertex order
    preproc.src_order_shuffle = shuffle::get_shuffle(id, rngs, network, n);  // Shuffle for switching to src order
    preproc.dst_order_shuffle = shuffle::get_shuffle(id, rngs, network, n);  // Shuffle for switching to dst order

    preproc.vtx_src_merge = shuffle::get_merged_shuffle(id, rngs, network, n, preproc.vtx_order_shuffle,
                                                        preproc.src_order_shuffle);  // Merged shuffle to switch between vtx and src order
    preproc.src_dst_merge = shuffle::get_merged_shuffle(id, rngs, network, n, preproc.src_order_shuffle,
                                                        preproc.dst_order_shuffle);  // Merged shuffle to swtich from src order to dst order
    preproc.dst_vtx_merge = shuffle::get_merged_shuffle(id, rngs, network, n, preproc.dst_order_shuffle,
                                                        preproc.vtx_order_shuffle);  // Merged shuffle to switch from dst order to vtx order

    auto sw_pre_end = std::chrono::system_clock::now();
    elapsed = std::chrono::duration_cast<std::chrono::seconds>(sw_pre_end - sw_pre_start);
    std::cout << "Switch Perm Preprocessing took: " << std::to_string(elapsed.count()) << " s" << std::endl;
    return preproc;
}

void mp::prepare_permutations(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc) {
    /* Prepare vtx order */
    auto shuffled_vtx_order = shuffle::shuffle(id, rngs, network, n, preproc.vtx_order_shuffle, preproc.vtx_order);
    auto clear_vtx_order = share::reveal_perm(id, network, shuffled_vtx_order);
    preproc.clear_shuffled_vtx_order = clear_vtx_order;

    /* Prepare src order */
    auto shuffled_src_order = shuffle::shuffle(id, rngs, network, n, preproc.src_order_shuffle, preproc.src_order);
    preproc.clear_shuffled_src_order = share::reveal_perm(id, network, shuffled_src_order);

    /* Prepare dst order */
    auto shuffled_dst_order = shuffle::shuffle(id, rngs, network, n, preproc.dst_order_shuffle, preproc.dst_order);
    preproc.clear_shuffled_dst_order = share::reveal_perm(id, network, shuffled_dst_order);
}

void mp::evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t n_bits, size_t n_iterations, size_t n_vertices,
                  MPPreprocessing &preproc, F_apply f_apply, std::vector<Ring> &weights, SecretSharedGraph &g) {
    /* Compute the three permutations */
    std::cout << "Start generating sorts..." << std::endl;
    auto sort_eval_start = std::chrono::system_clock::now();

    preproc.src_order = sort::get_sort_evaluate(id, rngs, network, n, preproc, g.src_order_bits);
    // if (preproc.deduplication_pre.is_null()) {
    preproc.dst_order = sort::get_sort_evaluate(id, rngs, network, n, preproc, g.dst_order_bits);
    //} else {
    ///* One more sort iteration to get dst_order */
    // preproc.dst_order =
    // sort::sort_iteration_evaluate(id, rngs, network, n, preproc.dst_order, preproc.dst_order_sort_iteration_pre, g.dst_order_bits[n_bits + 1]);
    //}
    preproc.vtx_order = sort::sort_iteration_evaluate(id, rngs, network, n, preproc.src_order, preproc, g.inverted_isV);
    auto sort_eval_end = std::chrono::system_clock::now();

    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(sort_eval_end - sort_eval_start);
    std::cout << "Sort Evaluation took: " << std::to_string(elapsed.count()) << " s" << std::endl << std::endl;

    /* Bring payload into vertex order */
    auto payload_v = g.payload;

    /* Prepare Permutations for apply / switch perm */
    prepare_permutations(id, rngs, network, n, preproc);

    /* Apply Perm */
    auto shuffled_input_share = shuffle::shuffle(id, rngs, network, n, preproc.vtx_order_shuffle, payload_v);
    payload_v = preproc.clear_shuffled_vtx_order(shuffled_input_share);

    // Run message passing interations
    for (size_t i = 0; i < n_iterations; ++i) {
        /* Add current weight only to vertices */
        if (id == P0) {
#pragma omp parallel for if (n_vertices > 10000)
            for (size_t j = 0; j < n_vertices; ++j) payload_v[j] += weights[weights.size() - 1 - i];
        }

        /* Propagate-1 */
        auto payload_p = propagate_1(payload_v, n_vertices);

        /* Switch Perm from vtx to src order */
        auto shuffled_payload_p = preproc.clear_shuffled_vtx_order.inverse()(payload_p);
        auto double_shuffled_payload_p = shuffle::shuffle(id, rngs, network, n, preproc.vtx_src_merge, shuffled_payload_p);
        auto payload_src = preproc.clear_shuffled_src_order(double_shuffled_payload_p);

        auto shuffled_payload_v = preproc.clear_shuffled_vtx_order.inverse()(payload_v);
        auto double_shuffled_payload_v = shuffle::shuffle(id, rngs, network, n, preproc.vtx_src_merge, shuffled_payload_v);
        auto payload_corr = preproc.clear_shuffled_src_order(double_shuffled_payload_v);

        /* Propagate-2 */
        payload_p = propagate_2(payload_src, payload_corr);

        /* Switch Perm from src to dst order*/
        shuffled_payload_p = preproc.clear_shuffled_src_order.inverse()(payload_p);
        double_shuffled_payload_p = shuffle::shuffle(id, rngs, network, n, preproc.src_dst_merge, shuffled_payload_p);
        auto payload_dst = preproc.clear_shuffled_dst_order(double_shuffled_payload_p);

        /* Gather-1*/
        payload_p = gather_1(payload_dst);

        /* Switch Perm from dst to vtx order */
        shuffled_payload_p = preproc.clear_shuffled_dst_order.inverse()(payload_p);
        double_shuffled_payload_p = shuffle::shuffle(id, rngs, network, n, preproc.dst_vtx_merge, shuffled_payload_p);
        payload_p = preproc.clear_shuffled_vtx_order(double_shuffled_payload_p);

        /* Gather-2 */
        auto update = gather_2(payload_p, n_vertices);

        /* Apply */
        payload_v = f_apply(payload_v, update);
    }
    g.payload = payload_v;
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