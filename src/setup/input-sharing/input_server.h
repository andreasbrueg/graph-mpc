#pragma once

#include <socket/TCPSSLServer.h>

#include <cassert>
#include <future>

#include "../../utils/graph.h"

class InputServer {
   public:
    struct Packet {
        size_t start;
        size_t end;
        std::vector<Ring> entries;
    };

    InputServer(std::string password, std::string port, size_t n_clients) : password(password), clients(n_clients) {
        auto PRINT_LOG = [](const std::string &strLogMsg) { std::cout << strLogMsg << std::endl; };
        m_pSSLTCPServer.reset(new CTCPSSLServer(PRINT_LOG, port));  // creates an SSL/TLS TCP server

        std::string SSL_CERT_FILE = "certs/socket_cert.pem";
        std::string SSL_KEY_FILE = "certs/socket_key.pem";

        m_pSSLTCPServer->SetSSLCertFile(SSL_CERT_FILE);
        m_pSSLTCPServer->SetSSLKeyFile(SSL_KEY_FILE);
    };

    ~InputServer() = default;

    void connect_clients() {
        bool success = true;
        do {
            for (auto &client : clients) {
                bool connected = m_pSSLTCPServer->Listen(client);
                success = success && connected;
            }
        } while (!success);
        std::cout << "Clients connected." << std::endl;
    }

    Graph recv_graph() {
        /* Receive passwords */
        size_t password_size;
        std::string recvd_pwd;
        for (auto &client : clients) {
            m_pSSLTCPServer->Receive(client, reinterpret_cast<char *>(&password_size), sizeof(size_t));
            std::cout << "Received password size: " << password_size << std::endl;

            recvd_pwd.resize(password_size);
            m_pSSLTCPServer->Receive(client, recvd_pwd.data(), password_size);
            std::cout << "Received password: " << recvd_pwd << std::endl;
            if (!check_password(recvd_pwd)) {
                throw std::runtime_error("A client failed to authenticate.");
            }
        }

        /* Receive n_vertices */
        size_t nodes_total = 0;
        for (auto &client : clients) {
            size_t nodes;
            m_pSSLTCPServer->Receive(client, reinterpret_cast<char *>(&nodes), sizeof(size_t));
            nodes_total += nodes;
        }
        std::cout << "nodes received: " << nodes_total << std::endl;
        bits = std::ceil(std::log2(nodes_total + 2));

        /* Receive contents */
        std::vector<Packet> pkts;
        try {
            pkts = recv_packets();
        } catch (const std::exception &e) {
            std::cout << "Could not receive all packets: " << e.what() << std::endl;
            return {};
        }
        std::cout << "Packets received." << std::endl;

        size_t total_size = 0;
        for (auto &pkt : pkts) {
            size_t pkt_size = pkt.end - pkt.start;
            assert(pkt.start < pkt.end);
            assert(pkt.entries.size() == (4 + 2 * bits) * pkt_size);
            total_size += pkt_size;
        }

        std::vector<Ring> src(total_size);
        std::vector<Ring> dst(total_size);
        std::vector<Ring> isV(total_size);
        std::vector<Ring> data(total_size);
        std::vector<std::vector<Ring>> src_bits(bits, std::vector<Ring>(total_size));
        std::vector<std::vector<Ring>> dst_bits(bits, std::vector<Ring>(total_size));

        for (auto &pkt : pkts) {
            size_t pkt_size = pkt.end - pkt.start;
            size_t pkt_idx = 0;

            for (size_t i = pkt.start; i < pkt.start + pkt_size; ++i) {
                src[i] = pkt.entries[pkt_idx++];
                dst[i] = pkt.entries[pkt_idx++];
                isV[i] = pkt.entries[pkt_idx++];
                data[i] = pkt.entries[pkt_idx++];
            }

            for (size_t i = 0; i < bits; ++i) {
                for (size_t j = pkt.start; j < pkt.start + pkt_size; ++j) {
                    src_bits[i][j] = pkt.entries[pkt_idx++];
                }
            }

            for (size_t i = 0; i < bits; ++i) {
                for (size_t j = pkt.start; j < pkt.start + pkt_size; ++j) {
                    dst_bits[i][j] = pkt.entries[pkt_idx++];
                }
            }
        }

        return {src, dst, isV, data, src_bits, dst_bits, nodes_total};
    }

    void recv_nodes(size_t &nodes_total) {
        nodes_total = 0;
        for (auto &client : clients) {
            size_t nodes;
            m_pSSLTCPServer->Receive(client, reinterpret_cast<char *>(&nodes), sizeof(size_t));
            nodes_total += nodes;
        }
        std::cout << "Received nodes: " << nodes_total << std::endl;
    }

    void recv_size(size_t &size_total) {
        size_total = 0;
        for (auto &client : clients) {
            size_t size;
            m_pSSLTCPServer->Receive(client, reinterpret_cast<char *>(&size), sizeof(size_t));
            size_total += size;
        }
        std::cout << "Received nodes: " << size_total << std::endl;
    }

    void send_result(std::vector<Ring> &data, size_t client_idx) { m_pSSLTCPServer->Send(clients[client_idx], reinterpret_cast<char *>(&data)); }

   private:
    std::vector<Packet> recv_packets() {
        std::vector<Packet> packets;
        for (auto &client : clients) {
            auto packet = recv_packet(client);
            packets.push_back(packet);
        }
        return packets;
    }

    Packet recv_packet(ASecureSocket::SSLSocket &client) {
        Packet pkt;

        std::array<size_t, 2> header{};
        auto n_bytes_recvd = m_pSSLTCPServer->Receive(client, reinterpret_cast<char *>(header.data()), sizeof(header));

        if (n_bytes_recvd != sizeof(header)) {
            throw std::runtime_error("Failed to receive packet header");
        }

        pkt.start = header[0];
        pkt.end = header[1];

        if (pkt.end < pkt.start) {
            throw std::runtime_error("Invalid packet: end < start");
        }

        size_t n_entries = pkt.end - pkt.start;
        size_t elems_to_recv = 4 * n_entries + 2 * n_entries * bits;
        size_t bytes_to_recv = elems_to_recv * sizeof(Ring);
        pkt.entries.resize(elems_to_recv);

        if (n_entries == 0) {
            return pkt;  // nothing else to receive
        }

        n_bytes_recvd = m_pSSLTCPServer->Receive(client, reinterpret_cast<char *>(pkt.entries.data()), bytes_to_recv);

        if (n_bytes_recvd != static_cast<int>(bytes_to_recv)) {
            throw std::runtime_error("Failed to receive all ring elements.");
        }

        return pkt;
    }

    /* Not safe against side-channel attacks */
    bool check_password(std::string &recvd_pwd) {
        if (password == recvd_pwd) return true;
        return false;
    }

    std::string password;
    size_t bits = 0;
    std::vector<ASecureSocket::SSLSocket> clients;
    std::unique_ptr<CTCPSSLServer> m_pSSLTCPServer;
};
