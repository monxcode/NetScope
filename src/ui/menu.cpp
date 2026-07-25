#include "netscope/ui/menu.h"
#include "netscope/ui/terminal.h"

#include <iostream>
#include <sstream>
#include <algorithm>
#include <thread>
#include <chrono>
#include <iomanip>

namespace netscope {
namespace ui {

Menu::Menu(const std::string& title) : title_(title) {}

void Menu::SetTitle(const std::string& title) {
    title_ = title;
}

void Menu::AddItem(int key, const std::string& label,
                   std::function<void()> action,
                   const std::string& description) {
    items_.push_back({key, label, description, std::move(action)});
}

void Menu::AddSeparator() {
    separators_.push_back(static_cast<int>(items_.size()));
}

void Menu::SetFooter(const std::string& footer) {
    footer_ = footer;
}

void Menu::Display() {
    Terminal::Clear();

    std::cout << "\n";
    std::cout << Terminal::Colorize("  ╔═══════════════════════════════════════════╗", Color::Cyan) << "\n";
    std::cout << Terminal::Colorize("  ║  " + Terminal::PadRight(title_, 43) + "║", Color::Cyan) << "\n";
    std::cout << Terminal::Colorize("  ╚═══════════════════════════════════════════╝", Color::Cyan) << "\n";
    std::cout << "\n";

    for (size_t i = 0; i < items_.size(); ++i) {
        bool is_sep = std::find(separators_.begin(), separators_.end(),
                                static_cast<int>(i)) != separators_.end();
        if (is_sep) {
            std::cout << "  " << std::string(45, '─') << "\n";
        }

        const auto& item = items_[i];
        std::string key_str = std::to_string(item.key);

        std::cout << "  ";
        Terminal::SetColor(Color::Yellow);
        Terminal::SetStyle(Style::Bold);
        std::cout << "[" << key_str << "]";
        Terminal::ResetStyle();
        std::cout << " ";

        Terminal::SetStyle(Style::Bold);
        std::cout << item.label;
        Terminal::ResetStyle();

        if (!item.description.empty()) {
            std::cout << "\n      " << Terminal::Colorize(item.description, Color::BrightBlack);
        }
        std::cout << "\n";
    }

    if (!footer_.empty()) {
        std::cout << "\n  " << Terminal::Colorize(footer_, Color::BrightBlack) << "\n";
    }

    std::cout << "\n  " << Terminal::Colorize("Enter choice: ", Color::Cyan) << std::flush;
}

int Menu::Show() {
    Display();

    int choice;
    std::string input;
    std::getline(std::cin, input);

    try {
        choice = std::stoi(input);
    } catch (...) {
        choice = -1;
    }

    return choice;
}

void Menu::Run() {
    while (true) {
        int choice = Show();

        bool found = false;
        for (auto& item : items_) {
            if (item.key == choice) {
                found = true;
                if (item.action) {
                    item.action();
                }
                if (choice == 0) {
                    return;
                }
                std::cout << "\n  " << Terminal::Colorize("Press Enter to continue...", Color::BrightBlack) << std::flush;
                std::cin.get();
                break;
            }
        }

        if (!found) {
            Terminal::PrintError("Invalid choice");
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
}

ProgressBar::ProgressBar(int total, const std::string& label)
    : total_(total), label_(label) {
    Draw(0);
}

void ProgressBar::Update(int current) {
    Draw(current);
}

void ProgressBar::Complete(const std::string& message) {
    Draw(total_);
    std::cout << " " << message << std::endl;
}

void ProgressBar::SetSuffix(const std::string& suffix) {
    suffix_ = suffix;
}

void ProgressBar::Draw(int current) {
    if (current == last_drawn_) return;
    last_drawn_ = current;

    int width = Terminal::GetTerminalWidth() - 20;
    if (width < 20) width = 20;

    double pct = total_ > 0 ? (static_cast<double>(current) / total_) : 0;
    int filled = static_cast<int>(pct * width);

    Terminal::ClearLine();

    if (!label_.empty()) {
        std::cout << Terminal::PadRight(label_, 15);
    }

    Terminal::SetColor(Color::Cyan);
    std::cout << "[";
    Terminal::ResetStyle();

    for (int i = 0; i < width; ++i) {
        if (i < filled) {
            Terminal::SetColor(Color::Green);
            std::cout << "=";
            Terminal::ResetStyle();
        } else if (i == filled) {
            Terminal::SetColor(Color::Green);
            std::cout << ">";
            Terminal::ResetStyle();
        } else {
            std::cout << " ";
        }
    }

    Terminal::SetColor(Color::Cyan);
    std::cout << "]";
    Terminal::ResetStyle();

    std::cout << " " << std::fixed << std::setprecision(1) << (pct * 100) << "%";

    if (!suffix_.empty()) {
        std::cout << " " << suffix_;
    }

    std::cout << std::flush;
}

Table::Table(Header header) : header_(std::move(header)) {}

void Table::AddRow(const Row& row) {
    rows_.push_back(row);
    separators_.push_back(false);
}

void Table::AddSeparator() {
    if (!separators_.empty()) {
        separators_.back() = true;
    }
}

void Table::SetTitle(const std::string& title) {
    title_ = title;
}

std::vector<int> Table::CalculateWidths() const {
    std::vector<int> widths(header_.size(), 0);

    for (size_t i = 0; i < header_.size(); ++i) {
        widths[i] = std::max(widths[i], static_cast<int>(header_[i].length()));
    }

    for (const auto& row : rows_) {
        for (size_t i = 0; i < row.size() && i < widths.size(); ++i) {
            widths[i] = std::max(widths[i], static_cast<int>(row[i].length()));
        }
    }

    for (auto& w : widths) {
        w = std::min(w, 40);
        w += 2;
    }

    return widths;
}

std::string Table::Render() const {
    auto widths = CalculateWidths();
    std::ostringstream oss;

    auto hrule = [&]() {
        oss << "+";
        for (auto w : widths) {
            oss << std::string(w, '-') << "+";
        }
        oss << "\n";
    };

    auto render_row = [&](const Row& row, bool header_style) {
        oss << "|";
        for (size_t i = 0; i < widths.size(); ++i) {
            std::string cell = i < row.size() ? row[i] : "";
            int padding = widths[i] - static_cast<int>(cell.length()) - 1;
            oss << " " << Terminal::Truncate(cell, widths[i] - 2)
                << std::string(padding, ' ') << "|";
        }
        oss << "\n";
    };

    if (!title_.empty()) {
        int total_width = 1;
        for (auto w : widths) total_width += w + 1;
        oss << Terminal::Center(title_, total_width) << "\n";
    }

    hrule();
    render_row(header_, true);
    hrule();

    for (size_t i = 0; i < rows_.size(); ++i) {
        render_row(rows_[i], false);
        if (i + 1 < rows_.size() && separators_[i]) {
            hrule();
        }
    }

    hrule();

    return oss.str();
}

void Table::Display() const {
    std::cout << Render();
}

} // namespace ui
} // namespace netscope
