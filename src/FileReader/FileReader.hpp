#ifndef A5A3E861_4D78_42AE_A1F0_A6B4525B8353
#define A5A3E861_4D78_42AE_A1F0_A6B4525B8353

#include <string>
#include <fstream>
#include <iostream>
#include <filesystem>
namespace fs = std::filesystem;

/*
    @brief  Function used to read whole file into std::string
    @param  path - location of shader code
    @return string with shader code
 */
inline std::string read_file(fs::path path) {
    fs::path fullPath {fs::current_path()};
    fullPath += path;
//    std::cout<<fullPath;
    // Open the stream to 'lock' the file.
    std::ifstream f(fullPath, std::ios::in | std::ios::binary);

    // Obtain the size of the file.
    const auto sz = fs::file_size(fullPath);

    // Create a buffer.
    std::string result(sz, '\0');

    // Read the whole file into the buffer.
    f.read(result.data(), sz);

    return result;
}

#endif /* A5A3E861_4D78_42AE_A1F0_A6B4525B8353 */
