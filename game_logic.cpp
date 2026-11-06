#include "game_logic.h"

#include <algorithm>
#include <cstdlib>

namespace {

constexpr int ColorCount = 4;

bool isInside(const Grid& grid, int row, int column) {
    return row >= 0 && row < static_cast<int>(grid.size()) &&
           column >= 0 && column < static_cast<int>(grid[row].size());
}

bool areAdjacent(Position first, Position second) {
    const int rowDistance = std::abs(first.row - second.row);
    const int columnDistance = std::abs(first.column - second.column);
    return rowDistance + columnDistance == 1;
}

void swapCells(Grid& grid, Position first, Position second) {
    std::swap(grid[first.row][first.column], grid[second.row][second.column]);
}

bool checkMatches(Grid& grid, int* matchCount = nullptr) {
    if (matchCount != nullptr) {
        *matchCount = 0;
    }
    if (grid.empty() || grid[0].empty()) {
        return false;
    }

    const int rows = static_cast<int>(grid.size());
    const int columns = static_cast<int>(grid[0].size());
    std::vector<std::vector<bool>> matched(rows, std::vector<bool>(columns, false));

    for (int row = 0; row < rows; ++row) {
        int start = 0;
        while (start < columns) {
            int end = start + 1;
            while (end < columns && grid[row][end] != 0 &&
                   grid[row][end] == grid[row][start]) {
                ++end;
            }
            if (grid[row][start] != 0 && end - start >= 3) {
                for (int column = start; column < end; ++column) {
                    matched[row][column] = true;
                }
            }
            start = end;
        }
    }

    for (int column = 0; column < columns; ++column) {
        int start = 0;
        while (start < rows) {
            int end = start + 1;
            while (end < rows && grid[end][column] != 0 &&
                   grid[end][column] == grid[start][column]) {
                ++end;
            }
            if (grid[start][column] != 0 && end - start >= 3) {
                for (int row = start; row < end; ++row) {
                    matched[row][column] = true;
                }
            }
            start = end;
        }
    }

    // A 2x2 block is a match even though neither its rows nor columns contain
    // three tiles.
    for (int row = 0; row + 1 < rows; ++row) {
        for (int column = 0; column + 1 < columns; ++column) {
            const int value = grid[row][column];
            if (value != 0 && grid[row][column + 1] == value &&
                grid[row + 1][column] == value &&
                grid[row + 1][column + 1] == value) {
                matched[row][column] = true;
                matched[row][column + 1] = true;
                matched[row + 1][column] = true;
                matched[row + 1][column + 1] = true;
            }
        }
    }

    const Grid originalGrid = grid;
    bool foundMatch = false;
    for (int row = 0; row < rows; ++row) {
        for (int column = 0; column < columns; ++column) {
            if (matched[row][column]) {
                grid[row][column] = 0;
                foundMatch = true;
            }
        }
    }

    // Count connected matched tiles, so a line, 2x2 block, or L shape counts
    // as one match instead of counting each cleared tile.
    if (matchCount != nullptr) {
        std::vector<std::vector<bool>> visited(rows, std::vector<bool>(columns, false));
        const int rowOffsets[] = {-1, 1, 0, 0};
        const int columnOffsets[] = {0, 0, -1, 1};
        for (int row = 0; row < rows; ++row) {
            for (int column = 0; column < columns; ++column) {
                if (!matched[row][column] || visited[row][column]) {
                    continue;
                }
                ++*matchCount;
                std::vector<Position> pending{{row, column}};
                visited[row][column] = true;
                while (!pending.empty()) {
                    const Position current = pending.back();
                    pending.pop_back();
                    for (int direction = 0; direction < 4; ++direction) {
                        const int nextRow = current.row + rowOffsets[direction];
                        const int nextColumn = current.column + columnOffsets[direction];
                        if (isInside(grid, nextRow, nextColumn) &&
                            matched[nextRow][nextColumn] &&
                            originalGrid[nextRow][nextColumn] ==
                                originalGrid[current.row][current.column] &&
                            !visited[nextRow][nextColumn]) {
                            visited[nextRow][nextColumn] = true;
                            pending.push_back({nextRow, nextColumn});
                        }
                    }
                }
            }
        }
    }
    return foundMatch;
}

void applyGravityAndFill(Grid& grid, std::mt19937& randomGenerator) {
    std::uniform_int_distribution<int> color(1, ColorCount);
    for (int column = 0; column < static_cast<int>(grid[0].size()); ++column) {
        int writeRow = static_cast<int>(grid.size()) - 1;
        for (int row = static_cast<int>(grid.size()) - 1; row >= 0; --row) {
            if (grid[row][column] != 0) {
                grid[writeRow][column] = grid[row][column];
                --writeRow;
            }
        }
        while (writeRow >= 0) {
            grid[writeRow][column] = color(randomGenerator);
            --writeRow;
        }
    }
}

void animateGravityAndFill(Grid& grid, std::mt19937& randomGenerator,
                           const AnimationFrame& showFrame) {
    std::uniform_int_distribution<int> color(1, ColorCount);
    bool moved = true;
    while (moved) {
        moved = false;
        for (int column = 0; column < static_cast<int>(grid[0].size()); ++column) {
            for (int row = static_cast<int>(grid.size()) - 1; row > 0; --row) {
                if (grid[row][column] == 0 && grid[row - 1][column] != 0) {
                    grid[row][column] = grid[row - 1][column];
                    grid[row - 1][column] = 0;
                    moved = true;
                }
            }
        }
        if (moved && showFrame) {
            showFrame();
        }
    }

    for (int row = 0; row < static_cast<int>(grid.size()); ++row) {
        bool filled = false;
        for (int column = 0; column < static_cast<int>(grid[row].size()); ++column) {
            if (grid[row][column] == 0) {
                grid[row][column] = color(randomGenerator);
                filled = true;
            }
        }
        if (filled && showFrame) {
            showFrame();
        }
    }
}

void resolveMatches(Grid& grid, std::mt19937& randomGenerator, int& matchCount,
                    const AnimationFrame& showFrame) {
    int matchesThisResolution = 0;
    while (checkMatches(grid, &matchesThisResolution)) {
        matchCount += matchesThisResolution;
        animateGravityAndFill(grid, randomGenerator, showFrame);
    }
}

} // namespace

