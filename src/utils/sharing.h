#pragma once

#include <omp.h>

#include "graph.h"
#include "perm.h"
#include "protocol_config.h"
#include "types.h"

namespace share {

Row random_share(Party pid, RandomGenerators &rngs);

Row random_share_secret(Party pid, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, Row &secret);

void random_share_secret_send(Party dst_pid, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, Row &share, Row &secret);

void random_share_secret_recv(Party src_pid, std::shared_ptr<io::NetIOMP> network, Row &share);

void random_share_secret_vec_send(Party dst_pid, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, std::vector<Row> &share_vec,
                                  std::vector<Row> &secret_vec);

void random_share_secret_vec_recv(Party src_pid, std::shared_ptr<io::NetIOMP> network, std::vector<Row> &share_vec);

SecretSharedGraph random_share_graph(ProtocolConfig &conf, Graph &graph);

Graph reveal_graph(ProtocolConfig &conf, SecretSharedGraph &shared_graph);

std::vector<Row> reveal_vec(Party partner, std::shared_ptr<io::NetIOMP> network, std::vector<Row> &share_vec);

Row reveal(Party partner, std::shared_ptr<io::NetIOMP> network, Row &share);

std::vector<Row> reveal(ProtocolConfig &conf, std::vector<Row> &share);

Permutation reveal(ProtocolConfig &conf, Permutation &share);

};  // namespace share