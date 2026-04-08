#pragma once
#include <string>
#include <vector>

struct FileInfo {
    std::string path;
    std::string relative_path;
    time_t last_modified;
};

std::vector<FileInfo> scan_directory(const std::string& dir, const std::vector<std::string>& extensions);

