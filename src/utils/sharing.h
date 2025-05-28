#pragma once

#include <omp.h>

#include "../io/comm.h"
#include "../setup/utils.h"
#include "graph.h"
#include "perm.h"
#include "protocol_config.h"
#include "types.h"

namespace share {

Row random_share_3P(Party pid, RandomGenerators &rngs);

Row random_share_secret_3P(Party pid, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, Row &secret);

Row random_share_secret_2P(Party pid, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, Row &secret);

std::vector<Row> random_share_secret_vec_2P(Party pid, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, std::vector<Row> &secret_vec);

Row reveal(Party partner, std::shared_ptr<io::NetIOMP> network, Row &share);

std::vector<Row> reveal_vec(ProtocolConfig &conf, std::vector<Row> &share);

Permutation reveal_perm(ProtocolConfig &conf, Permutation &share);

SecretSharedGraph random_share_graph(ProtocolConfig &conf, Graph &graph);

Graph reveal_graph(ProtocolConfig &conf, SecretSharedGraph &shared_graph);
};  // namespace share