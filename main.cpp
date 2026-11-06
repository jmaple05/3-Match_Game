#include <random>
#include <sstream>
#include <string>

#include "game_logic.h"

#define WIN32_LEAN_AND_MEAN
//#define //NOMINMAX
#include <windows.h>

using namespace std;

namespace {

constexpr int WindowWidth = 640;
constexpr int WindowHeight = 700;
constexpr int MaxTileSize = 92;
constexpr int TileGap = 8;
constexpr int StartButtonId = 1001;

Grid gameGrid;
mt19937 randomGenerator;
int matchCount = 0;
Position selected{};
bool hasSelection = false;
bool inputBlocked = false;
bool setupScreen = true;
string statusMessage;
HWND rowsInput = nullptr;
HWND columnsInput = nullptr;
HWND startButton = nullptr;

const COLORREF TileColors[] = {
    RGB(37, 48, 65),   // Empty / background
    RGB(245, 106, 108),
    RGB(255, 190, 92),
    RGB(91, 190, 124),
    RGB(92, 157, 232),
};

int tileSizeForWindow(HWND window) {
    RECT client{};
    GetClientRect(window, &client);
    const int rows = static_cast<int>(gameGrid.size());
    const int columns = rows == 0 ? DefaultColumns : static_cast<int>(gameGrid[0].size());
    const int widthForTiles = (client.right - 190 - TileGap * (columns - 1)) / columns;
    const int heightForTiles = (client.bottom - 210 - TileGap * (rows - 1)) / rows;
    return std::max(24, std::min(MaxTileSize, std::min(widthForTiles, heightForTiles)));
}

RECT boardRect(HWND window) {
    RECT client{};
    GetClientRect(window, &client);
    const int rows = static_cast<int>(gameGrid.size());
    const int columns = rows == 0 ? DefaultColumns : static_cast<int>(gameGrid[0].size());
    const int tileSize = tileSizeForWindow(window);
    const int boardWidth = columns * tileSize + (columns - 1) * TileGap;
    const int boardHeight = rows * tileSize + (rows - 1) * TileGap;
    const int left = static_cast<int>(std::max(20L, (client.right - 170 - boardWidth) / 2));
    const int top = 120;
    return {left, top, left + boardWidth, top + boardHeight};
}

void setStatus(const string& message) {
    statusMessage = message;
}

void startNewGame(int rows, int columns) {
    gameGrid = initializeGrid(rows, columns, randomGenerator);
    if (!hasPossibleMove(gameGrid)) {
        reorganizeGrid(gameGrid, randomGenerator);
    }
    hasSelection = false;
    inputBlocked = false;
    matchCount = 0;
    setStatus("Select a tile, then select an adjacent tile to swap.");
}

void setSetupControlsVisible(bool visible) {
    const int command = visible ? SW_SHOW : SW_HIDE;
    ShowWindow(rowsInput, command);
    ShowWindow(columnsInput, command);
    ShowWindow(startButton, command);
}

bool readDimension(HWND input, int& dimension) {
    char text[32]{};
    GetWindowTextA(input, text, sizeof(text));
    istringstream stream(text);
    char extra{};
    return (stream >> dimension) && !(stream >> extra) && dimension >= 3 && dimension <= 12;
}

void beginGameFromSetup(HWND window) {
    int rows = 0;
    int columns = 0;
    if (!readDimension(rowsInput, rows) || !readDimension(columnsInput, columns)) {
        statusMessage = "Enter whole numbers from 3 to 12 for both dimensions.";
        InvalidateRect(window, nullptr, FALSE);
        return;
    }

    setupScreen = false;
    setSetupControlsVisible(false);
    startNewGame(rows, columns);
    InvalidateRect(window, nullptr, FALSE);
}

void createSetupControls(HWND window, HINSTANCE instance) {
    rowsInput = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "5",
                                WS_CHILD | WS_VISIBLE | ES_NUMBER | ES_CENTER,
                                290, 210, 100, 34, window, nullptr, instance, nullptr);
    columnsInput = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "5",
                                   WS_CHILD | WS_VISIBLE | ES_NUMBER | ES_CENTER,
                                   290, 270, 100, 34, window, nullptr, instance, nullptr);
    startButton = CreateWindowExA(0, "BUTTON", "Start Game",
                                  WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                                  240, 335, 200, 42, window,
                                  reinterpret_cast<HMENU>(StartButtonId), instance, nullptr);
}

