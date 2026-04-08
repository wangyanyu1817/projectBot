#include "context.h"
#include "wiki_generator.h"
#include "utils.h"
#include <filesystem>
#include <sstream>
#include <algorithm>
#include <vector>
#include <chrono>

// Token budget: use 60% of context window
static constexpr int CTX_BUDGET = 8192 * 0.6;

struct WikiEntry {
    std::string path;
    std::string content;
    double score;
};

static std::vector<std::string> split_words(const std::string& text) {
    std::vector<std::string> words;
    std::istringstream iss(text);
    std::string w;
    while (iss >> w) {
        std::transform(w.begin(), w.end(), w.begin(), ::tolower);
        words.push_back(w);
    }
    return words;
}

// Score = keyword hit count + recency bonus (newer files score higher)
static double score_entry(const std::string& content, const std::string& path,
                           const std::vector<std::string>& keywords) {
    std::string lower = content;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    double score = 0;
    for (auto& kw : keywords) {
        size_t pos = 0;
        while ((pos = lower.find(kw, pos)) != std::string::npos) {
            score += 1.0;
            pos += kw.size();
        }
    }
    if (score == 0) return 0;

    // Recency bonus: normalize mtime to [0, 1] range relative to now
    auto mtime = std::filesystem::last_write_time(path);
    auto age = std::filesystem::file_time_type::clock::now() - mtime;
    double age_hours = std::chrono::duration_cast<std::chrono::hours>(age).count();
    double recency = 1.0 / (1.0 + age_hours / 24.0); // decay over days
    score += recency;

    return score;
}

std::string get_relevant_context(const std::string& query) {
    std::string wiki_dir = ".wiki/details";
    if (!std::filesystem::exists(wiki_dir)) return "";

    auto keywords = split_words(query);
    if (keywords.empty()) return "";

    // Collect and score all wiki entries
    std::vector<WikiEntry> entries;
    for (auto& entry : std::filesystem::recursive_directory_iterator(wiki_dir)) {
        if (!entry.is_regular_file()) continue;
        std::string content = read_file(entry.path().string());
        if (content.empty()) continue;
        double s = score_entry(content, entry.path().string(), keywords);
        if (s > 0)
            entries.push_back({entry.path().string(), content, s});
    }

    // Sort by score descending
    std::sort(entries.begin(), entries.end(),
              [](const WikiEntry& a, const WikiEntry& b) { return a.score > b.score; });

    // Fill context up to token budget
    std::string context;
    int used_tokens = 0;
    for (auto& e : entries) {
        std::string block = "### " + e.path + "\n" + e.content + "\n\n";
        int block_tokens = count_tokens(block);
        if (used_tokens + block_tokens > CTX_BUDGET) break;
        context += block;
        used_tokens += block_tokens;
    }
    return context;
}
