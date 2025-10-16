#include "mp_protocol.h"

void MPProtocol::add_function(std::shared_ptr<Function> func) { f_queue.push_back(std::move(func)); }

std::vector<Ring> MPProtocol::add_input() {
    std::vector<Ring> output(size);
    std::iota(output.begin(), output.end(), num_wires);
    num_wires += size;

    add_function(std::make_shared<Input>(f_queue.size(), &conf, &input_to_val, output));
    return output;
}

std::vector<Ring> MPProtocol::propagate_1(std::vector<Ring> &input) {
    std::vector<Ring> output(size);
    std::iota(output.begin(), output.end(), num_wires);
    num_wires += size;

    add_function(std::make_shared<Propagate_1>(f_queue.size(), &conf, input, output));
    return output;
}

std::vector<Ring> MPProtocol::propagate_2(std::vector<Ring> &input1, std::vector<Ring> &input2) {
    std::vector<Ring> output(size);
    std::iota(output.begin(), output.end(), num_wires);
    num_wires += size;

    add_function(std::make_shared<Propagate_2>(f_queue.size(), &conf, input1, input2, output));
    return output;
}

std::vector<Ring> MPProtocol::gather_1(std::vector<Ring> &input) {
    std::vector<Ring> output(size);
    std::iota(output.begin(), output.end(), num_wires);
    num_wires += size;

    add_function(std::make_shared<Gather_1>(f_queue.size(), &conf, input, output));
    return output;
}

std::vector<Ring> MPProtocol::gather_2(std::vector<Ring> &input) {
    std::vector<Ring> output(size);
    std::iota(output.begin(), output.end(), num_wires);
    num_wires += size;

    add_function(std::make_shared<Gather_2>(f_queue.size(), &conf, input, output));
    return output;
}

std::vector<Ring> MPProtocol::shuffle(std::vector<Ring> &input, size_t shuffle_idx) {
    std::vector<Ring> output(size);
    std::iota(output.begin(), output.end(), num_wires);
    num_wires += size;

    add_function(
        std::make_shared<Shuffle>(f_queue.size(), &conf, &ctx.preproc, &ctx.shuffle_vals, &shuffles, input, output, recv_shuffle, &preproc_disk, shuffle_idx));
    return output;
}

std::vector<Ring> MPProtocol::unshuffle(std::vector<Ring> &input, size_t shuffle_idx) {
    std::vector<Ring> output(size);
    std::iota(output.begin(), output.end(), num_wires);
    num_wires += size;

    add_function(std::make_shared<Unshuffle>(f_queue.size(), &conf, &ctx.preproc, &ctx.shuffle_vals, &shuffles, input, output, recv_shuffle, &preproc_disk,
                                             shuffle_idx));
    return output;
}

std::vector<Ring> MPProtocol::merged_shuffle(std::vector<Ring> &input, size_t pi_idx, size_t omega_idx, size_t shuffle_idx) {
    std::vector<Ring> output(size);
    std::iota(output.begin(), output.end(), num_wires);
    num_wires += size;

    add_function(std::make_shared<MergedShuffle>(f_queue.size(), &conf, &ctx.preproc, &ctx.shuffle_vals, &shuffles, input, output, recv_shuffle, pi_idx,
                                                 omega_idx, shuffle_idx, &preproc_disk));
    return output;
}

std::vector<Ring> MPProtocol::compaction(std::vector<Ring> &input) {
    std::vector<Ring> output(size);
    std::iota(output.begin(), output.end(), num_wires);
    num_wires += size;

    add_function(std::make_shared<Compaction>(f_queue.size(), &conf, &ctx.preproc, &ctx.mult_vals, input, output, recv_mul, &preproc_disk, &triples_disk));
    return output;
}

std::vector<Ring> MPProtocol::reveal(std::vector<Ring> &input) {
    std::vector<Ring> output(size);
    std::iota(output.begin(), output.end(), num_wires);
    num_wires += size;

    add_function(std::make_shared<Reveal>(f_queue.size(), &conf, &ctx.reveal_vals, input, output));
    return output;
}

std::vector<Ring> MPProtocol::permute(std::vector<Ring> &input, std::vector<Ring> &perm, bool inverse) {
    std::vector<Ring> output(size);
    std::iota(output.begin(), output.end(), num_wires);
    num_wires += size;

    add_function(std::make_shared<Permute>(f_queue.size(), &conf, input, perm, output, inverse));
    return output;
}

