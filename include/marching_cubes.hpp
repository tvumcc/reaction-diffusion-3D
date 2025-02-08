#pragma once
#include <array>

// See https://paulbourke.net/geometry/polygonise/ for the source of these look up tables

extern std::array<int, 256> edge_table;
extern std::array<int, 24> vertex_table;
extern std::array<int, 4096> triangle_table;