#ifndef FABRICBINARYSEARCH_JARREADER_H
#define FABRICBINARYSEARCH_JARREADER_H

#include <string>
#include <optional>

class JarReader {
public:
    static std::optional<std::string> extractFabricModJson(const std::string& jarPath);

    static bool isValidJar(const std::string& jarPath);

private:
    static std::optional<std::string> readFileFromZip(const std::string& zipPath,
                                                      const std::string& filename);
};

#endif // FABRICBINARYSEARCH_JARREADER_H