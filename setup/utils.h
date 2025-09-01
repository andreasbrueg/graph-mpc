#pragma once

#include <boost/program_options.hpp>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "../src/io/netmp.h"
#include "../src/utils/graph.h"
#include "../src/utils/random_generators.h"
#include "comm.h"
#include "input-sharing/input_server.h"
#include "stats.h"

namespace bpo = boost::program_options;

namespace setup {

void print_vec(std::vector<Ring> &vec);

bpo::options_description programOptions();

bpo::options_description clientProgramOptions();

bpo::variables_map parseOptions(bpo::options_description &cmdline, bpo::options_description &prog_opts, int argc, char *argv[]);

void setupClient(const bpo::variables_map &opts, int &id, size_t &start_idx, std::string &ip_0, std::string &ip_1, int &port_0, int &port_1,
                 std::string &input_file, size_t &n_bits);

void setupExecution(const bpo::variables_map &opts, size_t &pid, size_t &nP, size_t &nC, size_t &repeat, size_t &threads, size_t &nodes,
                    io::NetworkConfig &net_config, uint64_t *seeds_h, uint64_t *seeds_l, bool &save_output, std::string &save_file, bool &save_to_disk,
                    std::string &input_file);

void run_test(const bpo::variables_map &opts,
              std::function<void(Party id, RandomGenerators &rngs, io::NetworkConfig &net_conf, size_t n, std::string input_file, Graph &g)> func);

void run_benchmark(const bpo::variables_map &opts,
                   std::function<void(Party id, RandomGenerators &rngs, io::NetworkConfig &net_conf, size_t n, size_t repeat, size_t n_vertices,
                                      bool save_output, std::string save_file, bool save_to_disk, std::string input_file, Graph &g)>
                       func);
}  // namespace setup