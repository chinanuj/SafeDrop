#include <iostream>
#include "FileHandler.h"
#include "CryptoHandler.h"
#include <thread>
#include <chrono>
#include <cstdio> 


void secureDelete(std::string filepath, int delaySeconds) {
    std::cout << "[SecureDelete] Timer started for: " << filepath << std::endl;
    
    // Wait (Simulate the file being available for download)
    std::this_thread::sleep_for(std::chrono::seconds(delaySeconds));
    
    // Delete the file
    if (std::remove(filepath.c_str()) == 0) {
        std::cout << "[SecureDelete] SUCCESS: File " << filepath << " has been wiped from disk." << std::endl;
    } else {
        std::cerr << "[SecureDelete] ERROR: Could not delete " << filepath << std::endl;
    }
}


int main() {
    std::string inputFile = "me2.jpeg";
    std::string encryptedFile = "me2.enc";
    std::string decryptedFile = "me2_restored.jpeg";

    try {
        // Generate Key and IV (In a real app, Key comes from User Password)
        std::cout << "Generating 256-bit AES Key and IV..." << std::endl;
        auto key = CryptoHandler::generateRandomBytes(CryptoHandler::KEY_SIZE);
        auto iv = CryptoHandler::generateRandomBytes(CryptoHandler::IV_SIZE);

        // Read Original File
        std::cout << "Reading file: " << inputFile << "..." << std::endl;
        auto fileData = FileHandler::readFile(inputFile);
        std::cout << "Original Size: " << fileData.size() << " bytes." << std::endl;

        // Encrypt
        std::cout << "Encrypting data..." << std::endl;
        auto encryptedData = CryptoHandler::encrypt(fileData, key, iv);
        FileHandler::writeFile(encryptedFile, encryptedData);
        std::cout << "Saved encrypted file to: " << encryptedFile << std::endl;

        // Decrypt (Simulate the receiver)
        std::cout << "Decrypting data..." << std::endl;
        auto loadedEncryptedData = FileHandler::readFile(encryptedFile);
        auto decryptedData = CryptoHandler::decrypt(loadedEncryptedData, key, iv);
        
        FileHandler::writeFile(decryptedFile, decryptedData);
        std::cout << "Restored file to: " << decryptedFile << std::endl;
        std::cout << "SUCCESS! Open me2_restored.jpeg to verify." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "CRITICAL ERROR: " << e.what() << std::endl;
        return 1;
    }

    std::thread cleaner(secureDelete, decryptedFile, 5);
    cleaner.join(); 

    return 0;
}