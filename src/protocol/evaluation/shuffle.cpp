#include "../shuffle.h"

std::vector<Ring> shuffle::shuffle_1(Party id, RandomGenerators &rngs, size_t n, Permutation &input_share, ShufflePre &perm_share) {
    std::vector<Ring> perm_vec = input_share.get_perm_vec();
    return shuffle_1(id, rngs, n, perm_vec, perm_share);
}

std::vector<Ring> shuffle::shuffle_1(Party id, RandomGenerators &rngs, size_t n, std::vector<Ring> &input_share, ShufflePre &perm_share) {
    std::vector<Ring> shuffled_share(n);
    std::vector<Ring> vec_A(n);

    Permutation perm;

    std::vector<Ring> R;

    if (perm_share.R.empty()) {
        Ring rand;

        /* Sampling 1: R_0 / R_1 */
        for (size_t i = 0; i < n; ++i) {
            if (id == P0)
                rngs.rng_D0().random_data(&rand, sizeof(Ring));
            else if (id == P1)
                rngs.rng_D1().random_data(&rand, sizeof(Ring));
            R.push_back(rand);
        }
        if (perm_share.save) perm_share.R = R;
    } else {
        R = perm_share.R;
    }

    /* Sampling 2: pi_0_p / pi_1 */
    if (id == P0) {
        /* Reuse pi_0_p if saved earlier */
        if (perm_share.pi_0_p.not_null())
            perm = perm_share.pi_0_p;
        else
            perm = Permutation::random(n, rngs.rng_D0());
        if (perm_share.save) perm_share.pi_0_p = perm;
    } else {
        /* Reuse pi_1 if saved earlier */
        if (perm_share.pi_1.not_null())
            perm = perm_share.pi_1;
        else
            perm = Permutation::random(n, rngs.rng_D1());
        if (perm_share.save) perm_share.pi_1 = perm;
    }

    /* Compute input + R */
    for (size_t j = 0; j < n; ++j) {
        vec_A[j] = input_share[j] + R[j];
    }

    /* Compute perm(input + R) */
    vec_A = perm(vec_A);

    return vec_A;
}

std::vector<Ring> shuffle::shuffle_2(Party id, RandomGenerators &rngs, std::vector<Ring> &vec_A, ShufflePre &perm_share, size_t n) {
    Permutation perm;

    if (id == P0) {
        /* Sampling 3: pi_0 */
        if (perm_share.pi_0.not_null())
            perm = perm_share.pi_0;
        else
            perm = Permutation::random(n, rngs.rng_D0());
        if (perm_share.save) perm_share.pi_0 = perm;
    } else {
        if (perm_share.pi_1_p.not_null())
            perm = perm_share.pi_1_p;
        else
            perm = Permutation::random(n, rngs.rng_D1());
    }

    vec_A = perm(vec_A);

    for (size_t i = 0; i < n; ++i) {
        vec_A[i] -= perm_share.B[i];
    }

    return vec_A;
}

std::vector<Ring> shuffle::unshuffle_1(Party id, RandomGenerators &rngs, ShufflePre &shuffle_share, std::vector<Ring> &input_share, size_t n) {
    std::vector<Ring> vec_t(n);
    std::vector<Ring> R;
    Ring rand;

    /* Sampling 1: R_0 / R_1 */
    for (size_t i = 0; i < n; ++i) {
        if (id == P0)
            rngs.rng_D0_unshuffle().random_data(&rand, sizeof(Ring));
        else
            rngs.rng_D1_unshuffle().random_data(&rand, sizeof(Ring));
        R.push_back(rand);
    }

    for (size_t i = 0; i < n; ++i) vec_t[i] = input_share[i] + R[i];

    Permutation perm = id == P0 ? shuffle_share.pi_0 : shuffle_share.pi_1_p;
    vec_t = perm.inverse()(vec_t);

    return vec_t;
}

std::vector<Ring> shuffle::unshuffle_2(Party id, ShufflePre &shuffle_share, std::vector<Ring> vec_t, std::vector<Ring> &B, size_t n) {
    /* Apply inverse */
    Permutation perm;
    perm = id == P0 ? shuffle_share.pi_0_p : shuffle_share.pi_1;
    vec_t = perm.inverse()(vec_t);

    /* Last step: subtract B_0 / B_1 */
    for (size_t i = 0; i < n; ++i) vec_t[i] -= B[i];
    return vec_t;
}

Permutation shuffle::shuffle(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, Permutation &input_share, ShufflePre &perm_share, size_t n,
                             size_t BLOCK_SIZE) {
    auto perm_vec = input_share.get_perm_vec();
    return Permutation(shuffle(id, rngs, network, perm_vec, perm_share, n, BLOCK_SIZE));
}