std::vector<Ring> MPProtocol::equals_zero(std::vector<Ring> &input, size_t size, size_t layer) {
    std::vector<Ring> output(size);
    std::iota(output.begin(), output.end(), num_wires);
    num_wires += size;

    add_function(std::make_shared<EQZ>(f_queue.size(), &conf, &ctx.preproc, &ctx.and_vals, input, output, recv_mul, size, layer, &preproc_disk, &triples_disk));
    return output;
}

std::vector<Ring> MPProtocol::bit2A(std::vector<Ring> &input, size_t size) {
    std::vector<Ring> output(size);
    std::iota(output.begin(), output.end(), num_wires);
    num_wires += size;

    add_function(std::make_shared<Bit2A>(f_queue.size(), &conf, &ctx.preproc, &ctx.mult_vals, input, output, recv_mul, size, &preproc_disk, &triples_disk));
    return output;
}

/* TODO: Connect output in Insert */
std::vector<Ring> MPProtocol::insert(std::vector<Ring> input) {
    std::vector<Ring> output(size);
    std::iota(output.begin(), output.end(), num_wires);
    num_wires += size;

    add_function(std::make_shared<Insert>(f_queue.size(), &conf, input, output));
    return output;
}

std::vector<Ring> MPProtocol::deduplication_sub(std::vector<Ring> &vec_p) {
    std::vector<Ring> vec_dupl(size - 1);
    std::iota(vec_dupl.begin(), vec_dupl.end(), num_wires);
    num_wires += (size - 1);

    add_function(std::make_shared<DeduplicationSub>(f_queue.size(), &conf, vec_p, vec_dupl));
    return vec_dupl;
}

std::vector<Ring> MPProtocol::mul(std::vector<Ring> &x, std::vector<Ring> &y, bool binary) {
    std::vector<Ring> output(size);
    std::iota(output.begin(), output.end(), num_wires);
    num_wires += size;

    if (binary) {
        add_function(std::make_shared<Mul>(f_queue.size(), &conf, &ctx.preproc, &ctx.and_vals, x, y, output, recv_mul, binary, &preproc_disk, &triples_disk));
    } else {
        add_function(std::make_shared<Mul>(f_queue.size(), &conf, &ctx.preproc, &ctx.mult_vals, x, y, output, recv_mul, binary, &preproc_disk, &triples_disk));
    }
    return output;
}

std::vector<Ring> MPProtocol::mul(std::vector<Ring> &x, std::vector<Ring> &y, size_t size, bool binary) {
    std::vector<Ring> output(size);
    std::iota(output.begin(), output.end(), num_wires);
    num_wires += size;

    if (binary) {
        add_function(
            std::make_shared<Mul>(f_queue.size(), &conf, &ctx.preproc, &ctx.and_vals, x, y, output, recv_mul, binary, size, &preproc_disk, &triples_disk));
    } else {
        add_function(
            std::make_shared<Mul>(f_queue.size(), &conf, &ctx.preproc, &ctx.mult_vals, x, y, output, recv_mul, binary, size, &preproc_disk, &triples_disk));
    }
    return output;
}

/* TODO: Connect output in PushBack */
std::vector<Ring> MPProtocol::push_back(std::vector<std::vector<Ring>> &keys, std::vector<Ring> &vec) {
    std::vector<Ring> output(size);
    std::iota(output.begin(), output.end(), num_wires);
    num_wires += size;

    add_function(std::make_shared<PushBack>(f_queue.size(), &conf, &keys, vec));
    return output;
}

std::vector<Ring> MPProtocol::flip(std::vector<Ring> &input) {
    std::vector<Ring> output(size);
    std::iota(output.begin(), output.end(), num_wires);
    num_wires += size;

    add_function(std::make_shared<Flip>(f_queue.size(), &conf, input, output));
    return output;
}

std::vector<Ring> MPProtocol::add(std::vector<Ring> &input1, std::vector<Ring> &input2) {
    std::vector<Ring> output(size);
    std::iota(output.begin(), output.end(), num_wires);
    num_wires += size;

    add_function(std::make_shared<Add>(f_queue.size(), &conf, input1, input2, output));
    return output;
}

