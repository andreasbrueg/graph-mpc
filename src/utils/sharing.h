#pragma once

#include <omp.h>

#include "../io/comm.h"
#include "../setup/utils.h"
#include "graph.h"
#include "perm.h"
#include "protocol_config.h"
#include "types.h"

namespace share {

Row random_share_3P(ProtocolConfig &c);

Row random_share_secret_3P(ProtocolConfig &c, std::vector<Row> &vals_to_p1, size_t &idx, Row &secret);

Row random_share_secret_2P(ProtocolConfig &c, Row &secret);

std::vector<Row> random_share_secret_vec_2P(ProtocolConfig &c, std::vector<Row> &secret_vec);

Row reveal(ProtocolConfig &c, Row &share);

std::vector<Row> reveal_vec(ProtocolConfig &conf, std::vector<Row> &share);

Permutation reveal_perm(ProtocolConfig &conf, Permutation &share);

SecretSharedGraph random_share_graph(ProtocolConfig &conf, Graph &graph);

Graph reveal_graph(ProtocolConfig &conf, SecretSharedGraph &shared_graph);
};  // namespace share