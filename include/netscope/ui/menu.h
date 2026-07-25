#ifndef NETSCOPE_UI_MENU_H
#define NETSCOPE_UI_MENU_H

#include <string>
#include <vector>
#include <functional>

namespace netscope {
namespace ui {

struct MenuItem {
    int key;
    std::string label;
    std::string description;
    std::function<void()> action;
};

class Menu {
public:
    Menu() = default;
    explicit Menu(const std::string& title);

    void SetTitle(const std::string& title);
    void AddItem(int key, const std::string& label,
                 std::function<void()> action,
                 const std::string& description = "");
    void AddSeparator();
    void SetFooter(const std::string& footer);

    int Show();
    void Run();

    Menu(const Menu&) = delete;
    Menu& operator=(const Menu&) = delete;

private:
    void Display();

    std::string title_;
    std::vector<MenuItem> items_;
    std::vector<int> separators_;
    std::string footer_;
};

class ProgressBar {
public:
    ProgressBar(int total, const std::string& label = "");
    void Update(int current);
    void Complete(const std::string& message = "");
    void SetSuffix(const std::string& suffix);

private:
    void Draw(int current);
    int total_;
    int last_drawn_{-1};
    std::string label_;
    std::string suffix_;
};

class Table {
public:
    using Row = std::vector<std::string>;
    using Header = std::vector<std::string>;

    explicit Table(Header header);

    void AddRow(const Row& row);
    void AddSeparator();
    void SetTitle(const std::string& title);

    std::string Render() const;
    void Display() const;

private:
    std::vector<int> CalculateWidths() const;

    Header header_;
    std::vector<Row> rows_;
    std::vector<bool> separators_;
    std::string title_;
};

} // namespace ui
} // namespace netscope

#endif
