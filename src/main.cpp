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
        std::cerr << "Usage: project_bot <project_dir> [--model <model_path>] [--query <question>]\n";
        return 1;
    }

    std::string project_dir = argv[1];
    std::string model_path = "models/gemma-4-e4b.Q4_K_M.gguf";

    // 解析可选参数
    std::string query;
    for (int i = 2; i < argc - 1; ++i) {
        if (std::string(argv[i]) == "--model") model_path = argv[++i];
        else if (std::string(argv[i]) == "--query") query = argv[++i];
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
        write_file(out_path, wiki);
        std::cout << "Generated: " << out_path << std::endl;
    }

    build_index(project_dir, files);

    if (!query.empty()) {
        std::string context = get_relevant_context(query);
        std::string final_prompt = "根据以下项目信息回答问题：\n" + context + "\n问题：" + query + "\n回答：";
        std::string answer = run_inference(final_prompt, 512);
        std::cout << "\n=== 回答 ===\n" << answer << std::endl;
    }

    free_llama();
    return 0;
}
