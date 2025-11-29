#include "FileHandler.h"
#include <stdexcept>

std::vector<unsigned char> FileHandler::readFile(const std::string& filepath) {
    // Open the file with 'std::ios::binary' and 'std::ios::ate' (at the end)
    // 'ate' lets us check file size immediately.
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);

    if (!file) {
        throw std::runtime_error("Error: Could not open file " + filepath);
    }

    // Determine file size
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg); // Go back to start

    // Resize vector to fit data
    std::vector<unsigned char> buffer(size);

    // Read data
    if (file.read((char*)buffer.data(), size)) {
        return buffer;
    } else {
        throw std::runtime_error("Error: Failed to read file data.");
    }
}

bool FileHandler::writeFile(const std::string& filepath, const std::vector<unsigned char>& data) {
    std::ofstream file(filepath, std::ios::binary);
    
    if (!file) {
        std::cerr << "Error: Could not create file " + filepath << std::endl;
        return false;
    }

    // Write the raw bytes
    file.write((const char*)data.data(), data.size());
    return file.good();
}