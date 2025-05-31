#pragma once

#include <memory>
#include <tuple>
#include <vector>

#include "../io/comm.h"
#include "../utils/perm.h"
#include "../utils/protocol_config.h"
#include "../utils/sharing.h"
#include "../utils/types.h"

namespace compaction {
/**
 * Returns the multiplication triples as a tuple: {triple_a, triple_b, triple_c}
 */
std::tuple<std::vector<Ring>, std::vector<Ring>, std::vector<Ring>> preprocess(ProtocolConfig &c);

/**
 * Evaluates the compaction using the preprocessed triples
 */
Permutation evaluate(ProtocolConfig &c, std::vector<Ring> &triple_a, std::vector<Ring> &triple_b, std::vector<Ring> &triple_c, std::vector<Ring> &input_share);

/**
 * Ad-hoc preprocessing and evaluation
 */
Permutation get_compaction(ProtocolConfig &c, std::vector<Ring> &input_share);

};  // namespace compaction