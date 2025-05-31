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
std::tuple<std::vector<Row>, std::vector<Row>, std::vector<Row>> preprocess_one(ProtocolConfig &c);

/**
 * Returns vector that contains n preprocessed triples
 */
std::vector<std::tuple<std::vector<Row>, std::vector<Row>, std::vector<Row>>> preprocess(ProtocolConfig &c, size_t n);

/**
 * Evaluates the compaction using the preprocessed triples
 */
Permutation evaluate_one(ProtocolConfig &c, std::vector<Row> &triple_a, std::vector<Row> &triple_b, std::vector<Row> &triple_c, std::vector<Row> &input_share);

/**
 * Ad-hoc preprocessing and evaluation
 */
Permutation get_compaction(ProtocolConfig &c, std::vector<Row> &input_share);

};  // namespace compaction