Grid initializeGrid(int rows, int columns, std::mt19937& randomGenerator) {
    std::uniform_int_distribution<int> color(1, ColorCount);
    Grid grid;
    do {
        grid.assign(rows, std::vector<int>(columns));
        for (auto& row : grid) {
            for (int& cell : row) {
                cell = color(randomGenerator);
            }
        }
    } while (hasMatches(grid));
    return grid;
}

bool hasMatches(const Grid& grid) {
    Grid copy = grid;
    return checkMatches(copy);
}

bool makeMove(Grid& grid, Position first, Position second,
              std::mt19937& randomGenerator, int& matchCount,
              const AnimationFrame& showFrame) {
    if (!isInside(grid, first.row, first.column) ||
        !isInside(grid, second.row, second.column) ||
        !areAdjacent(first, second)) {
        return false;
    }
    swapCells(grid, first, second);
    int matchesThisMove = 0;
    if (!checkMatches(grid, &matchesThisMove)) {
        swapCells(grid, first, second);
        return false;
    }
    matchCount += matchesThisMove;
    animateGravityAndFill(grid, randomGenerator, showFrame);
    resolveMatches(grid, randomGenerator, matchCount, showFrame);
    if (!hasPossibleMove(grid)) {
        reorganizeGrid(grid, randomGenerator);
        if (showFrame) {
            showFrame();
        }
    }
    return true;
}

bool hasPossibleMove(Grid& grid) {
    if (grid.empty() || grid[0].empty()) {
        return false;
    }
    const int rows = static_cast<int>(grid.size());
    const int columns = static_cast<int>(grid[0].size());
    for (int row = 0; row < rows; ++row) {
        for (int column = 0; column < columns; ++column) {
            const Position first{row, column};
            const Position right{row, column + 1};
            const Position below{row + 1, column};
            for (const Position second : {right, below}) {
                if (!isInside(grid, second.row, second.column)) {
                    continue;
                }
                Grid testGrid = grid;
                swapCells(testGrid, first, second);
                if (hasMatches(testGrid)) {
                    return true;
                }
            }
        }
    }
    return false;
}

void reorganizeGrid(Grid& grid, std::mt19937& randomGenerator) {
    std::vector<int> tiles;
    for (const auto& row : grid) {
        tiles.insert(tiles.end(), row.begin(), row.end());
    }
    for (int attempt = 0; attempt < 5000; ++attempt) {
        std::shuffle(tiles.begin(), tiles.end(), randomGenerator);
        int tileIndex = 0;
        for (auto& row : grid) {
            for (int& tile : row) {
                tile = tiles[tileIndex++];
            }
        }
        if (!hasMatches(grid) && hasPossibleMove(grid)) {
            return;
        }
    }
    do {
        grid = initializeGrid(static_cast<int>(grid.size()),
                              static_cast<int>(grid[0].size()), randomGenerator);
    } while (!hasPossibleMove(grid));
}
