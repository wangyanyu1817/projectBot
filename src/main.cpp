#include "scanner.h"
#include "indexer.h"
#include "wiki_generator.h"
#include "context.h"
#include "utils.h"
#include <iostream>
#include <vector>
#include <filesystem>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: project_bot <project_dir> [--model <path>] [--query <question>] [--verbose]\n";
        return 1;
    }

    std::string project_dir = argv[1];
    std::string model_path = "models/gemma-4-e4b.Q4_K_M.gguf";

    // 解析可选参数
    std::string query;
    bool verbose = false;
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--model" && i + 1 < argc) model_path = argv[++i];
        else if (arg == "--query" && i + 1 < argc) query = argv[++i];
        else if (arg == "--verbose") verbose = true;
    }

    std::vector<std::string> extensions = {".py", ".md", ".cpp", ".h", ".txt"};
    auto files = scan_directory(project_dir, extensions);
    std::cout << "Found " << files.size() << " files\n";

    init_llama(model_path);

    std::string wiki_root = project_dir + "/.wiki/details";
    std::filesystem::create_directories(wiki_root);
    for (auto& f : files) {
        std::string out_path = wiki_root + "/" + f.relative_path + ".md";
        if (std::filesystem::exists(out_path) && std::filesystem::last_write_time(out_path) >= std::filesystem::last_write_time(f.path))
            continue;
        std::string content = read_file(f.path);
        if (content.empty()) continue;
        std::string wiki = generate_file_wiki(content, f.relative_path);
        std::filesystem::create_directories(std::filesystem::path(out_path).parent_path());
        write_file(out_path, wiki);
        std::cout << "Generated: " << out_path << std::endl;
    }

    build_index(project_dir, files);

    if (!query.empty()) {
        std::string context = get_relevant_context(query, project_dir);
        if (verbose) {
            std::cerr << "\n[verbose] context length: " << context.size() << " chars\n";
            std::cerr << "[verbose] context preview:\n" << context.substr(0, 500) << "\n...\n";
        }
        std::string final_prompt = "<start_of_turn>user\n你是一个代码助手。根据以下项目文档，详细回答问题，包括步骤说明和代码示例。\n\n文档：\n" + context + "\n问题：" + query + "<end_of_turn>\n<start_of_turn>model\n";
        if (verbose) {
            std::cerr << "[verbose] prompt tokens (approx): " << final_prompt.size() / 3 << "\n";
            std::cerr << "[verbose] full prompt:\n" << final_prompt << "\n[verbose] ---\n";
        }
        std::string answer = run_inference(final_prompt, 512);
        std::cout << "\n=== 回答 ===\n" << answer << std::endl;
    }

    free_llama();
    return 0;
}
