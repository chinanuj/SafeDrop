#include "CryptoHandler.h"
#include <iostream>

std::vector<unsigned char> CryptoHandler::generateRandomBytes(int size) {
    std::vector<unsigned char> buffer(size);
    if (!RAND_bytes(buffer.data(), size)) {
        throw std::runtime_error("Error generating random bytes");
    }
    return buffer;
}

std::vector<unsigned char> CryptoHandler::encrypt(const std::vector<unsigned char>& plaintext, 
                                                  const std::vector<unsigned char>& key, 
                                                  const std::vector<unsigned char>& iv) {
    
    // Create Cipher Context
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw std::runtime_error("Failed to create cipher context");

    std::vector<unsigned char> ciphertext(plaintext.size() + IV_SIZE); // Output buffer (allow space for padding)
    int len = 0;
    int ciphertext_len = 0;

    // Initialize Encryption Operation (AES-256-CBC)
    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key.data(), iv.data())) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Encryption Init failed");
    }

    // Provide bytes to be encrypted
    if (1 != EVP_EncryptUpdate(ctx, ciphertext.data(), &len, plaintext.data(), plaintext.size())) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Encryption Update failed");
    }
    ciphertext_len = len;

    // Finalize (Handle padding)
    if (1 != EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len)) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Encryption Final failed");
    }
    ciphertext_len += len;

    // Clean up
    EVP_CIPHER_CTX_free(ctx);
    
    // Resize vector to actual encrypted size
    ciphertext.resize(ciphertext_len);
    return ciphertext;
}

std::vector<unsigned char> CryptoHandler::decrypt(const std::vector<unsigned char>& ciphertext, 
                                                  const std::vector<unsigned char>& key, 
                                                  const std::vector<unsigned char>& iv) {
    
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw std::runtime_error("Failed to create cipher context");

    std::vector<unsigned char> plaintext(ciphertext.size());
    int len = 0;
    int plaintext_len = 0;

    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key.data(), iv.data())) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Decryption Init failed");
    }

    if (1 != EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext.data(), ciphertext.size())) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Decryption Update failed");
    }
    plaintext_len = len;

    if (1 != EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len)) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Decryption Final failed (Wrong Key?)");
    }
    plaintext_len += len;

    EVP_CIPHER_CTX_free(ctx);
    plaintext.resize(plaintext_len);
    return plaintext;
}