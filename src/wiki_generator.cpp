#include "wiki_generator.h"
#include "llama.h"
#include <string>
#include <vector>

static llama_model* model = nullptr;
static llama_context* ctx = nullptr;

void init_llama(const std::string& model_path) {
    llama_backend_init();
    auto model_params = llama_model_default_params();
    model_params.n_gpu_layers = 99;
    model = llama_model_load_from_file(model_path.c_str(), model_params);
    if (!model) { fprintf(stderr, "Failed to load model\n"); exit(1); }
    auto ctx_params = llama_context_default_params();
    ctx_params.n_ctx = 8192;
    ctx_params.n_batch = 8192;
    ctx_params.n_threads = 8;
    ctx = llama_init_from_model(model, ctx_params);
    if (!ctx) { fprintf(stderr, "Failed to create context\n"); exit(1); }
}

void free_llama() {
    llama_free(ctx);
    llama_model_free(model);
    llama_backend_free();
}

std::string run_inference(const std::string& prompt, int max_tokens) {
    const llama_vocab* vocab = llama_model_get_vocab(model);

    std::vector<llama_token> tokens(prompt.size() + 32);
    int n_tokens = llama_tokenize(vocab, prompt.c_str(), prompt.size(),
                                  tokens.data(), tokens.size(), true, false);

    // Truncate to leave room for output
    const int max_prompt_tokens = 8192 - max_tokens;
    if (n_tokens > max_prompt_tokens) n_tokens = max_prompt_tokens;
    tokens.resize(n_tokens);

    // Recreate context to reset KV cache between inferences
    llama_free(ctx);
    auto ctx_params = llama_context_default_params();
    ctx_params.n_ctx = 8192;
    ctx_params.n_batch = 8192;
    ctx_params.n_threads = 8;
    ctx = llama_init_from_model(model, ctx_params);
    if (!ctx) return "";

    auto batch = llama_batch_get_one(tokens.data(), tokens.size());
    if (llama_decode(ctx, batch)) { return ""; }

    auto smpl_params = llama_sampler_chain_default_params();
    llama_sampler* smpl = llama_sampler_chain_init(smpl_params);
    llama_sampler_chain_add(smpl, llama_sampler_init_temp(0.7f));
    llama_sampler_chain_add(smpl, llama_sampler_init_dist(42));

    std::string output;
    for (int i = 0; i < max_tokens; ++i) {
        llama_token new_token = llama_sampler_sample(smpl, ctx, -1);
        if (llama_vocab_is_eog(vocab, new_token)) break;
        char buf[128];
        int n = llama_token_to_piece(vocab, new_token, buf, sizeof(buf), 0, false);
        output.append(buf, n);
        llama_batch batch_next = llama_batch_get_one(&new_token, 1);
        if (llama_decode(ctx, batch_next)) break;
    }
    llama_sampler_free(smpl);
    // 过滤模型可能输出的字面 special token 字符串
    for (auto& tok : {"<eos>", "<bos>", "<end_of_turn>", "<start_of_turn>"}) {
        std::string s = tok;
        size_t pos;
        while ((pos = output.find(s)) != std::string::npos)
            output.erase(pos, s.size());
    }
    return output;
}

int count_tokens(const std::string& text) {
    const llama_vocab* vocab = llama_model_get_vocab(model);
    std::vector<llama_token> tokens(text.size() + 32);
    int n = llama_tokenize(vocab, text.c_str(), text.size(),
                           tokens.data(), tokens.size(), false, false);
    return n < 0 ? 0 : n;
}

std::string generate_file_wiki(const std::string& file_content, const std::string& file_path) {
    std::string prompt = "<start_of_turn>user\n你是一个代码文档专家。为以下文件生成 Wiki 条目（Markdown 格式）：\n文件：" + file_path + "\n内容：\n" + file_content + "<end_of_turn>\n<start_of_turn>model\n";
    return run_inference(prompt, 1024);
}

std::string generate_module_summary(const std::string& module_code) {
    std::string prompt = "<start_of_turn>user\n用一句话总结以下代码模块的功能，只输出一句话，不要多余内容：\n" + module_code + "<end_of_turn>\n<start_of_turn>model\n";
    return run_inference(prompt, 80);
}
