#include "processor.h"

void Processor::add(FunctionToken function) { queue.push_back(function); }

void Processor::run_mp_preprocessing(size_t n_iterations) {
    pre_completed = false;

    queue.push_back(SORT);
    queue.push_back(SORT);
    queue.push_back(SORT_ITERATION);

    queue.push_back(APPLY_PERM);

    for (size_t i = 0; i < n_iterations; ++i) {
        for (size_t j = 0; j < 4; ++j) {
            queue.push_back(SWITCH_PERM);
        }
    }

    run_preprocessing_dealer();
    run_preprocessing_parties();
    pre_completed = true;
}

void Processor::run_mp_evaluation(size_t n_iterations, size_t n_vertices) {
    if (id == D) return;

    if (!pre_completed) throw std::logic_error("Preprocessing has to complete before evaluation.");

    mp::evaluate(id, rngs, network, n, BLOCK_SIZE, g, n_iterations, n_vertices, mp_preproc);
}

size_t Processor::preprocessing_data_size(Party id) {
    size_t data_recv_P0 = 0;
    size_t data_recv_P1 = 0;

    size_t sort_idx = 0;

    for (auto &func : queue) {
        switch (func) {
            case COMPACTION: {
                data_recv_P1 += n;  // P1 receives all c's for the n triples
                break;
            }
            case SHUFFLE: {
                data_recv_P0 += n;      // P0 receives B_0
                data_recv_P1 += 2 * n;  // P1 receives B_1 and pi_1_p
                break;
            }
            case UNSHUFFLE: {
                data_recv_P0 += n;
                data_recv_P1 += n;
                break;
            }
            case SORT: {
                size_t n_bits = sizeof(Ring) * 8 + 1;
                data_recv_P0 += (n_bits - 1) * 2 * n;      // P0 receives n_bits-1 B_0's for shuffle and unshuffle respectively
                data_recv_P1 += (n_bits - 1) * 4 * n + n;  // P1 receives the same as P0, but n_bits triple shares on top
                sort_idx++;
                break;
            }
            case SORT_ITERATION: {
                data_recv_P0 += 2 * n;
                data_recv_P1 += 4 * n;
                break;
            }
            case SWITCH_PERM: {
                data_recv_P0 += 4 * n;  // pi_B0, omega_B0, merged_B0, sigma_0_p
                data_recv_P1 += 6 * n;  // pi_shares_P1 (2n), omega_shares_P1 (2n), merged_B0, sigma_1
                break;
            }
            case APPLY_PERM: {
                data_recv_P0 += n;      // P0 receives B_0
                data_recv_P1 += 2 * n;  // P1 receives B_1 and pi_1_p
                break;
            }
            default:
                break;
        }
    }
    if (id == P0) {
        return data_recv_P0;
    } else {
        return data_recv_P1;
    }
}

