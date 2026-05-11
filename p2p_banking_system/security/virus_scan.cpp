#include "virus_scan.h"

VirusScanner::VirusScanner()
{
    loadDefaultSignatures();
}

void VirusScanner::loadDefaultSignatures()
{
    virusSignatures.insert("X5O!P%@AP[4\\PZX54(P^)7CC)7}$EICAR-STANDARD-ANTIVIRUS-TEST-FILE!$H+H*");
    virusSignatures.insert(sha1("malware_pattern_1"));
    virusSignatures.insert(sha1("malware_pattern_2"));
    virusSignatures.insert(sha1("trojan_pattern"));
    virusSignatures.insert(sha1("ransomware_pattern"));

    heuristicPatterns.insert("eval(");
    heuristicPatterns.insert("exec(");
    heuristicPatterns.insert("system(");
    heuristicPatterns.insert("popen(");
    heuristicPatterns.insert("shell_exec(");
    heuristicPatterns.insert("base64_decode(");
    heuristicPatterns.insert("gzinflate(");
}

std::string VirusScanner::computeSignature(const std::string &data, size_t offset, size_t length)
{
    if (offset + length > data.size())
        return "";
    return sha1(data.substr(offset, length));
}

void VirusScanner::addSignature(const std::string &signature)
{
    std::lock_guard<std::mutex> lock(scannerMutex);
    virusSignatures.insert(signature);
}

void VirusScanner::addSignatures(const std::vector<std::string> &signatures)
{
    std::lock_guard<std::mutex> lock(scannerMutex);
    for (const auto &sig : signatures)
    {
        virusSignatures.insert(sig);
    }
}

void VirusScanner::removeSignature(const std::string &signature)
{
    std::lock_guard<std::mutex> lock(scannerMutex);
    virusSignatures.erase(signature);
}

bool VirusScanner::scanData(const std::string &data, std::vector<std::string> &detectedViruses)
{
    std::lock_guard<std::mutex> lock(scannerMutex);
    detectedViruses.clear();

    for (const auto &pattern : virusSignatures)
    {
        if (data.find(pattern) != std::string::npos)
        {
            detectedViruses.push_back(pattern);
        }
    }

    for (size_t i = 0; i + 32 <= data.size(); i++)
    {
        std::string sig = computeSignature(data, i, 32);
        if (virusSignatures.find(sig) != virusSignatures.end())
        {
            detectedViruses.push_back(sig);
        }
    }

    for (const auto &pattern : heuristicPatterns)
    {
        if (data.find(pattern) != std::string::npos)
        {
            detectedViruses.push_back("heuristic:" + pattern);
        }
    }

    return detectedViruses.empty();
}

bool VirusScanner::scanFile(const std::string &filename, std::vector<std::string> &detectedViruses)
{
    return true;
}

bool VirusScanner::isSignatureKnown(const std::string &signature) const
{
    std::lock_guard<std::mutex> lock(scannerMutex);
    return virusSignatures.find(signature) != virusSignatures.end();
}

void VirusScanner::clearSignatures()
{
    std::lock_guard<std::mutex> lock(scannerMutex);
    virusSignatures.clear();
    loadDefaultSignatures();
}

size_t VirusScanner::getSignatureCount() const
{
    std::lock_guard<std::mutex> lock(scannerMutex);
    return virusSignatures.size();
}

FileSanitizer::FileSanitizer() : maxFileSize(10485760)
{
    blockedExtensions = {".exe", ".dll", ".so", ".dylib", ".sh", ".bat", ".cmd", ".ps1", ".vbs", ".js", ".jar"};
    allowedExtensions = {".txt", ".json", ".xml", ".csv", ".log", ".dat", ".bin"};
}

bool FileSanitizer::isSafeExtension(const std::string &filename) const
{
    for (const auto &ext : blockedExtensions)
    {
        if (filename.size() >= ext.size() &&
            filename.substr(filename.size() - ext.size()) == ext)
        {
            return false;
        }
    }
    return true;
}

bool FileSanitizer::isSafeSize(size_t fileSize) const
{
    return fileSize <= maxFileSize;
}

std::string FileSanitizer::sanitizeFilename(const std::string &filename)
{
    std::string result;
    for (char c : filename)
    {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '.' || c == '-' || c == '_')
        {
            result.push_back(c);
        }
        else
        {
            result.push_back('_');
        }
    }
    return result;
}

bool FileSanitizer::containsMaliciousPatterns(const std::string &data)
{
    std::vector<std::string> patterns = {
        "../", "..\\", "~", "eval", "exec", "system", "popen",
        "base64_decode", "gzinflate", "str_rot13", "assert",
        "create_function", "call_user_func", "preg_replace"};

    for (const auto &pattern : patterns)
    {
        if (data.find(pattern) != std::string::npos)
        {
            return true;
        }
    }
    return false;
}
