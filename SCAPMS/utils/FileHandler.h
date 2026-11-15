#ifndef SCAPMS_UTILS_FILEHANDLER_H
#define SCAPMS_UTILS_FILEHANDLER_H

#include <string>
#include <vector>

namespace scapms {
namespace utils {

/**
 * FileHandler - thin text-file abstraction used for persistence.
 *
 * The clinic data is stored as plain CSV text files:
 *   line 1  : header (ignored on load)
 *   lines 2+ : one serialized record per line
 *
 * Every operation throws utils::FileOperationError on failure so the GUI
 * can show a meaningful message instead of crashing.
 */
class FileHandler {
public:
    // Reads all lines of a text file (without the line breaks).
    static std::vector<std::string> readLines(const std::string& path);

    // Writes the given lines, one per line, overwriting the file.
    static void writeLines(const std::string& path,
                           const std::vector<std::string>& lines);

    static bool exists(const std::string& path);
};

} // namespace utils
} // namespace scapms

#endif // SCAPMS_UTILS_FILEHANDLER_H
