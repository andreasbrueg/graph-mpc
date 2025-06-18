#include "../clip.h"

std::vector<Ring> clip::equals_zero(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t BLOCK_SIZE, std::vector<Ring> &input_share) {
    const size_t n_layers = 5;
    const size_t n_triples = n_layers * input_share.size();

    std::vector<Ring> result(input_share.size());

    std::vector<Ring> vals_to_p1;
    size_t idx = 0;

    if (id == P1) recv_vec(D, network, n_triples, vals_to_p1, BLOCK_SIZE);

    auto triples = mul::preprocess_bin(id, rngs, vals_to_p1, idx, n_triples);

    if (id == D) send_vec(P1, network, vals_to_p1.size(), vals_to_p1, BLOCK_SIZE);

    for (size_t i = 0; i < input_share.size(); ++i) {
        auto share = input_share[i];

        /* x_0 == -x_1 <=> ~x_0 ^ -x_1 */
        if (id == P0)
            share = ~share;
        else if (id == P1)
            share = -share;

        for (size_t layer = 0; layer < 5; ++layer) {
            if (id != D) {
                auto left = share;
                auto right = share;

                size_t width = 1 << (4 - layer);
                left >>= width;

                auto res = mul::evaluate_one_bin(id, network, BLOCK_SIZE, triples[(i * n_layers) + layer], left, right);

                if (layer == 4) {
                    res <<= 31;
                    res >>= 31;
                }

                share = res;
            }
        }
        result[i] = share;
    }
    return result;
}

std::vector<Ring> clip::B2A(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t BLOCK_SIZE, std::vector<Ring> &input_share) {
    const size_t n = input_share.size();

    auto triples = mul::preprocess(id, rngs, network, n, BLOCK_SIZE);

    std::vector<Ring> result(n);
    if (id == D) return result;

    std::vector<Ring> vals_send;
    for (size_t i = 0; i < n; ++i) {
        auto [a, b, mul] = triples[i];
        auto xa = (id == P0 ? 1 : 0) * (input_share[i] & 1) + a;
        auto yb = (id == P0 ? 0 : 1) * (input_share[i] & 1) + b;
        vals_send.push_back(xa);
        vals_send.push_back(yb);
    }

    std::vector<Ring> vals_receive;
    if (id == P0) {
        send_vec(P1, network, vals_send.size(), vals_send, BLOCK_SIZE);
        recv_vec(P1, network, 2 * n, vals_receive, BLOCK_SIZE);
    } else {
        recv_vec(P0, network, 2 * n, vals_receive, BLOCK_SIZE);
        send_vec(P0, network, vals_send.size(), vals_send, BLOCK_SIZE);
    }

    for (size_t i = 0; i < 2 * n; ++i) {
        vals_send[i] += vals_receive[i];
    }

    for (size_t i = 0; i < n; ++i) {
        auto [a, b, c] = triples[i];
        auto xa = vals_send[2 * i];
        auto yb = vals_send[2 * i + 1];

        auto mul_result = (xa * yb * (id)) - (xa * b) - (yb * a) + c;
        result[i] = (input_share[i] & 1) - 2 * mul_result;
    }

    return result;
}