void Processor::run_preprocessing_parties() {
    if (id == D) return;

    std::vector<Ring> vals;
    size_t idx = 0;
    size_t n_elems = preprocessing_data_size(id);

    recv_vec(D, network, n_elems, vals, BLOCK_SIZE);

    size_t sort_idx = 0;

    for (auto &func : queue) {
        switch (func) {
            case COMPACTION: {
                auto triples = compaction::preprocess_Parties(id, rngs, n, vals, idx);
                preproc.compaction_triples.push_back(triples);
                break;
            }
            case SHUFFLE: {
                ShufflePre perm_share = shuffle::get_shuffle_2(id, rngs, n, vals, idx, false);
                preproc.shuffle_shares.push_back(perm_share);
                break;
            }
            case UNSHUFFLE: {
                auto B = shuffle::get_unshuffle_2(id, n, vals, idx);
                preproc.unshuffle_Bs.push_back(B);
                break;
            }
            case SORT: {
                size_t n_bits = sizeof(Ring) * 8 + 1;
                SortPreprocessing sort_preproc = sort::get_sort_preprocess_Parties(id, rngs, n, n_bits, vals, idx);

                if (sort_idx == 0) {
                    mp_preproc.src_order_pre = sort_preproc;
                } else if (sort_idx == 1) {
                    mp_preproc.dst_order_pre = sort_preproc;
                }

                sort_idx++;
                break;
            }
            case SORT_ITERATION: {
                SortIterationPreprocessing sort_iteration_pre = sort::sort_iteration_preprocess_Parties(id, rngs, n, vals, idx);
                mp_preproc.vtx_order_pre = sort_iteration_pre;
                break;
            }
            case SWITCH_PERM: {
                SwitchPermPreprocessing sw_perm_preproc = sort::switch_perm_preprocess_Parties(id, rngs, n, vals, idx);
                mp_preproc.sw_perm_pre.push_back(sw_perm_preproc);
                break;
            }
            case APPLY_PERM: {
                ShufflePre perm_share = shuffle::get_shuffle_2(id, rngs, n, vals, idx, true);
                mp_preproc.apply_perm_pre = perm_share;
                break;
            }
            default:
                break;
        }
    }
}

void Processor::run_preprocessing_dealer() {
    if (id != D) return;

    std::vector<Ring> vals_P0;
    std::vector<Ring> vals_P1;

    size_t sort_idx = 0;

    for (auto &func : queue) {
        switch (func) {
            case COMPACTION: {
                auto shares_P1 = compaction::preprocess_Dealer(id, rngs, n);
                for (auto &val : shares_P1) vals_P1.push_back(val);
                break;
            }
            case SHUFFLE: {
                auto [perm_share, B_0, shares_P1] = shuffle::get_shuffle_1(id, rngs, n);
                for (auto &val : B_0) vals_P0.push_back(val);
                for (auto &val : shares_P1) vals_P1.push_back(val);
                last_shuffle = perm_share;
                break;
            }
            case UNSHUFFLE: {
                auto [B_0, B_1] = shuffle::get_unshuffle_1(id, rngs, n, last_shuffle);
                for (auto &val : B_0) vals_P0.push_back(val);
                for (auto &val : B_1) vals_P1.push_back(val);
                break;
            }
            case SORT: {
                size_t n_bits = sizeof(Ring) * 8 + 1;

                auto preproc = sort::get_sort_preprocess_Dealer(id, rngs, n, n_bits);
                auto [sort_vals_P0, sort_vals_P1] = preproc.to_vals();
                vals_P0.insert(vals_P0.end(), sort_vals_P0.begin(), sort_vals_P0.end());
                vals_P1.insert(vals_P1.end(), sort_vals_P1.begin(), sort_vals_P1.end());

                sort_idx++;
                break;
            }
            case SORT_ITERATION: {
                auto preproc = sort::sort_iteration_preprocess_Dealer(id, rngs, n);
                auto [sort_it_vals_P0, sort_it_vals_P1] = preproc.to_vals();
                vals_P0.insert(vals_P0.end(), sort_it_vals_P0.begin(), sort_it_vals_P0.end());
                vals_P1.insert(vals_P1.end(), sort_it_vals_P1.begin(), sort_it_vals_P1.end());
                break;
            }
            case SWITCH_PERM: {
                SwitchPermPreprocessing_Dealer preproc = sort::switch_perm_preprocess_Dealer(id, rngs, n);
                auto [sw_perm_vals_P0, sw_perm_vals_P1] = preproc.to_vals();
                vals_P0.insert(vals_P0.end(), sw_perm_vals_P0.begin(), sw_perm_vals_P0.end());
                vals_P1.insert(vals_P1.end(), sw_perm_vals_P1.begin(), sw_perm_vals_P1.end());
                break;
            }
            case APPLY_PERM: {
                auto [perm_share, B_0, shares_P1] = shuffle::get_shuffle_1(id, rngs, n);
                for (auto &val : B_0) vals_P0.push_back(val);
                for (auto &val : shares_P1) vals_P1.push_back(val);
                break;
            }
            default:
                break;
        }
    }

    send_vec(P0, network, vals_P0.size(), vals_P0, BLOCK_SIZE);
    send_vec(P1, network, vals_P1.size(), vals_P1, BLOCK_SIZE);
}

