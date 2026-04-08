#pragma once
#include <string>

void init_llama(const std::string& model_path);
void free_llama();
std::string run_inference(const std::string& prompt, int max_tokens = 512);
int count_tokens(const std::string& text);
std::string generate_file_wiki(const std::string& file_content, const std::string& file_path);
std::string generate_module_summary(const std::string& module_code);
