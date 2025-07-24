// MIT License
//
// Copyright (c) 2018 Xiao Wang (wangxiao@gmail.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// The following code has been adopted from
// https://github.com/emp-toolkit/emp-agmpc. It has been modified to define the
// class within a namespace and add additional methods (sendRelative,
// recvRelative).

#pragma once

#include <errno.h>   // errno
#include <fcntl.h>   // open
#include <unistd.h>  // write, close, lseek

#include <cassert>
#include <cstdio>
#include <cstring>  // strerror
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "../utils/types.h"
#include "emp-tool/emp-tool.h"
#include "tls_net_io_channel.h"

namespace io {
using namespace emp;

struct NetworkConfig {
    Party id;
    int n_parties;
    int port;
    char **IP;
    std::string certificate_path;
    std::string private_key_path;
    std::string trusted_cert_path;
    bool localhost;
    size_t BLOCK_SIZE;
};

class NetIOMP {
   public:
    int party;
    int nP;
    std::mutex mtx;
    std::condition_variable cv;
    bool connection_established;
    bool save_to_disk;

    NetIOMP(NetworkConfig &conf, bool save_to_disk = false)
        : conf(conf),
          ios(conf.n_parties),
          ios2(conf.n_parties),
          party(conf.id),
          nP(conf.n_parties),
          sent(conf.n_parties, false),
          BLOCK_SIZE(conf.BLOCK_SIZE),
          connection_established(false),
          save_to_disk(save_to_disk),
          n_send(conf.n_parties, 0),
          send_buffer(conf.n_parties),
          recv_buffer(conf.n_parties) {
        /* Remove all old files */
        for (int i = 0; i < nP; ++i) {
            std::string filename = std::to_string(party) + "_" + std::to_string((Party)i) + ".bin";
            std::filesystem::remove(filename);
        }

        init_thread = std::thread(&NetIOMP::init_network, this);
    }

    ~NetIOMP() {
        if (init_thread.joinable()) init_thread.join();
    }

    void init_network() {
        for (int i = 0; i < nP; ++i) {
            for (int j = i + 1; j < nP; ++j) {
                if (i == party) {
                    usleep(1000);
                    if (conf.localhost) {
                        ios[j] = std::make_unique<TLSNetIO>("127.0.0.1", conf.port + 2 * (i * nP + j), conf.trusted_cert_path, true);
                    } else {
                        ios[j] = std::make_unique<TLSNetIO>(conf.IP[j], conf.port + 2 * (i * nP + j), conf.trusted_cert_path, true);
                    }
                    ios[j]->set_nodelay();

                    usleep(1000);
                    if (conf.localhost) {
                        ios2[j] = std::make_unique<TLSNetIO>(conf.port + 2 * (i * nP + j) + 1, conf.certificate_path, conf.private_key_path, true);
                    } else {
                        ios2[j] = std::make_unique<TLSNetIO>(conf.port + 2 * (i * nP + j) + 1, conf.certificate_path, conf.private_key_path, true);
                    }
                    ios2[j]->set_nodelay();
                } else if (j == party) {
                    usleep(1000);
                    if (conf.localhost) {
                        ios[i] = std::make_unique<TLSNetIO>(conf.port + 2 * (i * nP + j), conf.certificate_path, conf.private_key_path, true);
                    } else {
                        ios[i] = std::make_unique<TLSNetIO>(conf.port + 2 * (i * nP + j), conf.certificate_path, conf.private_key_path, true);
                    }
                    ios[i]->set_nodelay();

                    usleep(1000);
                    if (conf.localhost) {
                        ios2[i] = std::make_unique<TLSNetIO>("127.0.0.1", conf.port + 2 * (i * nP + j) + 1, conf.trusted_cert_path, true);
                    } else {
                        ios2[i] = std::make_unique<TLSNetIO>(conf.IP[i], conf.port + 2 * (i * nP + j) + 1, conf.trusted_cert_path, true);
                    }
                    ios2[i]->set_nodelay();
                }
            }
        }
        {
            std::lock_guard<std::mutex> lock(mtx);
            connection_established = true;
        }
        cv.notify_all();
    }

    int64_t count() {
        int64_t res = 0;
        for (int i = 0; i < nP; ++i)
            if (i != party) {
                res += ios[i]->counter;
                res += ios2[i]->counter;
            }
        return res;
    }

    void resetStats() {
        for (int i = 0; i < nP; ++i) {
            if (i != party) {
                ios[i]->counter = 0;
                ios2[i]->counter = 0;
            }
        }
    }

