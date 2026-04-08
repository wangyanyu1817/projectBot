#include "scanner.h"
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

std::vector<FileInfo> scan_directory(const std::string& dir, const std::vector<std::string>& extensions) {
    std::vector<FileInfo> result;
    for (auto& entry : fs::recursive_directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        // Skip .wiki and hidden directories
        auto rel = fs::relative(entry.path(), dir);
        bool skip = false;
        for (auto& part : rel) {
            auto s = part.string();
            if (!s.empty() && s[0] == '.') { skip = true; break; }
        }
        if (skip) continue;
        auto ext = entry.path().extension().string();
        if (std::find(extensions.begin(), extensions.end(), ext) == extensions.end()) continue;
        FileInfo info;
        info.path = entry.path().string();
        info.relative_path = rel.string();
        info.last_modified = fs::last_write_time(entry).time_since_epoch().count();
        result.push_back(info);
    }
    return result;
}
