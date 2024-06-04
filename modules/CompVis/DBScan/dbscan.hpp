#pragma once
#include <vector>

std::vector<std::vector<size_t>> dbscan(const std::vector<std::pair<float, float>>& data, float eps, int min_pts);
