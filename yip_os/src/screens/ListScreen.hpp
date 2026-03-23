#pragma once

#include "Screen.hpp"

namespace YipOS {

class ListScreen : public Screen {
public:
    using Screen::Screen;

    void Render() override;
    void RenderDynamic() override;
    bool OnInput(const std::string& key) override;

protected:
    // Subclasses must implement these
    virtual int ItemCount() const = 0;
    virtual void RenderRow(int i, bool selected) = 0;
    virtual void WriteSelectionMark(int i, bool selected) = 0;

    // Override to show a message when the list is empty (default: "No data")
    virtual void RenderEmpty();

    // Override to handle TR key (default: no-op, returns false)
    virtual bool OnSelect(int index);

    // Pagination helpers
    int PageCount() const;
    int ItemCountOnPage() const;
    int GlobalIndex(int local_index) const { return page_ * ROWS_PER_PAGE + local_index; }

    void RenderRows();
    void RenderPageIndicators();
    void RefreshCursorRows(int old_cursor, int new_cursor);

    int page_ = 0;
    int cursor_ = 0;
    static constexpr int ROWS_PER_PAGE = 6;
    static constexpr int SEL_WIDTH = 3;
};

} // namespace YipOS
