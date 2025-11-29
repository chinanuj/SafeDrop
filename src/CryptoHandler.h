#ifndef CRYPTOHANDLER_H
#define CRYPTOHANDLER_H

#include <vector>
#include <string>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <stdexcept>

class CryptoHandler {
public:
    // AES-256 requires a 32-byte key and a 16-byte IV
    static const int KEY_SIZE = 32;
    static const int IV_SIZE = 16;

    // Helper to generate random bytes (for Key and IV)
    static std::vector<unsigned char> generateRandomBytes(int size);

    // Encrypts plaintext -> ciphertext
    static std::vector<unsigned char> encrypt(const std::vector<unsigned char>& plaintext, 
                                              const std::vector<unsigned char>& key, 
                                              const std::vector<unsigned char>& iv);

    // Decrypts ciphertext -> plaintext
    static std::vector<unsigned char> decrypt(const std::vector<unsigned char>& ciphertext, 
                                              const std::vector<unsigned char>& key, 
                                              const std::vector<unsigned char>& iv);
};

#endif