    void add_send(Party dst, std::vector<Ring> &data) {
        /* Count the total number of elements */
        n_send[dst] += data.size();

        if (party == D && save_to_disk) {
            write_binary(std::to_string(party) + "_" + std::to_string(dst) + ".bin", data);
        } else {
            auto &buf = send_buffer[dst];
            buf.insert(buf.end(), data.begin(), data.end());
        }
    }

    void send_all() {
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this] { return connection_established; });
        }

        for (int i = 0; i < nP; ++i) {
            Party dst = (Party)i;
            size_t n_elems = n_send[(Party)i];
            if (dst != party && n_elems > 0) {
                /* Send number of elements to receive */
                send(dst, &n_elems, sizeof(size_t));

                /* Sending actual data */
                if (party == D && save_to_disk) {
                    std::string filename = std::to_string(party) + "_" + std::to_string((Party)i) + ".bin";
                    std::ifstream infile(filename, std::ios::binary);
                    if (!infile) {
                        throw std::runtime_error("Failed to open file: " + filename);
                    }
                    std::vector<Ring> chunk(CHUNK_SIZE);
                    std::cout << "Sending values that are loaded from disk in chunks." << std::endl;
                    while (infile) {
                        infile.read(reinterpret_cast<char *>(chunk.data()), CHUNK_SIZE * sizeof(Ring));
                        std::streamsize n_read = infile.gcount() / sizeof(Ring);
                        if (n_read > 0) {
                            send_vec((Party)i, n_read, chunk);
                        }
                    }
                    infile.close();
                } else {
                    send_vec((Party)i, send_buffer[i].size(), send_buffer[i]);
                }
                /* Clear n_send */
                n_send[(Party)i] = 0;
            }
        }
        std::cout << "Sent all." << std::endl;
    }

    void send(Party dst, const void *data, size_t len) {
        if (dst != party) {
            if (party < dst)
                ios[dst]->send_data(data, len);
            else
                ios2[dst]->send_data(data, len);
            sent[dst] = true;
        }
#ifdef __clang__
        flush(dst);
#endif
    }

    void send_vec(Party dst, size_t n_elems, std::vector<Ring> &data) {
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this] { return connection_established; });
        }
        size_t n_msgs = n_elems / BLOCK_SIZE;
        size_t last_msg_size = n_elems % BLOCK_SIZE;
        for (size_t i = 0; i < n_msgs; i++) {
            std::vector<Ring> data_send_i;
            data_send_i.resize(BLOCK_SIZE);
            for (size_t j = 0; j < BLOCK_SIZE; j++) {
                data_send_i[j] = data[i * BLOCK_SIZE + j];
            }
            send(dst, data_send_i.data(), sizeof(Ring) * BLOCK_SIZE);
        }

        std::vector<Ring> data_send_last;
        data_send_last.resize(last_msg_size);
        for (size_t j = 0; j < last_msg_size; j++) {
            data_send_last[j] = data[n_msgs * BLOCK_SIZE + j];
        }
        send(dst, data_send_last.data(), sizeof(Ring) * last_msg_size);
    }

    void recv(Party src, void *data, size_t len) {
        if (src != party) {
            if (sent[src]) flush(src);
            if (src < party)
                ios[src]->recv_data(data, len);
            else
                ios2[src]->recv_data(data, len);
        }
    }

    void recv_buffered(Party src) {
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this] { return connection_established; });
        }

        size_t n_elems;
        recv(src, &n_elems, sizeof(size_t));
        std::cout << "Receiving " << n_elems << " from " << src << " (buffered)..." << std::endl;

        recv_vec(src, n_elems, recv_buffer[src]);

        // recv_buffer[src].resize(n_elems);
        // size_t n_msgs = n_elems / BLOCK_SIZE;
        // size_t last_msg_size = n_elems % BLOCK_SIZE;
        // for (size_t i = 0; i < n_msgs; i++) {
        // std::vector<Ring> data_recv_i(BLOCK_SIZE);
        // recv(src, data_recv_i.data(), sizeof(Ring) * BLOCK_SIZE);
        // for (size_t j = 0; j < BLOCK_SIZE; j++) {
        // recv_buffer[src][i * BLOCK_SIZE + j] = data_recv_i[j];
        //}
        //}

        ///* Receive last elements */
        // std::vector<Ring> data_recv_last(last_msg_size);
        // recv(src, data_recv_last.data(), sizeof(Ring) * last_msg_size);
        // for (size_t j = 0; j < last_msg_size; j++) {
        // recv_buffer[src][n_msgs * BLOCK_SIZE + j] = data_recv_last[j];
        //}
    }

    void recv_vec(Party src, size_t n_elems, std::vector<Ring> &buffer) {
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this] { return connection_established; });
        }

        buffer.resize(n_elems);
        size_t n_msgs = n_elems / BLOCK_SIZE;
        size_t last_msg_size = n_elems % BLOCK_SIZE;
        for (size_t i = 0; i < n_msgs; i++) {
            std::vector<Ring> data_recv_i(BLOCK_SIZE);
            recv(src, data_recv_i.data(), sizeof(Ring) * BLOCK_SIZE);
            for (size_t j = 0; j < BLOCK_SIZE; j++) {
                buffer[i * BLOCK_SIZE + j] = data_recv_i[j];
            }
        }

        /* Receive last elements */
        std::vector<Ring> data_recv_last(last_msg_size);
        recv(src, data_recv_last.data(), sizeof(Ring) * last_msg_size);
        for (size_t j = 0; j < last_msg_size; j++) {
            buffer[n_msgs * BLOCK_SIZE + j] = data_recv_last[j];
        }
    }

    std::vector<Ring> read(Party src, size_t n_elems) {
        if (src == party) {
            throw std::logic_error("Cannot receive data from yourself.");
        }

        auto &buffer = recv_buffer[src];
        assert(buffer.size() >= n_elems);

        std::vector<Ring> chunk;
        chunk.reserve(n_elems);
        std::move(buffer.begin(), buffer.begin() + n_elems, std::back_inserter(chunk));
        buffer.erase(buffer.begin(), buffer.begin() + n_elems);

        return chunk;
    }

    TLSNetIO *get(size_t idx, bool b = false) {
        if (b)
            return ios[idx].get();
        else
            return ios2[idx].get();
    }

    TLSNetIO *getSendChannel(size_t idx) {
        if ((size_t)party < idx) {
            return ios[idx].get();
        }

        return ios2[idx].get();
    }

    TLSNetIO *getRecvChannel(size_t idx) {
        if (idx < (size_t)party) {
            return ios[idx].get();
        }

        return ios2[idx].get();
    }

    void flush(int idx = -1) {
        if (idx == -1) {
            for (int i = 0; i < nP; ++i) {
                if (i != party) {
                    ios[i]->flush();
                    ios2[i]->flush();
                }
            }
        } else {
            if (party < idx) {
                ios[idx]->flush();
            } else {
                ios2[idx]->flush();
            }
        }
    }

    void sync() {
        for (int i = 0; i < nP; ++i) {
            for (int j = 0; j < nP; ++j) {
                if (i < j) {
                    if (i == party) {
                        ios[j]->sync();
                        ios2[j]->sync();
                    } else if (j == party) {
                        ios[i]->sync();
                        ios2[i]->sync();
                    }
                }
            }
        }
    }

    void write_to_file(const char *filename, Ring val) {
        int fd = open(filename, O_WRONLY | O_CREAT, 0666);
        if (fd < 0) {
            throw new std::runtime_error("Error opening file.");
        }

        if (lseek(fd, 0, SEEK_END) == (off_t)-1) {
            close(fd);
            throw new std::runtime_error("Could not seek end of file.");
        }

        ssize_t bytes_written = write(fd, &val, sizeof(Ring));
        close(fd);
    }

    /**
     * Writes the data to a file.
     */
    void write_binary(const std::string &filename, const std::vector<Ring> &data) {
        std::ofstream out(filename, std::ios::binary | std::ios::app);
        if (!out) throw std::runtime_error("Failed to open file for writing.");

        out.write(reinterpret_cast<const char *>(data.data()), data.size() * sizeof(Ring));
    }

    /**
     * Reads n_elements Ring-Elements from file filename.
     */
    std::vector<Ring> read_binary_file(const std::string &filename) {
        std::ifstream in(filename, std::ios::binary);
        if (!in) throw std::runtime_error("Failed to open file.");

        auto file_size = std::filesystem::file_size(filename);
        if (file_size % sizeof(Ring) != 0) throw std::runtime_error("File size is not a multiple of element size.");

        size_t n_elems = file_size / sizeof(Ring);
        std::vector<Ring> data(n_elems);

        in.read(reinterpret_cast<char *>(data.data()), file_size);

        std::filesystem::remove(filename);
        return data;
    }

   private:
    NetworkConfig conf;
    std::vector<std::unique_ptr<TLSNetIO>> ios;
    std::vector<std::unique_ptr<TLSNetIO>> ios2;
    std::vector<bool> sent;
    size_t BLOCK_SIZE;
    const size_t CHUNK_SIZE = 16384;

    std::thread init_thread;

    std::vector<std::vector<Ring>> send_buffer;
    std::vector<std::vector<Ring>> recv_buffer;
    std::vector<size_t> n_send;
};

};  // namespace io
