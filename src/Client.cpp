#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include "FileHandler.h"

#define PORT 8080

int main(int argc, char const *argv[]) {
    if (argc != 2) {
        std::cout << "Usage: ./client <filepath>" << std::endl;
        return 1;
    }

    std::string filepath = argv[1];
    int sock = 0;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cout << "\n Socket creation error \n";
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        std::cout << "\nInvalid address/ Address not supported \n";
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cout << "\nConnection Failed \n";
        return -1;
    }

    std::cout << "Reading file: " << filepath << "..." << std::endl;
    try {
        std::vector<unsigned char> fileData = FileHandler::readFile(filepath);
        
        std::cout << "Sending " << fileData.size() << " bytes to server..." << std::endl;
        send(sock, fileData.data(), fileData.size(), 0);
        std::cout << "File Sent Successfully!" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    close(sock);
    return 0;
}