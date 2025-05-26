#pragma once

#include "graph.h"
#include "protocol_config.h"
#include "sorting.h"

namespace mp {
std::vector<Row> propagate(ProtocolConfig &conf, std::vector<Row> &input_vector);

std::vector<Row> gather(ProtocolConfig &conf, std::vector<Row> &input_vector);

std::vector<Row> apply(ProtocolConfig &conf, std::vector<Row> &in1, std::vector<Row> &in2);

void run(ProtocolConfig &conf, SecretSharedGraph &graph, size_t n_iterations);
}  // namespace mp