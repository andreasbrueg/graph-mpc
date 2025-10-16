#include "mp_protocol.h"

void MPProtocol::preprocess() {
    if (id != D) {
        size_t n_recv;
        network->recv(D, &n_recv, sizeof(size_t));
        if (ssd) {
            network->recv_vec(D, n_recv, preproc_disk);
        } else {
            network->recv_vec(D, n_recv, ctx.preproc.at(id));
        }
    }

    for (size_t level_idx = 0; level_idx < circ.size(); ++level_idx) {
        auto &level = circ[level_idx];

        for (size_t f_idx = 0; f_idx < level.size(); ++f_idx) {
            auto &f = level[f_idx];
            if (f) f->preprocess();
        }
    }

    if (id == D) {
        for (auto &party : {P0, P1}) {
            auto &data_send = ctx.preproc.at(party);
            size_t n_send = data_send.size();
            network->send(party, &n_send, sizeof(size_t));
            network->send_vec(party, n_send, data_send);
        }
    }
}

void MPProtocol::evaluate() {
    if (id != D) {
        for (size_t level_idx = 0; level_idx < circ.size(); ++level_idx) {
            auto &level = circ[level_idx];

            for (size_t f_idx = 0; f_idx < level.size(); ++f_idx) {
                auto &f = level[f_idx];
                if (f) f->evaluate_send();
            }

            online_communication();

            for (size_t f_idx = 0; f_idx < level.size(); ++f_idx) {
                auto &f = level[f_idx];
                if (f) f->evaluate_recv();
            }
        }
    }
    std::cout << "Communication round: " << comm_ms << " ms" << std::endl;
    std::cout << "Sending/Receiving: " << sr_ms << " ms" << std::endl << std::endl;

    reset();
}

void MPProtocol::online_communication() {
    auto round_begin = std::chrono::high_resolution_clock::now();

    size_t n_send = ctx.mult_vals.size() + ctx.and_vals.size() + ctx.shuffle_vals.size() + ctx.reveal_vals.size();
    size_t n_recv = 0;

    std::vector<Ring> send_vals;
    send_vals.insert(send_vals.end(), ctx.mult_vals.begin(), ctx.mult_vals.end());
    send_vals.insert(send_vals.end(), ctx.and_vals.begin(), ctx.and_vals.end());
    send_vals.insert(send_vals.end(), ctx.shuffle_vals.begin(), ctx.shuffle_vals.end());
    send_vals.insert(send_vals.end(), ctx.reveal_vals.begin(), ctx.reveal_vals.end());

    auto comm_begin = std::chrono::high_resolution_clock::now();
    if (id == P0) {
        network->send(P1, &n_send, sizeof(size_t));
        network->send_vec(P1, n_send, send_vals);
        network->recv(P1, &n_recv, sizeof(size_t));
        ctx.data_recv.resize(n_recv);
        network->recv_vec(P1, n_recv, ctx.data_recv);
    } else {
        network->recv(P0, &n_recv, sizeof(size_t));
        ctx.data_recv.resize(n_recv);
        network->recv_vec(P0, n_recv, ctx.data_recv);
        network->send(P0, &n_send, sizeof(size_t));
        network->send_vec(P0, n_send, send_vals);
    }
    auto comm_end = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < ctx.mult_vals.size(); i++) {
        ctx.mult_vals[i] += ctx.data_recv[i];
    }
    for (size_t i = 0; i < ctx.and_vals.size(); i++) {
        ctx.and_vals[i] ^= ctx.data_recv[ctx.mult_vals.size() + i];
    }
    for (size_t i = 0; i < ctx.shuffle_vals.size(); i++) {
        ctx.shuffle_vals[i] = ctx.data_recv[ctx.mult_vals.size() + ctx.and_vals.size() + i];
    }
    for (size_t i = 0; i < ctx.reveal_vals.size(); i++) {
        ctx.reveal_vals[i] += ctx.data_recv[ctx.mult_vals.size() + ctx.and_vals.size() + ctx.shuffle_vals.size() + i];
    }
    ctx.data_recv.clear();

    auto round_end = std::chrono::high_resolution_clock::now();

    auto time_round = std::chrono::duration_cast<std::chrono::milliseconds>(round_end - round_begin);
    auto time_comm = std::chrono::duration_cast<std::chrono::milliseconds>(comm_end - comm_begin);

    comm_ms += time_round.count();
    sr_ms += time_comm.count();
}

