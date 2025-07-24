#pragma once

#include <omp.h>

#include <algorithm>
#include <chrono>
#include <functional>
#include <tuple>

#include "../setup/utils.h"
#include "../utils/bits.h"
#include "../utils/graph.h"
#include "clip.h"
#include "deduplication.h"
#include "permute.h"
#include "sort.h"
#include "utils/preprocessings.h"

namespace mp {

using F_apply = std::function<std::vector<Ring>(std::vector<Ring> &, std::vector<Ring> &)>;

using F_pre_mp = std::function<void(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, SecretSharedGraph &g)>;

using F_pre_mp_preprocess =
    std::function<void(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, size_t n_bits, MPPreprocessing &preproc)>;

using F_pre_mp_evaluate =
    std::function<void(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, MPPreprocessing &preproc, SecretSharedGraph &g)>;

using F_post_mp = std::function<void(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, SecretSharedGraph &g,
                                     std::vector<Ring> &new_payload)>;

using F_post_mp_preprocess =
    std::function<void(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, MPPreprocessing &preproc)>;

using F_post_mp_evaluate = std::function<void(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, SecretSharedGraph &g,
                                              MPPreprocessing &preproc, std::vector<Ring> &new_payload)>;

std::vector<Ring> propagate_1(std::vector<Ring> &input_vector, size_t n_vertices);

std::vector<Ring> propagate_2(std::vector<Ring> &input_vector, std::vector<Ring> &correction_vector);

std::vector<Ring> gather_1(std::vector<Ring> &input_vector);

std::vector<Ring> gather_2(std::vector<Ring> &input_vector, size_t n_vertices);

void init(Party id, SecretSharedGraph &g);

MPPreprocessing preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, size_t n_bits, size_t n_iterations,
                           F_pre_mp_preprocess f_preprocess, F_post_mp_preprocess f_postprocess);

void evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, size_t n_bits, size_t n_iterations, size_t n_vertices,
              SecretSharedGraph &g, std::vector<Ring> &weights, F_apply f_apply, F_pre_mp_evaluate f_preprocess, F_post_mp_evaluate f_postprocess,
              MPPreprocessing &preproc);

void run(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, size_t n_iterations, size_t n_vertices, SecretSharedGraph &g,
         std::vector<Ring> &weights, F_apply f_apply, F_pre_mp f_preprocess, F_post_mp f_postprocess);

}  // namespace mp