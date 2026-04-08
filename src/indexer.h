#pragma once
#include <string>
#include <vector>
#include "scanner.h"

void build_index(const std::string& project_root, const std::vector<FileInfo>& files);
