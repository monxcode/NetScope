#include "netscope/ui/terminal.h"

#include <iostream>
#include <sstream>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <consoleapi.h>
#endif

namespace netscope {
namespace ui {

#ifdef _WIN32
static HANDLE console_handle = nullptr;
static WORD original_attributes = 0;
static bool console_initialized = false;

static void InitConsole() {
    if (console_initialized) return;
    console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (GetConsoleScreenBufferInfo(console_handle, &info)) {
        original_attributes = info.wAttributes;
    }
    console_initialized = true;
}
#endif

void Terminal::Init() {
#ifdef _WIN32
    InitConsole();
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(hOut, &mode)) {
        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, mode);
    }
#endif
}

void Terminal::Reset() {
#ifdef _WIN32
    if (console_handle && console_initialized) {
        SetConsoleTextAttribute(console_handle, original_attributes);
    }
#endif
    std::cout << "\033[0m" << std::flush;
}

void Terminal::Clear() {
    std::cout << "\033[2J\033[H" << std::flush;
}

void Terminal::ClearLine() {
    std::cout << "\033[2K\r" << std::flush;
}

void Terminal::MoveUp(int lines) {
    std::cout << "\033[" << lines << "A" << std::flush;
}

void Terminal::MoveDown(int lines) {
    std::cout << "\033[" << lines << "B" << std::flush;
}

void Terminal::SetCursor(int row, int col) {
    std::cout << "\033[" << row << ";" << col << "H" << std::flush;
}

void Terminal::HideCursor() {
    std::cout << "\033[?25l" << std::flush;
}

void Terminal::ShowCursor() {
    std::cout << "\033[?25h" << std::flush;
}

void Terminal::SetColor(Color color) {
    std::cout << "\033[" << static_cast<int>(color) << "m" << std::flush;
}

void Terminal::SetStyle(Style style) {
    std::cout << "\033[" << static_cast<int>(style) << "m" << std::flush;
}

void Terminal::ResetStyle() {
    std::cout << "\033[0m" << std::flush;
}

void Terminal::Print(const std::string& text) {
    std::cout << text << std::flush;
}

void Terminal::PrintLine(const std::string& text) {
    std::cout << text << std::endl;
}

void Terminal::PrintColored(const std::string& text, Color color) {
    SetColor(color);
    std::cout << text << std::flush;
    ResetStyle();
}

void Terminal::PrintStyled(const std::string& text, Style style) {
    SetStyle(style);
    std::cout << text << std::flush;
    ResetStyle();
}

void Terminal::PrintSuccess(const std::string& text) {
    PrintColored("[+] " + text, Color::Green);
    std::cout << std::endl;
}

void Terminal::PrintError(const std::string& text) {
    PrintColored("[-] " + text, Color::Red);
    std::cout << std::endl;
}

void Terminal::PrintWarning(const std::string& text) {
    PrintColored("[!] " + text, Color::Yellow);
    std::cout << std::endl;
}

void Terminal::PrintInfo(const std::string& text) {
    PrintColored("[*] " + text, Color::Cyan);
    std::cout << std::endl;
}

void Terminal::PrintHeader(const std::string& text) {
    std::cout << std::endl;
    SetColor(Color::Cyan);
    SetStyle(Style::Bold);
    std::cout << "=== " << text << " ===" << std::endl;
    ResetStyle();
}

std::string Terminal::Colorize(const std::string& text, Color color) {
    return "\033[" + std::to_string(static_cast<int>(color)) + "m" +
           text + "\033[0m";
}

std::string Terminal::Stylize(const std::string& text, Style style) {
    return "\033[" + std::to_string(static_cast<int>(style)) + "m" +
           text + "\033[0m";
}

int Terminal::GetTerminalWidth() {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
        return csbi.srWindow.Right - csbi.srWindow.Left + 1;
    }
#endif
    return 80;
}

int Terminal::GetTerminalHeight() {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
        return csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    }
#endif
    return 24;
}

std::string Terminal::Center(const std::string& text, int width) {
    if (width == 0) width = GetTerminalWidth();
    int padding = std::max(0, (width - static_cast<int>(text.length())) / 2);
    return std::string(padding, ' ') + text;
}

std::string Terminal::PadRight(const std::string& text, int width) {
    if (static_cast<int>(text.length()) >= width) return text.substr(0, width);
    return text + std::string(width - text.length(), ' ');
}

std::string Terminal::PadLeft(const std::string& text, int width) {
    if (static_cast<int>(text.length()) >= width) return text.substr(0, width);
    return std::string(width - text.length(), ' ') + text;
}

std::string Terminal::Truncate(const std::string& text, int max_width) {
    if (static_cast<int>(text.length()) <= max_width) return text;
    return text.substr(0, max_width - 3) + "...";
}

std::string Terminal::HorizontalRule(char c, int width) {
    if (width == 0) width = GetTerminalWidth();
    return std::string(width, c);
}

std::string Terminal::Box(const std::vector<std::string>& lines, int padding) {
    if (lines.empty()) return "";

    size_t max_len = 0;
    for (const auto& line : lines) {
        max_len = std::max(max_len, line.length());
    }

    int inner_width = static_cast<int>(max_len) + 2 * padding;
    std::ostringstream oss;

    oss << "┌" << std::string(inner_width, '─') << "┐\n";
    for (const auto& line : lines) {
        oss << "│" << std::string(padding, ' ')
            << PadRight(line, static_cast<int>(max_len))
            << std::string(padding, ' ') << "│\n";
    }
    oss << "└" << std::string(inner_width, '─') << "┘";

    return oss.str();
}

bool Terminal::SupportsColor() {
#ifdef _WIN32
    return true;
#else
    const char* term = getenv("TERM");
    return term != nullptr;
#endif
}

} // namespace ui
} // namespace netscope
