#include "compaction.h"

std::tuple<std::vector<Row>, std::vector<Row>, std::vector<Row>> compaction::preprocess_one(ProtocolConfig &c) {
    std::vector<Row> triple_a, triple_b, triple_c;

    for (size_t i = 0; i < c.n_rows; ++i) {
        triple_a.push_back(share::random_share_3P(c));
        triple_b.push_back(share::random_share_3P(c));
        Row mul = triple_a[i] * triple_b[i];
        triple_c.push_back(share::random_share_secret_3P(c, mul));
    }

    return {triple_a, triple_b, triple_c};
}

std::vector<std::tuple<std::vector<Row>, std::vector<Row>, std::vector<Row>>> compaction::preprocess(ProtocolConfig &c, size_t n) {
    std::vector<std::tuple<std::vector<Row>, std::vector<Row>, std::vector<Row>>> preproc;
    for (size_t i = 0; i < n; ++i) {
        preproc.push_back(preprocess_one(c));
    }
    return preproc;
}

Permutation compaction::evaluate_one(ProtocolConfig &c, std::vector<Row> &triple_a, std::vector<Row> &triple_b, std::vector<Row> &triple_c,
                                     std::vector<Row> &input_share) {
    std::vector<Row> output(c.n_rows);
    std::vector<Row> vals;

    size_t idx_mult = 0;

    if (c.pid != D) {
        std::vector<Row> f_0;
        // set f_0 to 1 - input and f_1 to input, we just immediately use input instead of f_1
        for (size_t i = 0; i < c.n_rows; ++i) {
            f_0.push_back(-input_share[i]);
            if (c.pid == P0) {
                f_0[i] += 1;  // 1 as constant only to one share
            }
        }

        std::vector<Row> s_0, s_1;
        Row s = 0;
        // Set s_0 to prefix sum of f_0 and s_1 to prefix sum of f_1/input continuing from the prior final value
        for (size_t i = 0; i < c.n_rows; ++i) {
            s += f_0[i];
            s_0.push_back(s);
        }
        for (size_t i = 0; i < c.n_rows; ++i) {
            s += input_share[i];
            s_1.push_back(s - s_0[i]);  // s_0[i] see below
        }

        // We now have to compute s_0 + input * (s_1 - s_0) (element-wise multiplication).
        // Previously, we already subtracted s_0 from s_1, so we just compute s_0 + input * s_1
        // s_0 is added after the communication though, here, we just multiply.

        for (size_t i = 0; i < c.n_rows; i++) {
            auto xa = triple_a[i] + input_share[i];
            auto yb = triple_b[i] + s_1[i];
            vals.push_back(xa);
            vals.push_back(yb);
        }

        std::vector<Row> data_recv(2 * c.n_rows);
        if (c.pid == P0) {
            send_vec(P1, c.network, vals.size(), vals, c.BLOCK_SIZE);
            recv_vec(P1, c.network, data_recv, c.BLOCK_SIZE);
        } else if (c.pid == P1) {
            recv_vec(P0, c.network, data_recv, c.BLOCK_SIZE);
            send_vec(P0, c.network, vals.size(), vals, c.BLOCK_SIZE);
        }

        for (size_t i = 0; i < vals.size(); ++i) vals[i] += data_recv[i];

        // Now, finalize the multiplications and add vector s_0.
        for (size_t i = 0; i < c.n_rows; ++i) {
            auto a = triple_a[i];
            auto b = triple_b[i];
            auto mul = triple_c[i];

            auto xa = vals[2 * idx_mult];
            auto yb = vals[2 * idx_mult + 1];

            output[i] = s_0[i] + (xa * yb * (c.pid)) - (xa * b) - (yb * a) + mul;
            if (c.pid == P0) output[i]--;
            idx_mult++;
        }
    }
    return Permutation(output);
}

Permutation compaction::get_compaction(ProtocolConfig &c, std::vector<Row> &input_share) {
    auto [triple_a, triple_b, triple_c] = preprocess_one(c);
    c.network->sync();
    return evaluate_one(c, triple_a, triple_b, triple_c, input_share);
}
