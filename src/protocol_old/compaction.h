#pragma once

#include <omp.h>

#include <cassert>
#include <memory>
#include <tuple>
#include <vector>

#include "../io/comm.h"
#include "../utils/mul.h"
#include "../utils/perm.h"
#include "../utils/sharing.h"
#include "../utils/types.h"

namespace compaction {

std::vector<std::tuple<Ring, Ring, Ring>> preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n);

Permutation evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, std::vector<std::tuple<Ring, Ring, Ring>> &triples,
                     std::vector<Ring> &input_share);

Permutation get_compaction(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, std::vector<Ring> &input_share);

};  // namespace compaction