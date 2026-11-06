#pragma once

#include <functional>
#include <random>
#include <vector>

using Grid = std::vector<std::vector<int>>;
using AnimationFrame = std::function<void()>;

constexpr int DefaultColumns = 5;

struct Position {
    int row;
    int column;
};

Grid initializeGrid(int rows, int columns, std::mt19937& randomGenerator);
bool hasMatches(const Grid& grid);
bool hasPossibleMove(Grid& grid);
void reorganizeGrid(Grid& grid, std::mt19937& randomGenerator);
bool makeMove(Grid& grid, Position first, Position second,
              std::mt19937& randomGenerator, int& matchCount,
              const AnimationFrame& showFrame = {});
