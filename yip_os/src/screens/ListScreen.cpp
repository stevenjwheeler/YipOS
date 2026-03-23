#include "ListScreen.hpp"
#include "app/PDAController.hpp"
#include "app/PDADisplay.hpp"
#include "core/Glyphs.hpp"
#include <algorithm>
#include <cstdio>

namespace YipOS {

using namespace Glyphs;

int ListScreen::PageCount() const {
    int n = ItemCount();
    if (n == 0) return 1;
    return (n + ROWS_PER_PAGE - 1) / ROWS_PER_PAGE;
}

int ListScreen::ItemCountOnPage() const {
    if (ItemCount() == 0) return 0;
    int base = page_ * ROWS_PER_PAGE;
    int remaining = ItemCount() - base;
    return std::min(remaining, ROWS_PER_PAGE);
}

void ListScreen::RenderEmpty() {
    display_.WriteText(2, 3, "No data");
}

bool ListScreen::OnSelect(int /*index*/) {
    return false;
}

void ListScreen::RenderRows() {
    if (ItemCount() == 0) {
        RenderEmpty();
        return;
    }

    int items = ItemCountOnPage();
    for (int i = 0; i < items; i++) {
        RenderRow(i, i == cursor_);
    }
}

void ListScreen::RenderPageIndicators() {
    if (ItemCount() > 0) {
        int global_idx = page_ * ROWS_PER_PAGE + cursor_ + 1;
        int total = ItemCount();
        char pos[12];
        std::snprintf(pos, sizeof(pos), "%d/%d", global_idx, total);
        display_.WriteText(5, 7, pos);
    }

    if (PageCount() <= 1) return;

    if (page_ > 0) {
        display_.WriteGlyph(0, 3, G_UP);
    }
    if (page_ < PageCount() - 1) {
        display_.WriteGlyph(0, 5, G_DOWN);
    }
}

void ListScreen::RefreshCursorRows(int old_cursor, int new_cursor) {
    // Insert at the front of any in-progress buffered render:
    //  1. Selection marks (immediate visual feedback)
    //  2. Full selected row (fills in around the marks)
    //  3. Then remaining buffered content resumes for other rows
    // EndPriority deduplicates so stale versions of these cells are pruned.
    display_.BeginPriority();
    if (old_cursor != new_cursor && old_cursor >= 0 && old_cursor < ItemCountOnPage()) {
        WriteSelectionMark(old_cursor, false);
    }
    if (new_cursor >= 0 && new_cursor < ItemCountOnPage()) {
        WriteSelectionMark(new_cursor, true);
        RenderRow(new_cursor, true);
    }
    RenderPageIndicators();
    display_.EndPriority();
}

void ListScreen::Render() {
    RenderFrame(name);
    RenderRows();
    RenderPageIndicators();
    RenderStatusBar();
}

void ListScreen::RenderDynamic() {
    RenderRows();
    RenderPageIndicators();
    RenderClock();
    RenderCursor();
}

bool ListScreen::OnInput(const std::string& key) {
    if (ItemCount() == 0) return false;

    if (key == "Joystick") {
        int items = ItemCountOnPage();
        if (items == 0) return true;
        int old_cursor = cursor_;
        cursor_ = (cursor_ + 1) % items;
        RefreshCursorRows(old_cursor, cursor_);
        return true;
    }

    if (key == "TR") {
        int idx = page_ * ROWS_PER_PAGE + cursor_;
        return OnSelect(idx);
    }

    if (key == "ML" && PageCount() > 1 && page_ > 0) {
        page_--;
        cursor_ = 0;
        pda_.StartRender(this);
        return true;
    }
    if (key == "BL" && PageCount() > 1 && page_ < PageCount() - 1) {
        page_++;
        cursor_ = 0;
        pda_.StartRender(this);
        return true;
    }

    return false;
}

} // namespace YipOS