bool positionFromPoint(HWND window, int x, int y, Position& position) {
    const RECT board = boardRect(window);
    if (x < board.left || y < board.top || x >= board.right || y >= board.bottom) {
        return false;
    }

    const int tileSize = tileSizeForWindow(window);
    const int step = tileSize + TileGap;
    const int column = (x - board.left) / step;
    const int row = (y - board.top) / step;
    const int tileX = (x - board.left) % step;
    const int tileY = (y - board.top) % step;
    if (tileX >= tileSize || tileY >= tileSize) {
        return false;
    }

    position = {row, column};
    return true;
}

void fillRect(HDC deviceContext, const RECT& rectangle, COLORREF color) {
    HBRUSH brush = CreateSolidBrush(color);
    FillRect(deviceContext, &rectangle, brush);
    DeleteObject(brush);
}

void drawText(HDC deviceContext, const string& text, int x, int y,
              int width, int height, COLORREF color, int fontSize, UINT format) {
    SetTextColor(deviceContext, color);
    SetBkMode(deviceContext, TRANSPARENT);
    HFONT font = CreateFontA(fontSize, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
    HFONT previousFont = static_cast<HFONT>(SelectObject(deviceContext, font));
    RECT rectangle{x, y, x + width, y + height};
    DrawTextA(deviceContext, text.c_str(), -1, &rectangle, format);
    SelectObject(deviceContext, previousFont);
    DeleteObject(font);
}

void paintWindow(HWND window, HDC deviceContext) {
    RECT client{};
    GetClientRect(window, &client);
    fillRect(deviceContext, client, RGB(19, 25, 38));

    drawText(deviceContext, "3-MATCH", 0, 24, client.right, 42,
             RGB(245, 248, 252), 30, DT_CENTER | DT_SINGLELINE);

    if (setupScreen) {
        drawText(deviceContext, "Choose the size of your board", 0, 105,
                 client.right, 28, RGB(210, 219, 232), 17, DT_CENTER | DT_SINGLELINE);
        drawText(deviceContext, "Rows", 175, 218, 100, 24,
                 RGB(163, 176, 197), 15, DT_RIGHT | DT_SINGLELINE);
        drawText(deviceContext, "Columns", 175, 278, 100, 24,
                 RGB(163, 176, 197), 15, DT_RIGHT | DT_SINGLELINE);
        drawText(deviceContext, statusMessage, 40, 390, client.right - 80, 42,
                 RGB(245, 150, 150), 14, DT_CENTER | DT_WORDBREAK);
        return;
    }

    drawText(deviceContext, "Match lines, 2x2 blocks, or L shapes", 0, 70,
             client.right, 26, RGB(163, 176, 197), 15, DT_CENTER | DT_SINGLELINE);

    drawText(deviceContext, "MATCHES", client.right - 155, 150, 130, 26,
             RGB(163, 176, 197), 15, DT_CENTER | DT_SINGLELINE);
    drawText(deviceContext, to_string(matchCount), client.right - 155, 180, 130, 55,
             RGB(245, 248, 252), 38, DT_CENTER | DT_SINGLELINE);

    const RECT board = boardRect(window);
    const int tileSize = tileSizeForWindow(window);
    for (int row = 0; row < static_cast<int>(gameGrid.size()); ++row) {
        for (int column = 0; column < static_cast<int>(gameGrid[row].size()); ++column) {
            const int left = board.left + column * (tileSize + TileGap);
            const int top = board.top + row * (tileSize + TileGap);
            RECT tile{left, top, left + tileSize, top + tileSize};
            const int value = gameGrid[row][column];
            fillRect(deviceContext, tile, TileColors[value]);

            if (hasSelection && selected.row == row && selected.column == column) {
                HPEN pen = CreatePen(PS_SOLID, 4, RGB(255, 255, 255));
                HGDIOBJ previousPen = SelectObject(deviceContext, pen);
                HGDIOBJ previousBrush = SelectObject(deviceContext, GetStockObject(HOLLOW_BRUSH));
                Rectangle(deviceContext, left + 2, top + 2, left + tileSize - 2, top + tileSize - 2);
                SelectObject(deviceContext, previousBrush);
                SelectObject(deviceContext, previousPen);
                DeleteObject(pen);
            }
        }
    }

    drawText(deviceContext, statusMessage, 30, 610, client.right - 60, 28,
             RGB(210, 219, 232), 14, DT_CENTER | DT_SINGLELINE);
    drawText(deviceContext, "R  New game    |    Esc  Quit", 0, 650, client.right, 24,
             RGB(120, 137, 161), 13, DT_CENTER | DT_SINGLELINE);
}

LRESULT CALLBACK windowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_PAINT: {
        PAINTSTRUCT paint{};
        HDC deviceContext = BeginPaint(window, &paint);
        paintWindow(window, deviceContext);
        EndPaint(window, &paint);
        return 0;
    }
    case WM_LBUTTONUP: {
        // Treat swapping and resolving all resulting matches as one transaction.
        // This also ignores clicks that may already be queued by Windows.
        if (inputBlocked) {
            return 0;
        }

        Position clicked{};
        const int mouseX = static_cast<int>(static_cast<short>(LOWORD(lParam)));
        const int mouseY = static_cast<int>(static_cast<short>(HIWORD(lParam)));
        if (!positionFromPoint(window, mouseX, mouseY, clicked)) {
            return 0;
        }

        if (!hasSelection) {
            selected = clicked;
            hasSelection = true;
            setStatus("Now select an adjacent tile.");
        } else if (selected.row == clicked.row && selected.column == clicked.column) {
            hasSelection = false;
            setStatus("Selection cleared. Select a tile to begin a swap.");
        } else {
            inputBlocked = true;
            setStatus("Resolving match...");
            InvalidateRect(window, nullptr, FALSE);
            UpdateWindow(window);

            const AnimationFrame showGravityFrame = [window]() {
                InvalidateRect(window, nullptr, FALSE);
                UpdateWindow(window);
                Sleep(250);
            };
            const bool validMove = makeMove(gameGrid, selected, clicked, randomGenerator,
                                            matchCount, showGravityFrame);
            inputBlocked = false;
            hasSelection = false;
            if (validMove) {
                setStatus("Match found! Select another tile to continue.");
            } else {
                setStatus("That swap is invalid or makes no match.");
            }
        }
        InvalidateRect(window, nullptr, FALSE);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == StartButtonId && HIWORD(wParam) == BN_CLICKED) {
            beginGameFromSetup(window);
            return 0;
        }
        break;
    case WM_KEYDOWN:
        if (inputBlocked) {
            return 0;
        }
        if (wParam == 'R') {
            setupScreen = true;
            statusMessage.clear();
            setSetupControlsVisible(true);
            InvalidateRect(window, nullptr, FALSE);
            return 0;
        }
        if (wParam == VK_ESCAPE) {
            DestroyWindow(window);
            return 0;
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        break;
    }
    return DefWindowProcA(window, message, wParam, lParam);
}

} // namespace

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int showCommand) {
    random_device seed;
    randomGenerator.seed(seed());
    statusMessage.clear();

    const char* className = "MatchGameWindow";
    WNDCLASSA windowClass{};
    windowClass.hInstance = instance;
    windowClass.lpfnWndProc = windowProcedure;
    windowClass.lpszClassName = className;
    windowClass.hCursor = LoadCursor(nullptr, IDC_HAND);
    windowClass.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    RegisterClassA(&windowClass);

    HWND window = CreateWindowExA(
        0, className, "3-Match Game", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, WindowWidth, WindowHeight,
        nullptr, nullptr, instance, nullptr);
    if (window == nullptr) {
        return 1;
    }

    createSetupControls(window, instance);

    ShowWindow(window, showCommand);
    UpdateWindow(window);

    MSG message{};
    while (GetMessageA(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }
    return static_cast<int>(message.wParam);
}
