# 3. UNDERSTANDING THE DISPLAY

YIP-BOI OS uses consistent visual conventions across every screen. Learn these once and you'll always know what's happening.

## 3.1 Screen Layout

Every screen follows the same structure:

```
 +---------- SCREEN TITLE ---------+  <- Row 0: title bar
 |                                  |
 |        Content area              |  <- Rows 1-6: content
 |       (screen-specific)          |
 |                                  |
 |                                  |
 |                                  |
 +|----------------- HH:MM:SS-----+  <- Row 7: status bar
  ^                   ^
  spinner             clock
```

The display is a 40-column by 8-row character grid. Columns 0 and 39 are reserved for the border. Row 0 is the title bar. Row 7 is the status bar. That leaves rows 1-6 (228 cells) for content.

## 3.2 The Spinnyboi

Bottom-left corner, row 7, column 1. The **spinner cursor** (a.k.a. "the spinnyboi") cycles through four characters: `| / - \`

This is the system's heartbeat. If it's spinning, the desktop companion is connected and actively driving the display. If it stops, the connection has been lost. Restart the companion software.

<!-- IMAGE: spinner closeup.jpg — The spinner cursor ("spinnyboi") in the status bar. -->

## 3.3 The "C" Clearing Flag

On screens with graphs (NET, HEART), the spinner may be temporarily replaced by a **"C"** character. This means the display is busy **clearing old graph data** — for example, after switching network interfaces.

When "C" disappears and the spinner resumes, clearing is complete and fresh data is being drawn. Think of it as the CRT's way of saying "one moment, please."

<!-- IMAGE: C glyph closeup.jpg — The "C" clearing flag replacing the spinner. -->

## 3.4 Inverted Text = Touchable

> **IMPORTANT:** This is the most important rule in YIP-BOI OS: **if text appears in inverted video (light background, dark characters), you can touch it.** Inverted text always means "this is a button."

Normal text: green on black. Inverted text: black on green. The convention is universal — Home screen tiles, Config settings, StayPutVR body parts, network interface names. If it's inverted, it responds to touch.

When you press an inverted element, it briefly **flashes to normal** (un-inverts) as visual feedback confirming your input.

<!-- IMAGE: inverted text closeup.jpg — Inverted text (touchable) vs. normal text on the CRT. -->

## 3.5 The Clock

The bottom-right corner displays a **24-hour clock** (`HH:MM:SS`). It updates once per second and is always visible on every screen.

## 3.6 Page Indicators

Screens with multiple pages display arrow glyphs on the left border:

- ↑ on the left border = ML will page up
- ↓ on the left border = BL will page down
- Page number (e.g., "1/2") appears on the status bar

## 3.7 Progress Bars

Several screens use block-character progress bars. Filled portions use solid blocks, empty portions use light shade characters. Bars are **self-healing** — periodic refresh re-stamps the background and redraws only the filled portion, so shrinking values are automatically corrected.
