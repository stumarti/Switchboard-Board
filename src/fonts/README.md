# Fonts at twice the size

When a layout is shown large, the E1004 draws the viewport's 800 × 600 screen doubled, but its text is not doubled: each character is drawn from a face made at twice the size, at the panel's own resolution (`Doubled::write` in `src/board/frame.cpp`), so it's as crisp as the viewport's text, just bigger.

| This face | Twice the viewport's built-in |
|---|---|
| `AtkinsonHyperlegible_Regular18pt7b.h` | Regular 9pt (its 18 px text, regular and "bold") |
| `AtkinsonHyperlegible_Bold24pt7b.h` | Bold 12pt (24 px) |
| `AtkinsonHyperlegible_Bold84pt7b.h` | Bold 42pt (the big temperature) |

`fine::finerFont` (`src/board/fine.cpp`) picks the face for each built-in one. Text in a theme's own font has no finer face and is doubled instead.

## Making them again

The font is [Atkinson Hyperlegible](https://fonts.google.com/specimen/Atkinson+Hyperlegible) by the Braille Institute, under the SIL Open Font License 1.1 (`OFL.txt`). Use the TTFs from Google Fonts: they have the same metrics as the viewport's built-in faces (the newer ones on GitHub are wider, so the text wouldn't line up).

Adafruit GFX's `fontconvert` (in its `fontconvert/` folder, needs FreeType) at the viewport's 141 dpi:

```sh
fontconvert AtkinsonHyperlegible-Regular.ttf 18 > AtkinsonHyperlegible_Regular18pt7b.h
fontconvert AtkinsonHyperlegible-Bold.ttf 24 > AtkinsonHyperlegible_Bold24pt7b.h
fontconvert AtkinsonHyperlegible-Bold.ttf 84 > AtkinsonHyperlegible_Bold84pt7b.h
```

(with `#define DPI 141` in `fontconvert.c`), then put back the three-line header at the top.
