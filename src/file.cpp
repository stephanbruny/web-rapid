#include "file.h"

std::string File::read(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        // Throw an exception or handle the error as needed
        throw std::runtime_error("Failed to open file: " + filename);
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());

    file.close();
    return content;
}

void File::write(const std::string &filename, const std::string &content) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        // Throw an exception or handle the error as needed
        throw std::runtime_error("Failed to open file: " + filename);
    }

    file << content;

    file.close();
}
