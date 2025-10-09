#include "JarReader.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <vector>
#include <zlib.h>

// ZIP file format structures
#pragma pack(push, 1)
struct ZipLocalFileHeader {
    uint32_t signature;           // 0x04034b50
    uint16_t versionNeeded;
    uint16_t flags;
    uint16_t compressionMethod;
    uint16_t lastModTime;
    uint16_t lastModDate;
    uint32_t crc32;
    uint32_t compressedSize;
    uint32_t uncompressedSize;
    uint16_t filenameLength;
    uint16_t extraFieldLength;
};

struct ZipCentralDirEntry {
    uint32_t signature;           // 0x02014b50
    uint16_t versionMadeBy;
    uint16_t versionNeeded;
    uint16_t flags;
    uint16_t compressionMethod;
    uint16_t lastModTime;
    uint16_t lastModDate;
    uint32_t crc32;
    uint32_t compressedSize;
    uint32_t uncompressedSize;
    uint16_t filenameLength;
    uint16_t extraFieldLength;
    uint16_t commentLength;
    uint16_t diskNumber;
    uint16_t internalAttr;
    uint32_t externalAttr;
    uint32_t localHeaderOffset;
};

struct ZipEndOfCentralDir {
    uint32_t signature;           // 0x06054b50
    uint16_t diskNumber;
    uint16_t centralDirDisk;
    uint16_t numEntriesThisDisk;
    uint16_t numEntries;
    uint32_t centralDirSize;
    uint32_t centralDirOffset;
    uint16_t commentLength;
};
#pragma pack(pop)

std::optional<std::string> JarReader::extractFabricModJson(const std::string& jarPath) {
    return readFileFromZip(jarPath, "fabric.mod.json");
}

bool JarReader::isValidJar(const std::string& jarPath) {
    std::ifstream file(jarPath, std::ios::binary);
    if (!file.is_open()) return false;

    uint32_t signature;
    file.read(reinterpret_cast<char*>(&signature), sizeof(signature));

    // Check for ZIP signature (0x04034b50 = PK\x03\x04)
    return signature == 0x04034b50;
}

std::optional<std::string> JarReader::readFileFromZip(const std::string& zipPath,
                                                       const std::string& filename) {
    std::ifstream file(zipPath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Could not open JAR file: " << zipPath << std::endl;
        return std::nullopt;
    }

    // Find end of central directory
    file.seekg(-static_cast<int>(sizeof(ZipEndOfCentralDir)), std::ios::end);
    ZipEndOfCentralDir endDir{};
    file.read(reinterpret_cast<char*>(&endDir), sizeof(endDir));

    if (endDir.signature != 0x06054b50) {
        std::cerr << "Invalid ZIP file: " << zipPath << std::endl;
        return std::nullopt;
    }

    // Read central directory entries
    file.seekg(endDir.centralDirOffset, std::ios::beg);

    for (uint16_t i = 0; i < endDir.numEntries; ++i) {
        ZipCentralDirEntry entry{};
        file.read(reinterpret_cast<char*>(&entry), sizeof(entry));

        if (entry.signature != 0x02014b50) {
            std::cerr << "Invalid central directory entry" << std::endl;
            return std::nullopt;
        }

        std::vector<char> filenameBuf(entry.filenameLength + 1, 0);
        file.read(filenameBuf.data(), entry.filenameLength);
        std::string entryName(filenameBuf.data());

        file.seekg(entry.extraFieldLength + entry.commentLength, std::ios::cur);

        if (entryName == filename) {
            auto currentPos = file.tellg();
            file.seekg(entry.localHeaderOffset, std::ios::beg);

            ZipLocalFileHeader localHeader{};
            file.read(reinterpret_cast<char*>(&localHeader), sizeof(localHeader));

            file.seekg(localHeader.filenameLength + localHeader.extraFieldLength, std::ios::cur);

            std::vector<char> compressedData(entry.compressedSize);
            file.read(compressedData.data(), entry.compressedSize);

            if (entry.compressionMethod == 0) {
                return std::string(compressedData.begin(), compressedData.end());
            }

            if (entry.compressionMethod == 8) {
                std::vector<char> uncompressedData(entry.uncompressedSize);

                z_stream stream = {};
                stream.next_in = reinterpret_cast<Bytef*>(compressedData.data());
                stream.avail_in = entry.compressedSize;
                stream.next_out = reinterpret_cast<Bytef*>(uncompressedData.data());
                stream.avail_out = entry.uncompressedSize;

                if (inflateInit2(&stream, -MAX_WBITS) != Z_OK) {
                    std::cerr << "Failed to initialize decompression" << std::endl;
                    return std::nullopt;
                }

                int result = inflate(&stream, Z_FINISH);
                inflateEnd(&stream);

                if (result != Z_STREAM_END) {
                    std::cerr << "Decompression failed" << std::endl;
                    return std::nullopt;
                }

                return std::string(uncompressedData.begin(), uncompressedData.end());
            }

            file.seekg(currentPos, std::ios::beg);
        }
    }

    return std::nullopt;
}