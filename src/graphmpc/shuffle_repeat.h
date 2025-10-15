#pragma once

#include "shuffle.h"

class ShuffleRepeat : public Shuffle {
   public:
    ShuffleRepeat(size_t f_id, ProtocolConfig *conf, std::unordered_map<Party, std::vector<Ring>> *preproc_vals, std::vector<Ring> *online_vals,
                  std::vector<Ring> input, std::vector<Ring> output, ShufflePre *perm_share, Party &recv)
        : Shuffle(f_id, conf, preproc_vals, online_vals, input, output, recv, perm_share) {}

    ShuffleRepeat(size_t f_id, ProtocolConfig *conf, std::unordered_map<Party, std::vector<Ring>> *preproc_vals, std::vector<Ring> *online_vals,
                  std::vector<Ring> input, std::vector<Ring> output, ShufflePre *perm_share, Party &recv, FileWriter *disk)
        : Shuffle(f_id, conf, preproc_vals, online_vals, input, output, recv, perm_share, disk) {}

    void preprocess() override {}
};