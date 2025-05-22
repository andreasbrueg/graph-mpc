#pragma once

#include "compaction.h"
#include "perm.h"
#include "protocol_config.h"
#include "sharing.h"
#include "shuffle.h"

namespace sort {

std::vector<Row> sort_iteration(ProtocolConfig &conf, std::vector<Row> &input_share) {
    Shuffle shuffle(conf, 1);

    /* Set wire to input*/
    if (input_share.size() != conf.n_rows) throw std::invalid_argument("Input share has wrong size.");

    /* Generate secret shared compaction perm */
    std::vector<Row> perm_y = compaction::get_compaction(conf, input_share);
    /* Shuffle it */
    shuffle.set_input(perm_y);
    shuffle.run();

    /* Reveal shuffled perm and generate inverse (due to difference of how Permutation works)*/
    std::vector<Row> revealed = shuffle.reveal();
    Permutation perm_shuffled(revealed);
    perm_shuffled = perm_shuffled.inverse();

    /* Shuffle input using the previous order */
    shuffle.set_input(input_share);
    shuffle.repeat();
    std::vector<Row> input_shuffled = shuffle.get_output();

    std::vector<Row> sorted_share = perm_shuffled(input_shuffled);
    return sorted_share;
}
};  // namespace sort