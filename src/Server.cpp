#include <iostream>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <thread>
#include <vector>
#include <iomanip>
#include <fstream>
#include <chrono>
#include <arpa/inet.h>
#include "FileHandler.h"
#include "CryptoHandler.h"

#define PORT 8080
#define BUFFER_SIZE 4096

std::string generateFileID() {
    const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::string id = "";
    for (int i = 0; i < 8; i++) id += charset[rand() % 36];
    return id;
}

std::vector<unsigned char> hexToBytes(const std::string& hex) {
    std::vector<unsigned char> bytes;
    for (unsigned int i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        unsigned char byte = (unsigned char)strtol(byteString.c_str(), NULL, 16);
        bytes.push_back(byte);
    }
    return bytes;
}

void handleClient(int client_socket) {
    char command_buf[1]; 
    if (read(client_socket, command_buf, 1) <= 0) { close(client_socket); return; }
    
    if (command_buf[0] == 'U') { 
        // UPLOAD: Expect 0xBEEF + Limit + Expiry
        unsigned short magic;
        unsigned int params[2];
        
        read(client_socket, &magic, 2);
        
        // CHECK MAGIC BYTES
        if (ntohs(magic) != 0xBEEF) {
            std::cerr << "[Core] Protocol Error: Expected 0xBEEF" << std::endl;
            close(client_socket);
            return;
        }

        read(client_socket, params, sizeof(params));
        int limit = ntohl(params[0]);
        int seconds = ntohl(params[1]);

        if (limit <= 0) limit = 1;
        if (seconds <= 0) seconds = 86400;

        std::cout << "[Core] Upload: Limit=" << limit << ", Expiry=" << seconds << "s" << std::endl;

        std::vector<unsigned char> buffer;
        unsigned char tempBuf[BUFFER_SIZE];
        int n;
        while ((n = read(client_socket, tempBuf, BUFFER_SIZE)) > 0) {
            buffer.insert(buffer.end(), tempBuf, tempBuf + n);
        }

        auto key = CryptoHandler::generateRandomBytes(CryptoHandler::KEY_SIZE);
        auto iv = CryptoHandler::generateRandomBytes(CryptoHandler::IV_SIZE);
        auto encryptedData = CryptoHandler::encrypt(buffer, key, iv);

        std::vector<unsigned char> fileContent;
        fileContent.insert(fileContent.end(), iv.begin(), iv.end());
        fileContent.insert(fileContent.end(), encryptedData.begin(), encryptedData.end());

        std::string fileID = generateFileID();
        FileHandler::writeFile(fileID + ".enc", fileContent);

        long long expiry_ts = std::time(0) + seconds;
        std::ofstream metaFile(fileID + ".meta");
        metaFile << limit << " " << 0 << " " << expiry_ts; 
        metaFile.close();

        std::stringstream keyHex;
        for(auto c : key) keyHex << std::hex << std::setw(2) << std::setfill('0') << (int)c;
        std::string response = fileID + "|" + keyHex.str();
        send(client_socket, response.c_str(), response.length(), 0);

    } else if (command_buf[0] == 'D') {
        // DOWNLOAD
        char headerBuf[72]; 
        int total = 0;
        while(total < 72) {
            int r = read(client_socket, headerBuf + total, 72 - total);
            if (r <= 0) { close(client_socket); return; }
            total += r;
        }

        std::string fileID(headerBuf, 8);
        std::string keyHex(headerBuf + 8, 64);
        std::string encF = fileID + ".enc";
        std::string metaF = fileID + ".meta";

        try {
            std::ifstream metaIn(metaF);
            if(!metaIn.is_open()) throw std::runtime_error("Missing");
            
            int max_dl, curr_dl;
            long long expiry;
            metaIn >> max_dl >> curr_dl >> expiry;
            metaIn.close();

            if (curr_dl >= max_dl || std::time(0) > expiry) {
                std::remove(encF.c_str());
                std::remove(metaF.c_str());
                throw std::runtime_error("Burned");
            }

            auto raw = FileHandler::readFile(encF);
            if (raw.size() < CryptoHandler::IV_SIZE) throw std::runtime_error("Corrupt");

            std::vector<unsigned char> iv(raw.begin(), raw.begin() + CryptoHandler::IV_SIZE);
            std::vector<unsigned char> cipher(raw.begin() + CryptoHandler::IV_SIZE, raw.end());
            std::vector<unsigned char> key = hexToBytes(keyHex);
            
            auto plain = CryptoHandler::decrypt(cipher, key, iv);
            send(client_socket, plain.data(), plain.size(), 0);
            
            shutdown(client_socket, SHUT_WR);
            std::this_thread::sleep_for(std::chrono::seconds(1));

            curr_dl++;
            if (curr_dl >= max_dl) {
                std::remove(encF.c_str());
                std::remove(metaF.c_str());
                std::cout << "[Core] " << fileID << " burned." << std::endl;
            } else {
                std::ofstream metaOut(metaF);
                metaOut << max_dl << " " << curr_dl << " " << expiry;
                metaOut.close();
            }

        } catch (...) {
            std::string err = "ERROR";
            send(client_socket, err.c_str(), err.length(), 0);
        }
    }
    close(client_socket);
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) return 0;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 3);

    std::cout << ">>> SafeDrop Core V3 (Magic Bytes Enabled)..." << std::endl;

    while (true) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) >= 0) {
            std::thread(handleClient, new_socket).detach();
        }
    }
    return 0;
}