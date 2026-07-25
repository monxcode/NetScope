#ifndef NETSCOPE_UI_TERMINAL_H
#define NETSCOPE_UI_TERMINAL_H

#include <string>
#include <vector>
#include <functional>

namespace netscope {
namespace ui {

enum class Color {
    Reset,
    Black,
    Red,
    Green,
    Yellow,
    Blue,
    Magenta,
    Cyan,
    White,
    BrightBlack,
    BrightRed,
    BrightGreen,
    BrightYellow,
    BrightBlue,
    BrightMagenta,
    BrightCyan,
    BrightWhite
};

enum class Style {
    Normal,
    Bold,
    Dim,
    Italic,
    Underline,
    Blink,
    Reverse
};

class Terminal {
public:
    static void Init();
    static void Reset();
    static void Clear();
    static void ClearLine();
    static void MoveUp(int lines = 1);
    static void MoveDown(int lines = 1);
    static void SetCursor(int row, int col);
    static void HideCursor();
    static void ShowCursor();

    static void SetColor(Color color);
    static void SetStyle(Style style);
    static void ResetStyle();

    static void Print(const std::string& text);
    static void PrintLine(const std::string& text);
    static void PrintColored(const std::string& text, Color color);
    static void PrintStyled(const std::string& text, Style style);
    static void PrintSuccess(const std::string& text);
    static void PrintError(const std::string& text);
    static void PrintWarning(const std::string& text);
    static void PrintInfo(const std::string& text);
    static void PrintHeader(const std::string& text);

    static std::string Colorize(const std::string& text, Color color);
    static std::string Stylize(const std::string& text, Style style);

    static int GetTerminalWidth();
    static int GetTerminalHeight();

    static std::string Center(const std::string& text, int width = 0);
    static std::string PadRight(const std::string& text, int width);
    static std::string PadLeft(const std::string& text, int width);
    static std::string Truncate(const std::string& text, int max_width);

    static std::string HorizontalRule(char c = '=', int width = 0);
    static std::string Box(const std::vector<std::string>& lines, int padding = 1);

    static bool SupportsColor();

    Terminal() = delete;
};

} // namespace ui
} // namespace netscope

#endif
