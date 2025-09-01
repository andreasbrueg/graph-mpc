#pragma once

#include <emp-tool/emp-tool.h>
#include <socket/TCPSSLClient.h>

#include <future>

#include "../../src/utils/graph.h"
#include "../../src/utils/types.h"

class InputClient {
   public:
    struct Packet {
        size_t start;
        size_t end;
        std::vector<Ring> entries;
    };

    InputClient(int id) : id(id), connected(false) {
        auto PRINT_LOG = [](const std::string &strLogMsg) { std::cout << strLogMsg << std::endl; };

        m_pSSLTCPClient.reset(new CTCPSSLClient(PRINT_LOG));
    };

    ~InputClient() { m_pSSLTCPClient->Disconnect(); };

    void connect(std::string ip, int port) {
        m_pSSLTCPClient->Disconnect();
        auto ConnectTask = [&]() -> bool {
            bool bRet = m_pSSLTCPClient->Connect(ip, std::to_string(port));
            return bRet;
        };
        std::packaged_task<bool(void)> packageConnect{ConnectTask};
        std::future<bool> futConnect = packageConnect.get_future();
        std::thread ConnectThread{std::move(packageConnect)};

        futConnect.get();
        ConnectThread.join();

        connected = true;
    }

    void send_graph(Graph &g, size_t start) {
        Packet pkt;
        pkt.start = start;
        pkt.end = start + g.size() * 4;
        pkt.entries = g.serialize();

        if (connected) {
            send_packet(pkt);
        } else {
            std::cout << "Could not send packet since client is not connected to server." << std::endl;
        }
    }

   private:
    void send_packet(Packet &pkt) {
        std::array<size_t, 2> header{pkt.start, pkt.end};
        m_pSSLTCPClient->Send(reinterpret_cast<const char *>(header.data()), sizeof(header));

        size_t n_entries = pkt.end - pkt.start;
        if (n_entries > 0) {
            m_pSSLTCPClient->Send(reinterpret_cast<const char *>(pkt.entries.data()), n_entries * sizeof(Ring));
        }
    }

   private:
    int id;
    bool connected;
    std::unique_ptr<CTCPSSLClient> m_pSSLTCPClient;
};
