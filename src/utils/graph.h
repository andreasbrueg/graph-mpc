#pragma once

#include <omp.h>

#include <cassert>
#include <vector>

#include "permutation.h"
#include "types.h"

struct Graph {
    std::vector<Ring> src;
    std::vector<Ring> dst;
    std::vector<Ring> isV;
    std::vector<Ring> payload;
    size_t size = 0;
    size_t n_vertices = 0;

    Graph() = default;
    Graph(size_t n_rows) {
        src = std::vector<Ring>(n_rows);
        dst = std::vector<Ring>(n_rows);
        isV = std::vector<Ring>(n_rows);
        payload = std::vector<Ring>(n_rows);
        size = n_rows;
    }

    void add_list_entry(Ring _src, Ring _dst, Ring _isV, Ring _payload) {
        src.push_back(_src);
        dst.push_back(_dst);
        isV.push_back(_isV);
        payload.push_back(_payload);
        size++;

        if (_isV) n_vertices++;
    }

    void add_list_entry(Ring _src, Ring _dst, Ring _isV) { add_list_entry(_src, _dst, _isV, 0); }

    void print() {
        for (size_t i = 0; i < size; ++i) {
            std::cout << src[i] << " " << dst[i] << " " << isV[i] << " " << payload[i] << std::endl;
        }
        std::cout << std::endl;
    }
};

struct SecretSharedGraph {
    std::vector<Ring> src;
    std::vector<Ring> dst;
    std::vector<Ring> isV;
    std::vector<Ring> payload;

    std::vector<std::vector<Ring>> src_bits;
    std::vector<std::vector<Ring>> dst_bits;

    std::vector<std::vector<Ring>> src_order_bits;
    std::vector<std::vector<Ring>> dst_order_bits;
    std::vector<Ring> inverted_isV;

    Permutation src_order;
    Permutation dst_order;
    Permutation vtx_order;

    size_t size = 0;
    size_t n_vertices = 0;

    SecretSharedGraph(Party id, std::vector<std::vector<Ring>> &src_bits, std::vector<std::vector<Ring>> &dst_bits, std::vector<Ring> &isV_bits)
        : src_bits(src_bits), dst_bits(dst_bits), isV(isV_bits) {
        size_t n_bits = src_bits.size();
        size_t n = isV_bits.size();

        assert(dst_bits.size() == n_bits);

        for (size_t i = 0; i < n_bits; ++i) {
            assert(src_bits[i].size() == n);
            assert(dst_bits[i].size() == n);
        }

        size = n;

        /* Generate vector containing { 1-isV, src[0], src[1], ..., src[n_bits - 1] } */
        src_order_bits.resize(src_bits.size() + 1);

        inverted_isV.resize(isV.size());
        for (size_t i = 0; i < inverted_isV.size(); ++i) {
            inverted_isV[i] = -isV[i];
            if (id == P0) inverted_isV[i] += 1;
        }

        std::copy(src_bits.begin(), src_bits.end(), src_order_bits.begin() + 1);
        src_order_bits[0] = inverted_isV;

        /* Generate vector containing { isV, dst[0], dst[1], ..., dst[n_bits - 1] } */
        dst_order_bits.resize(dst_bits.size() + 1);

        std::copy(dst_bits.begin(), dst_bits.end(), dst_order_bits.begin() + 1);
        dst_order_bits[0] = isV;
    }
};