/*
void Processor::run_evaluation() {
    if (id == D) return;

    if (!pre_completed) throw std::logic_error("Preprocessing needs to run before evaluation.");

    size_t compaction_idx = 0;
    size_t shuffle_idx = 0;
    size_t unshuffle_idx = 0;
    size_t sort_idx = 0;
    size_t sw_perm_idx = 0;

    for (auto &func : queue) {
        switch (func) {
            case COMPACTION: {
                compaction_idx++;
                break;
            }
            case SHUFFLE: {
                ShufflePre perm_share = preproc.shuffle_shares[shuffle_idx];
                wire = shuffle::shuffle(id, rngs, network, wire, perm_share, n, BLOCK_SIZE);
                last_shuffle = perm_share;
                shuffle_idx++;
                break;
            }
            case UNSHUFFLE: {
                auto unshuffle_B = preproc.unshuffle_Bs[unshuffle_idx];
                wire = shuffle::unshuffle(id, rngs, network, last_shuffle, unshuffle_B, wire, n, BLOCK_SIZE);
                unshuffle_idx++;
                break;
            }
            case SORT: {
                auto sort_preproc = preproc.sort_preproc_2[sort_idx];
                if (sort_idx == 0) {
                    sort_wire = src_order_bits;
                } else if (sort_idx == 1) {
                    sort_wire = dst_order_bits;
                }

                Permutation res = sort::get_sort_evaluate(id, rngs, network, n, BLOCK_SIZE, sort_wire, sort_preproc);

                if (sort_idx == 0) {
                    src_order = res;
                    auto src_rev = share::reveal_perm(id, network, BLOCK_SIZE, src_order);
                } else if (sort_idx == 1) {
                    dst_order = res;
                } else if (sort_idx == 2) {
                    vtx_order = res;
                }

                auto dst_rev = share::reveal_perm(id, network, BLOCK_SIZE, dst_order);
                auto vtx_rev = share::reveal_perm(id, network, BLOCK_SIZE, vtx_order);

                sort_idx++;
                break;
            }
            case SWITCH_PERM: {
                auto sw_perm_preproc = preproc.sw_perm_preproc[sw_perm_idx];
                auto pi = sw_perm_preproc.pi_share;
                auto omega = sw_perm_preproc.omega_share;
                auto merged = sw_perm_preproc.merged_share;

                Permutation p1;
                Permutation p2;

                if (sw_perm_idx % 4 == 0 || sw_perm_idx % 4 == 1) {
                    p1 = vtx_order;
                    p2 = src_order;
                } else if (sw_perm_idx % 4 == 2) {
                    p1 = src_order;
                    p2 = dst_order;
                } else if (sw_perm_idx % 4 == 3) {
                    p1 = dst_order;
                    p2 = vtx_order;
                }

                wire = sort::switch_perm_evaluate(id, rngs, network, n, BLOCK_SIZE, p1, p2, pi, omega, merged, wire);
                sw_perm_idx++;
                break;
            }
            case APPLY_PERM: {
                ShufflePre perm_share = preproc.shuffle_shares[shuffle_idx];
                wire = sort::apply_perm_evaluate(id, rngs, network, n, BLOCK_SIZE, vtx_order, perm_share, wire);
                shuffle_idx++;
            }
        }
    }
}


void Processor::run_evaluation_1(std::vector<Ring> &send) {
    size_t compaction_idx = 0;
    size_t shuffle_idx = 0;
    size_t sort_idx = 0;

    for (auto &func : queue) {
        switch (func) {
            case COMPACTION: {
                auto triples = preproc.compaction_triples[compaction_idx];
                auto vals = compaction::evaluate_1(id, n, triples, wire);
                for (auto &elem : vals) send.push_back(elem);
                compaction_idx++;
                break;
            }
            case SHUFFLE: {
                ShufflePre shuffle_share = preproc.shuffle_shares[shuffle_idx];
                auto vec_A = shuffle::shuffle_1(id, rngs, n, wire, shuffle_share, false);
                for (auto &elem : vec_A) send.push_back(elem);
                shuffle_idx++;
                break;
            }
            case SORT: {
                sort::sort_evaluate_1();
                break;
            }
            default:
                break;
        }
    }
}

void Processor::run_evaluation_2(std::vector<Ring> &vals) {
    size_t n_bits = sizeof(Ring) * 8;
    size_t compaction_comm = 2 * n;
    size_t shuffle_comm = n;
    size_t sort_comm = n_bits * compaction_comm + (n_bits - 1) * (shuffle_comm + shuffle_comm);

    size_t compaction_idx = 0;
    size_t shuffle_idx = 0;
    size_t sort_idx = 0;

    for (auto &func : queue) {
        size_t idx = shuffle_idx * shuffle_comm + compaction_idx * compaction_comm + sort_idx * sort_comm;

        switch (func) {
            case COMPACTION: {
                auto triples = preproc.compaction_triples[compaction_idx];
                std::vector<Ring> compaction_vals(vals.begin() + idx, vals.begin() + idx + 2 * n);
                compaction::evaluate_2(id, n, triples, compaction_vals, wire);
                compaction_idx++;
                break;
            }
            case SHUFFLE: {
                ShufflePre shuffle_share = preproc.shuffle_shares[shuffle_idx];
                std::vector<Ring> vec_A(vals.begin() + idx, vals.begin() + idx + n);
                wire = shuffle::shuffle_2(id, rngs, vec_A, shuffle_share, n, false);
                shuffle_idx++;
                break;
            }
            case SORT: {
                sort_idx++;
                break;
            }
            default:
                break;
        }
    }
}

size_t Processor::evaluation_data_size() {
    size_t data_recv = 0;

    for (auto &func : queue) {
        switch (func) {
            case COMPACTION: {
                data_recv += 2 * n;
                break;
            }
            case SHUFFLE: {
                data_recv += n;  // Each receives vec_A(n)
                break;
            }
            case SORT: {
                size_t n_bits = sizeof(Ring) * 8;
                size_t compaction_comm = 2 * n;
                size_t shuffle_comm = n;
                size_t unshuffle_comm = n;
                data_recv += n_bits * compaction_comm + (n_bits - 1) * (shuffle_comm + unshuffle_comm);
                break;
            }
            default:
                break;
        }
    }
    return data_recv;
}

std::vector<Ring> Processor::add_vals(std::vector<Ring> &sent, std::vector<Ring> &received) {
    size_t compaction_idx = 0;
    size_t shuffle_idx = 0;
    size_t sort_idx = 0;

    std::vector<Ring> result(sent.size());

    for (auto &func : queue) {
        size_t idx = shuffle_idx * shuffle_comm + compaction_idx * compaction_comm + sort_idx * sort_comm;
        switch (func) {
            case COMPACTION: {
                for (size_t i = idx; i < idx + compaction_comm; ++i) {
                    result[i] = sent[i] + received[i];
                }
                compaction_idx++;
                break;
            }
            case SHUFFLE: {
                shuffle_idx++;
                break;
            }
            case SORT: {
                auto _idx = idx;
                for (size_t j = 0; j < n_bits; ++j) {
                    for (size_t i = _idx; i < _idx + compaction_comm; ++i) {
                        result[i] = sent[i] + received[i];
                    }
                    _idx += compaction_comm;
                }
                sort_idx++;
            }
        }
    }
}
*/