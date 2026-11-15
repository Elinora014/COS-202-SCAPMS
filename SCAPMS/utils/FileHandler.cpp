#include "FileHandler.h"

#include <fstream>
#include <sstream>

#include "Exceptions.h"

namespace scapms {
namespace utils {

std::vector<std::string> FileHandler::readLines(const std::string& path)
{
    std::ifstream in(path);
    if (!in.is_open())
        throw FileOperationError("Cannot open file for reading: " + path);

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line)) {
        // Skip empty lines and comment/header lines starting with '#'.
        if (line.empty() || line[0] == '#')
            continue;
        lines.push_back(line);
    }
    if (in.bad())
        throw FileOperationError("Error while reading file: " + path);
    return lines;
}

void FileHandler::writeLines(const std::string& path,
                             const std::vector<std::string>& lines)
{
    std::ofstream out(path, std::ios::trunc);
    if (!out.is_open())
        throw FileOperationError("Cannot open file for writing: " + path);

    for (const std::string& line : lines) {
        out << line << '\n';
        if (!out.good())
            throw FileOperationError("Error while writing file: " + path);
    }
}

bool FileHandler::exists(const std::string& path)
{
    std::ifstream f(path);
    return f.good();
}

} // namespace utils
} // namespace scapms