std::vector<Ring> shuffle::shuffle(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, std::vector<Ring> &input_share,
                                   ShufflePre &perm_share, size_t n, size_t BLOCK_SIZE) {
    std::vector<Ring> shuffled_share(n);
    std::vector<Ring> vec_A(n);

    if (id == D) return shuffled_share;

    Permutation perm;

    std::vector<Ring> R;

    if (perm_share.R.empty()) {
        Ring rand;

        /* Sampling 1: R_0 / R_1 */
        for (size_t i = 0; i < n; ++i) {
            if (id == P0)
                rngs.rng_D0().random_data(&rand, sizeof(Ring));
            else if (id == P1)
                rngs.rng_D1().random_data(&rand, sizeof(Ring));
            R.push_back(rand);
        }
        if (perm_share.save) perm_share.R = R;
    } else {
        R = perm_share.R;
    }

    /* Sampling 2: pi_0_p / pi_1 */
    if (id == P0) {
        /* Reuse pi_0_p if saved earlier */
        if (perm_share.pi_0_p.not_null())
            perm = perm_share.pi_0_p;
        else
            perm = Permutation::random(n, rngs.rng_D0());
        if (perm_share.save) perm_share.pi_0_p = perm;
    } else {
        /* Reuse pi_1 if saved earlier */
        if (perm_share.pi_1.not_null())
            perm = perm_share.pi_1;
        else
            perm = Permutation::random(n, rngs.rng_D1());
        if (perm_share.save) perm_share.pi_1 = perm;
    }

    /* Compute input + R */
    for (size_t j = 0; j < n; ++j) {
        vec_A[j] = input_share[j] + R[j];
    }

    /* Compute perm(input + R) */
    vec_A = perm(vec_A);

    /* Send A to the other party */
    if (id == P0) {
        send_vec(P1, network, vec_A.size(), vec_A, BLOCK_SIZE);
        recv_vec(P1, network, vec_A, BLOCK_SIZE);
    } else {
        std::vector<Ring> data_recv(n);
        recv_vec(P0, network, data_recv, BLOCK_SIZE);
        send_vec(P0, network, vec_A.size(), vec_A, BLOCK_SIZE);
        std::copy(data_recv.begin(), data_recv.end(), vec_A.begin());
    }

    std::copy(vec_A.begin(), vec_A.end(), shuffled_share.begin());

    if (id == P0) {
        /* Sampling 3: pi_0 */
        if (perm_share.pi_0.not_null())
            perm = perm_share.pi_0;
        else
            perm = Permutation::random(n, rngs.rng_D0());
        if (perm_share.save) perm_share.pi_0 = perm;
    } else {
        if (perm_share.pi_1_p.not_null())
            perm = perm_share.pi_1_p;
        else
            perm = Permutation::random(n, rngs.rng_D1());
    }

    shuffled_share = perm(shuffled_share);

    for (size_t i = 0; i < n; ++i) {
        shuffled_share[i] -= perm_share.B[i];
    }

    return shuffled_share;
}

std::vector<Ring> shuffle::unshuffle(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, ShufflePre &shuffle_share, std::vector<Ring> &B,
                                     std::vector<Ring> &input_share, size_t n, size_t BLOCK_SIZE) {
    std::vector<Ring> output_share(n);

    if (id == D) return output_share;

    std::vector<Ring> vec_t(n);
    std::vector<Ring> R;
    Ring rand;

    /* Sampling 1: R_0 / R_1 */
    for (size_t i = 0; i < n; ++i) {
        if (id == P0)
            rngs.rng_D0_unshuffle().random_data(&rand, sizeof(Ring));
        else
            rngs.rng_D1_unshuffle().random_data(&rand, sizeof(Ring));
        R.push_back(rand);
    }

    for (size_t i = 0; i < n; ++i) vec_t[i] = input_share[i] + R[i];

    Permutation perm = id == P0 ? shuffle_share.pi_0 : shuffle_share.pi_1_p;
    vec_t = perm.inverse()(vec_t);

    /* Send and receive t_0/t_1 */
    if (id == P0) {
        send_vec(P1, network, vec_t.size(), vec_t, BLOCK_SIZE);
        recv_vec(P1, network, vec_t, BLOCK_SIZE);
    } else {
        std::vector<Ring> data_recv(n);
        recv_vec(P0, network, data_recv, BLOCK_SIZE);
        send_vec(P0, network, vec_t.size(), vec_t, BLOCK_SIZE);
        std::copy(data_recv.begin(), data_recv.end(), vec_t.begin());
    }

    /* Apply inverse */
    perm = id == P0 ? shuffle_share.pi_0_p : shuffle_share.pi_1;
    output_share = perm.inverse()(vec_t);

    /* Last step: subtract B_0 / B_1 */
    for (size_t i = 0; i < n; ++i) output_share[i] -= B[i];

    return output_share;
}

Permutation shuffle::unshuffle(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, ShufflePre &shuffle_share,
                               std::vector<Ring> &B, Permutation &input_share) {
    auto perm_vec = input_share.get_perm_vec();
    return Permutation(unshuffle(id, rngs, network, shuffle_share, B, perm_vec, n, BLOCK_SIZE));
}
