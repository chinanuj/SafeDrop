cmake_minimum_required(VERSION 3.10)
project(SafeDrop VERSION 1.0)

# 1. Set C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 2. Find OpenSSL (Required for Encryption)
find_package(OpenSSL REQUIRED)

# 3. Include Header Files
include_directories(src)
include_directories(${OPENSSL_INCLUDE_DIR})

# --- EXECUTABLE 1: The Core Server ---
add_executable(server 
    src/Server.cpp 
    src/FileHandler.cpp 
    src/CryptoHandler.cpp
)
# Link OpenSSL to Server
target_link_libraries(server OpenSSL::SSL OpenSSL::Crypto)

# --- EXECUTABLE 2: The Client (CLI Tool) ---
add_executable(client 
    src/Client.cpp 
    src/FileHandler.cpp
)
# Client doesn't do encryption (it just sends raw bytes), so no OpenSSL link needed here.
# If you add encryption to the client later, link OpenSSL here too.

# --- EXECUTABLE 3: Local Test (main.cpp) ---
add_executable(safedrop_test 
    src/main.cpp 
    src/FileHandler.cpp 
    src/CryptoHandler.cpp
)
target_link_libraries(safedrop_test OpenSSL::SSL OpenSSL::Crypto)
