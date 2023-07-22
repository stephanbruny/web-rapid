#ifndef GENESISQUEST_FILE_H
#define GENESISQUEST_FILE_H

#include <fstream>
#include <string>

namespace File {
    std::string read(const std::string& filename);
    void write(const std::string& filename, const std::string& content);
}

#endif //GENESISQUEST_FILE_H
