// =============================================================================
// fine.h — what the E1004 draws at its own resolution when a layout is shown
// large (twice the size): text in the panel's faces made at twice the size,
// and icons — those fetched from the server come at twice the size; the
// built-in art (colour pixel art at the screen's size) is enlarged with
// Scale2x, which keeps its edges sharp rather than blocky.
// =============================================================================
#pragma once
#include <stdint.h>
#include <vector>
#include "board/frame.h"
#include "render/draw.h"

namespace fine {

// The panel's built-in face `f` at twice the size, or nullptr (a theme's
// own font, which has no finer version: it's doubled).
const GFXfont* finerFont(const GFXfont* f);

// An icon twice the size by Scale2x (EPX): each pixel becomes four, each
// taking a neighbour's value where two neighbours agree, so diagonals and
// curves stay smooth. Same format (4-bit or 1-bit) as `ic`; `buf` holds it.
draw::Icon scale2x(const draw::Icon& ic, std::vector<uint8_t>& buf);

// draw::fine.icon for a Doubled screen: the icon at the panel's resolution.
void icon(frame::Doubled& screen, const draw::Icon& ic, int x, int y, uint16_t tint, draw::Mode mode);

}  // namespace fine
