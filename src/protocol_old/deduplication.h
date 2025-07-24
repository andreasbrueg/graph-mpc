#pragma once

#include "../utils/types.h"
#include "clip.h"
#include "permute.h"
#include "sort.h"
#include "utils/preprocessings.h"

DeduplicationPreprocessing deduplication_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, size_t n_bits);

void deduplication_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, DeduplicationPreprocessing &preproc,
                            SecretSharedGraph &g);

void deduplication(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, std::vector<std::vector<Ring>> &src_bits,
                   std::vector<std::vector<Ring>> &dst_bits, std::vector<Ring> &src, std::vector<Ring> &dst);
