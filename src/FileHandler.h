#ifndef FILEHANDLER_H
#define FILEHANDLER_H

#include <string>
#include <vector>
#include <fstream>
#include <iostream>

class FileHandler {
public:
    // Read a file from disk into a byte vector (unsigned char)
    // We use 'unsigned char' because regular 'char' can be negative, which breaks binary data.
    static std::vector<unsigned char> readFile(const std::string& filepath);

    // Write a byte vector back to disk
    static bool writeFile(const std::string& filepath, const std::vector<unsigned char>& data);
};

#endif