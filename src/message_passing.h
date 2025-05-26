#pragma once

#include "graph.h"
#include "protocol_config.h"
#include "sorting.h"

namespace mp {
void scatter(ProtocolConfig &conf, SecretSharedGraph &graph);

void gather(ProtocolConfig &conf, SecretSharedGraph &graph);

void apply(ProtocolConfig &conf, SecretSharedGraph &graph);
}