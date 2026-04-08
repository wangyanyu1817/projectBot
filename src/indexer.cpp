#include "indexer.h"
#include "utils.h"
#include "wiki_generator.h"
#include <algorithm>
#include <filesystem>
#include <map>

void build_index(const std::string& project_root, const std::vector<FileInfo>& files) {
    std::string index_content = "# 项目知识总览\n\n";
    index_content += "最后更新: " + std::to_string(time(nullptr)) + "\n\n";
    index_content += "## 模块索引\n\n";
    index_content += "| 文件 | 一句话摘要 | 关键词 |\n";
    index_content += "|------|------------|--------|\n";

    for (auto& file : files) {
        // 只对代码文件生成摘要（这里简单取前1000个字符作为输入）
        std::string content = read_file(file.path);
        if (content.empty()) continue;
        // 截断过长内容
        if (content.size() > 2000) content = content.substr(0, 2000);
        std::string summary = generate_module_summary(content);
        // 简单关键词提取：取文件名中的单词
        std::string keywords = file.relative_path;
        std::replace(keywords.begin(), keywords.end(), '/', ' ');
        std::replace(keywords.begin(), keywords.end(), '.', ' ');
        index_content += "| `" + file.relative_path + "` | " + summary + " | " + keywords + " |\n";
    }

    std::string index_path = project_root + "/.wiki/INDEX.md";
    std::filesystem::create_directories(project_root + "/.wiki");
    write_file(index_path, index_content);
}
