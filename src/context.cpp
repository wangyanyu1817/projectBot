#include "context.h"
#include "utils.h"
#include <filesystem>
#include <sstream>
#include <algorithm>
#include <vector>

std::string get_relevant_context(const std::string& query) {
    // Search .wiki/details for files whose content contains query keywords
    std::string wiki_dir = ".wiki/details";
    if (!std::filesystem::exists(wiki_dir)) return "";

    // Split query into words
    std::vector<std::string> keywords;
    std::istringstream iss(query);
    std::string word;
    while (iss >> word) {
        std::transform(word.begin(), word.end(), word.begin(), ::tolower);
        keywords.push_back(word);
    }

    std::string context;
    for (auto& entry : std::filesystem::recursive_directory_iterator(wiki_dir)) {
        if (!entry.is_regular_file()) continue;
        std::string content = read_file(entry.path().string());
        std::string lower = content;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        for (auto& kw : keywords) {
            if (lower.find(kw) != std::string::npos) {
                context += "### " + entry.path().string() + "\n" + content + "\n\n";
                break;
            }
        }
    }
    return context;
}