void MPProtocol::build() {
    init_inputs();
    pre_mp();
    compute_sorts();
    prepare_shuffles();
    if (w.mp_data_parallel.size() > 0) {  // Only the case for pi_r
        for (size_t i = 0; i < w.mp_data_parallel.size(); ++i) {
            w.buf = w.mp_data_parallel[i];
            w.data = w.mp_data_parallel[i];
            message_passing();
            w.mp_data_parallel[i] = w.data;
        }
    } else {
        message_passing();
    }
    post_mp();
    g.data = w.data;
    build_circuit();

    for (size_t i = 0; i < shuffle_idx + 1; ++i) {
        ShufflePre perm_share;
        if (id == P0) {
            perm_share.pi_0.perm_vec.resize(size);
            perm_share.pi_0_p.perm_vec.resize(size);
            perm_share.B.resize(size);
            perm_share.R.resize(size);
        }
        if (id == P1) {
            perm_share.pi_1.perm_vec.resize(size);
            perm_share.pi_1_p.perm_vec.resize(size);
            perm_share.B.resize(size);
            perm_share.R.resize(size);
        }
        if (id == D) {
            perm_share.pi_0.perm_vec.resize(size);
            perm_share.pi_1.perm_vec.resize(size);
        }
        shuffles.push_back(std::make_shared<ShufflePre>(perm_share));
    }
}

void MPProtocol::build_circuit() {
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

void MPProtocol::init_inputs() {
    if (id != D) {
        for (size_t i = 0; i < bits; ++i) {
            for (size_t j = 0; j < size; ++j) {
                input_to_val[i * size + j] = g.src_order_bits[i][j];
            }
        }
        for (size_t i = 0; i < bits; ++i) {
            for (size_t j = 0; j < size; ++j) {
                input_to_val[bits * size + i * size + j] = g.dst_order_bits[i][j];
            }
        }
        for (size_t i = 0; i < size; ++i) {
            input_to_val[2 * bits * size + i] = g.isV_inv[i];
        }
        for (size_t i = 0; i < size; ++i) {
            input_to_val[2 * bits * size + size + i] = g.data[i];
        }
    }

    for (size_t i = 0; i < bits + 1; ++i) {
        w.src_order_bits[i] = add_input();
    }
    for (size_t i = 0; i < bits + 1; ++i) {
        w.dst_order_bits[i] = add_input();
    }
    w.isV_inv = add_input();
    w.data = add_input();
}

json MPProtocol::details() {
    size_t rounds = circ.size();
    json output;
    output["details"] = {{"Party", id}, {"Size", size}, {"Nodes", nodes}, {"Depth", depth}, {"Bits", bits}, {"SSD", ssd}, {"Communication Rounds: ", rounds}};
    return output;
}

void MPProtocol::print() {
    auto to_print = details();
    std::cout << "--- Protocol Details ---\n";
    for (const auto &[key, value] : to_print["details"].items()) {
        std::cout << key << ": " << value << "\n";
    }
    std::cout << std::endl;
}

void MPProtocol::set_input(Graph &graph) { g = graph; }

Graph MPProtocol::get_output() { return g; }

void MPProtocol::benchmark_graph() {
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

void MPProtocol::reset() {
    std::filesystem::remove("preproc_" + std::to_string(id) + ".bin");
    std::filesystem::remove("triples_" + std::to_string(id) + ".bin");
    preproc_disk = FileWriter(id, "preproc_" + std::to_string(id) + ".bin");
    triples_disk = FileWriter(id, "triples_" + std::to_string(id) + ".bin");

    rngs.reseed();
    recv_shuffle = P0;
    recv_mul = P0;
    input_to_val.resize(2 * (bits + 1) * size + 2 * size);

    current_layer = 0;
    comm_ms = 0;
    sr_ms = 0;

    shuffles.clear();

    ctx.preproc[P0].clear();
    ctx.preproc[P1].clear();

    ctx.mult_vals.clear();
    ctx.and_vals.clear();
    ctx.shuffle_vals.clear();
    ctx.reveal_vals.clear();
    ctx.data_recv.clear();
}
