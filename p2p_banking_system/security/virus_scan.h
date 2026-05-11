#ifndef VIRUS_SCAN_H
#define VIRUS_SCAN_H

#include <string>
#include <vector>
#include <set>

class VirusScanner
{
    std::set<std::string> virusSignatures;
    std::set<std::string> heuristicPatterns;
    std::mutex scannerMutex;

    void loadDefaultSignatures();
    std::string computeSignature(const std::string &data, size_t offset, size_t length);

public:
    VirusScanner();

    void addSignature(const std::string &signature);
    void addSignatures(const std::vector<std::string> &signatures);
    void removeSignature(const std::string &signature);
    bool scanData(const std::string &data, std::vector<std::string> &detectedViruses);
    bool scanFile(const std::string &filename, std::vector<std::string> &detectedViruses);
    bool isSignatureKnown(const std::string &signature) const;
    void clearSignatures();
    size_t getSignatureCount() const;
};

class FileSanitizer
{
    std::vector<std::string> blockedExtensions;
    std::vector<std::string> allowedExtensions;
    size_t maxFileSize;

public:
    FileSanitizer();

    bool isSafeExtension(const std::string &filename) const;
    bool isSafeSize(size_t fileSize) const;
    std::string sanitizeFilename(const std::string &filename);
    bool containsMaliciousPatterns(const std::string &data);
};

#endif
