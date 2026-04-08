cmake_minimum_required(VERSION 3.14)
project(ProjectBot)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CUDA_ARCHITECTURES "89")  # 你的 RTX 4060 Ti

# 添加 llama.cpp 子项目
add_subdirectory(vendor/llama.cpp)

add_executable(project_bot
    src/main.cpp
    src/scanner.cpp
    src/indexer.cpp
    src/wiki_generator.cpp
    src/context.cpp
    src/utils.cpp
)

target_link_libraries(project_bot PRIVATE llama)
target_include_directories(project_bot PRIVATE src)
