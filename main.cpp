#include <algorithm>
#include <iomanip>
#include <iostream>
#include <random>
#include <utility>
#include <vector>

using Grid = std::vector<std::vector<int>>;

constexpr int Rows = 5;
constexpr int Columns = 5;
constexpr int ColorCount = 4;

struct Position {
    int row;
    int column;
};

bool hasMatches(const Grid& grid);

bool isInside(const Grid& grid, int row, int column) {
    return row >= 0 && row < static_cast<int>(grid.size()) &&
           column >= 0 && column < static_cast<int>(grid[row].size());
}

bool areAdjacent(Position first, Position second) {
    const int rowDistance = std::abs(first.row - second.row);
    const int columnDistance = std::abs(first.column - second.column);
    return rowDistance + columnDistance == 1;
}

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

void printGrid(const Grid& grid) {
    std::cout << "\n    ";
    for (int column = 0; column < static_cast<int>(grid[0].size()); ++column) {
        std::cout << std::setw(3) << column;
    }
    std::cout << '\n';

    for (int row = 0; row < static_cast<int>(grid.size()); ++row) {
        std::cout << std::setw(3) << row << ' ';
        for (int cell : grid[row]) {
            std::cout << std::setw(3) << cell;
        }
        std::cout << '\n';
    }
}

void swapCells(Grid& grid, Position first, Position second) {
    std::swap(grid[first.row][first.column], grid[second.row][second.column]);
}

// Finds every horizontal and vertical run of 3 or more values and replaces it
// with 0. Returns true when at least one match was found.
bool checkMatches(Grid& grid) {
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

    bool foundMatch = false;
    for (int row = 0; row < rows; ++row) {
        for (int column = 0; column < columns; ++column) {
            if (matched[row][column]) {
                grid[row][column] = 0;
                foundMatch = true;
            }
        }
    }
    return foundMatch;
}

bool hasMatches(const Grid& grid) {
    Grid copy = grid;
    return checkMatches(copy);
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

void resolveMatches(Grid& grid, std::mt19937& randomGenerator) {
    while (checkMatches(grid)) {
        applyGravityAndFill(grid, randomGenerator);
    }
}

bool makeMove(Grid& grid, Position first, Position second, std::mt19937& randomGenerator) {
    if (!isInside(grid, first.row, first.column) ||
        !isInside(grid, second.row, second.column) ||
        !areAdjacent(first, second)) {
        return false;
    }

    swapCells(grid, first, second);
    if (!checkMatches(grid)) {
        swapCells(grid, first, second); // Invalid move: restore the original grid.
        return false;
    }

    applyGravityAndFill(grid, randomGenerator);
    resolveMatches(grid, randomGenerator); // Check for new matches after gravity and fill.
    return true;
}

int main() {
    std::random_device seed;
    std::mt19937 randomGenerator(seed());
    Grid grid = initializeGrid(Rows, Columns, randomGenerator);

    std::cout << "My 3-match game.\n"
              << "Enter two adjacent positions as: row column row column.\n"
              << "Enter -1 to quit.\n";

    while (true) {
        // A new swap is only allowed after all existing matches are resolved.
        resolveMatches(grid, randomGenerator);
        printGrid(grid);
        std::cout << "\nSwap: ";

        Position first{};
        Position second{};
        if (!(std::cin >> first.row)) {
            break;
        }
        if (first.row == -1) {
            break;
        }
        if (!(std::cin >> first.column >> second.row >> second.column)) {
            std::cerr << "Please enter four integer coordinates.\n";
            break;
        }

        if (makeMove(grid, first, second, randomGenerator)) {
            std::cout << "Match found.\n";
        } else {
            std::cout << "Invalid move or no match; the swap was reversed.\n";
        }
    }

    return 0;
}
