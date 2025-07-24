#pragma once

#include <omp.h>

#include <algorithm>
#include <chrono>
#include <functional>
#include <tuple>

#include "../setup/comm.h"
#include "../setup/utils.h"
#include "sort.h"
#include "utils/bits.h"
#include "utils/graph.h"
#include "utils/preprocessings.h"

namespace mp {

using F_apply = std::function<std::vector<Ring>(std::vector<Ring> &, std::vector<Ring> &)>;

std::vector<Ring> propagate_1(std::vector<Ring> &input_vector, size_t n_vertices);

std::vector<Ring> propagate_2(std::vector<Ring> &input_vector, std::vector<Ring> &correction_vector);

std::vector<Ring> gather_1(std::vector<Ring> &input_vector);

std::vector<Ring> gather_2(std::vector<Ring> &input_vector, size_t n_vertices);

void prepare_permutations(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc);

MPPreprocessing preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t n_bits, size_t n_iterations);

void evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t n_bits, size_t n_iterations, size_t n_vertices,
              MPPreprocessing &preproc, F_apply f_apply, std::vector<Ring> &weights, SecretSharedGraph &g);

}  // namespace mp