std::vector<Ring> MPProtocol::add_const(std::vector<Ring> &data, Ring val) {
    std::vector<Ring> output(size);
    std::iota(output.begin(), output.end(), num_wires);
    num_wires += size;

    add_function(std::make_shared<AddConst>(f_queue.size(), &conf, data, val, output));
    return output;
}

std::vector<Ring> MPProtocol::construct_payload(std::vector<std::vector<Ring>> &payloads) {
    std::vector<Ring> output(size);
    std::iota(output.begin(), output.end(), num_wires);
    num_wires += size;

    add_function(std::make_shared<ConstructPayload>(f_queue.size(), &conf, &payloads, output));
    return output;
}

void MPProtocol::add_deduplication() {
    ctx.dst_order = add_sort(g.dst_order_bits, bits + 1);
    auto deduplication_perm = ctx.dst_order;

    for (size_t i = 0; i < bits; ++i) {  // Was g.src_bits().size()
        deduplication_perm = add_sort_iteration(deduplication_perm, g.src_bits[i]);
    }

    deduplication_perm = shuffle(deduplication_perm, shuffle_idx);
    deduplication_perm = reveal(deduplication_perm);

    auto deduplication_src = shuffle(g.src, shuffle_idx);
    auto deduplication_dst = shuffle(g.dst, shuffle_idx);

    deduplication_perm = permute(deduplication_src, deduplication_src);
    deduplication_perm = permute(deduplication_dst, deduplication_dst);

    auto deduplication_src_dupl = deduplication_sub(deduplication_src);
    auto deduplication_dst_dupl = deduplication_sub(deduplication_dst);

    deduplication_src_dupl = equals_zero(deduplication_src_dupl, size - 1, 0);
    deduplication_dst_dupl = equals_zero(deduplication_dst_dupl, size - 1, 0);
    deduplication_src_dupl = equals_zero(deduplication_src_dupl, size - 1, 1);
    deduplication_dst_dupl = equals_zero(deduplication_dst_dupl, size - 1, 1);
    deduplication_src_dupl = equals_zero(deduplication_src_dupl, size - 1, 2);
    deduplication_dst_dupl = equals_zero(deduplication_dst_dupl, size - 1, 2);
    deduplication_src_dupl = equals_zero(deduplication_src_dupl, size - 1, 3);
    deduplication_dst_dupl = equals_zero(deduplication_dst_dupl, size - 1, 3);
    deduplication_src_dupl = equals_zero(deduplication_src_dupl, size - 1, 4);
    deduplication_dst_dupl = equals_zero(deduplication_dst_dupl, size - 1, 4);

    auto deduplication_duplicates = mul(deduplication_src_dupl, deduplication_dst_dupl, size - 1, true);
    deduplication_duplicates = bit2A(deduplication_duplicates, size - 1);
    deduplication_duplicates = insert(deduplication_duplicates);

    deduplication_duplicates = permute(deduplication_duplicates, deduplication_perm, true);
    deduplication_duplicates = unshuffle(deduplication_duplicates, shuffle_idx);
    push_back(g.src_order_bits, deduplication_duplicates);
    push_back(g.dst_order_bits, deduplication_duplicates);
    shuffle_idx++;
}

void MPProtocol::add_clip() {
    for (size_t i = 0; i < w.mp_data_parallel.size(); ++i) {
        std::vector<Ring> &wire = w.data;
        if (w.mp_data_parallel.size() == 0) {
            wire = equals_zero(wire, size, 0);
            wire = equals_zero(wire, size, 1);
            wire = equals_zero(wire, size, 2);
            wire = equals_zero(wire, size, 3);
            wire = equals_zero(wire, size, 4);
            wire = bit2A(wire, size);
            wire = flip(wire);
        } else {
            w.mp_data_parallel[i] = equals_zero(w.mp_data_parallel[i], size, 0);
            w.mp_data_parallel[i] = equals_zero(w.mp_data_parallel[i], size, 1);
            w.mp_data_parallel[i] = equals_zero(w.mp_data_parallel[i], size, 2);
            w.mp_data_parallel[i] = equals_zero(w.mp_data_parallel[i], size, 3);
            w.mp_data_parallel[i] = equals_zero(w.mp_data_parallel[i], size, 4);
            w.mp_data_parallel[i] = bit2A(w.mp_data_parallel[i], size);
            w.mp_data_parallel[i] = flip(w.mp_data_parallel[i]);
        }
    }
}
