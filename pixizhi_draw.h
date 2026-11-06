// PixiZhi SpotPear "Soft LED" renderer.
//
// Every screen is drawn here from a plain UiState snapshot, so the exact same
// code runs on the device (into an LVGL canvas buffer) and on a PC (to produce
// preview images). No ESPHome or LVGL types are used in this header.
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace pixizhi {
namespace draw {

struct Rgb {
  uint8_t r, g, b;
};
constexpr Rgb rgb(uint32_t v) { return Rgb{uint8_t(v >> 16), uint8_t(v >> 8), uint8_t(v)}; }
inline Rgb scale(Rgb c, float f) {
  return Rgb{uint8_t(std::min(255.0f, c.r * f)), uint8_t(std::min(255.0f, c.g * f)), uint8_t(std::min(255.0f, c.b * f))};
}

// ---------------------------------------------------------------- palette
// Fully saturated hues only: the panel renders subtle shades poorly.
struct NamedColor {
  const char *name;
  uint32_t rgb;
};
constexpr NamedColor THEME_COLORS[] = {
    {"Red", 0xFF2020},    {"Vermilion", 0xFF4A10}, {"Orange", 0xFF7A00}, {"Amber", 0xFFB000},
    {"Yellow", 0xFFE600}, {"Lime", 0xC8FF00},      {"Chartreuse", 0x80FF00}, {"Green", 0x20FF40},
    {"Spring", 0x00FF90}, {"Mint", 0x00FFC8},      {"Cyan", 0x00F0FF},   {"Sky", 0x00B4FF},
    {"Azure", 0x0080FF},  {"Blue", 0x2050FF},      {"Indigo", 0x5030FF}, {"Violet", 0x8A2BFF},
    {"Purple", 0xB000FF}, {"Magenta", 0xFF00E0},   {"Pink", 0xFF2A8A},   {"White", 0xF0F0F0},
};
constexpr int THEME_COLOR_COUNT = sizeof(THEME_COLORS) / sizeof(THEME_COLORS[0]);
constexpr int THEME_DEFAULT = 3;  // Amber

constexpr Rgb WHITE = rgb(0xF2F2F2), LIGHT = rgb(0xC4C4C4), GREY = rgb(0x8C8C8C), DIM = rgb(0x4A4A4A),
              GHOST = rgb(0x262626), BLACK = rgb(0x000000), OK_GREEN = rgb(0x20FF40), ALARM = rgb(0xFF2A2A);

// ---------------------------------------------------------------- canvas
enum PixelFormat { RGB565 = 0, ARGB8888 = 1, RGB888 = 2 };

struct Canvas {
  uint8_t *data;
  int w, h;
  uint32_t stride;
  PixelFormat fmt;
  bool night;  // every colour becomes red (night mode)

  Rgb get(int x, int y) const {
    const uint8_t *p = data + size_t(y) * stride;
    if (fmt == RGB565) {
      uint16_t v;
      memcpy(&v, p + x * 2, 2);
      return Rgb{uint8_t((v >> 11) << 3), uint8_t(((v >> 5) & 63) << 2), uint8_t((v & 31) << 3)};
    }
    if (fmt == ARGB8888) return Rgb{p[x * 4 + 2], p[x * 4 + 1], p[x * 4]};
    return Rgb{p[x * 3], p[x * 3 + 1], p[x * 3 + 2]};
  }
  void set(int x, int y, Rgb c) {
    if (x < 0 || y < 0 || x >= w || y >= h) return;
    uint8_t *p = data + size_t(y) * stride;
    if (fmt == RGB565) {
      uint16_t v = uint16_t(((c.r >> 3) << 11) | ((c.g >> 2) << 5) | (c.b >> 3));
      memcpy(p + x * 2, &v, 2);
    } else if (fmt == ARGB8888) {
      p[x * 4] = c.b; p[x * 4 + 1] = c.g; p[x * 4 + 2] = c.r; p[x * 4 + 3] = 255;
    } else {
      p[x * 3] = c.r; p[x * 3 + 1] = c.g; p[x * 3 + 2] = c.b;
    }
  }
  Rgb tint(Rgb c) const {
    if (!night) return c;
    return Rgb{std::max({c.r, c.g, c.b}), 0, 0};
  }
  // Brightest-wins blending, so glow halos never darken their neighbours.
  void lighten(int x, int y, Rgb c) {
    if (x < 0 || y < 0 || x >= w || y >= h) return;
    Rgb o = get(x, y);
    set(x, y, Rgb{std::max(o.r, c.r), std::max(o.g, c.g), std::max(o.b, c.b)});
  }
  void clear() {
    for (int y = 0; y < h; y++) memset(data + size_t(y) * stride, 0, fmt == RGB565 ? w * 2 : fmt == ARGB8888 ? w * 4 : w * 3);
    if (fmt == ARGB8888)
      for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) data[size_t(y) * stride + x * 4 + 3] = 255;
  }
  void blit_rgb(int x, int y, Rgb c) { set(x, y, tint(c)); }
};

// One LED: a round core plus a soft halo.
inline void led(Canvas &cv, float cx, float cy, float pitch, Rgb c, bool glow = true) {
  c = cv.tint(c);
  const float core = pitch * 0.42f, halo = core + std::max(1.2f, pitch * 0.38f);
  const float reach = glow ? halo : core;
  const int x0 = int(floorf(cx - reach)), x1 = int(ceilf(cx + reach));
  const int y0 = int(floorf(cy - reach)), y1 = int(ceilf(cy + reach));
  const Rgb soft = scale(c, 0.30f);
  for (int y = y0; y <= y1; y++)
    for (int x = x0; x <= x1; x++) {
      const float dx = x + 0.5f - cx, dy = y + 0.5f - cy, d = sqrtf(dx * dx + dy * dy);
      if (d <= core) cv.lighten(x, y, c);
      else if (d <= core + 0.7f) cv.lighten(x, y, scale(c, 0.65f));
      else if (glow && d <= halo) cv.lighten(x, y, soft);
    }
}

inline void fill_rect(Canvas &cv, int x, int y, int w, int h, Rgb c) {
  c = cv.tint(c);
  for (int yy = std::max(0, y); yy < std::min(cv.h, y + h); yy++)
    for (int xx = std::max(0, x); xx < std::min(cv.w, x + w); xx++) cv.set(xx, yy, c);
}

// ---------------------------------------------------------------- font (5x7)
struct Glyph {
  char c;
  uint8_t w, shift;  // visible width and first lit column inside the 5-wide cell
  uint8_t rows[7];
};
static const Glyph FONT[] = {
    {'0', 5, 0, {14, 17, 19, 21, 25, 17, 14}}, {'1', 5, 0, {4, 12, 4, 4, 4, 4, 14}},
    {'2', 5, 0, {14, 17, 1, 2, 4, 8, 31}},     {'3', 5, 0, {31, 2, 4, 2, 1, 17, 14}},
    {'4', 5, 0, {2, 6, 10, 18, 31, 2, 2}},     {'5', 5, 0, {31, 16, 30, 1, 1, 17, 14}},
    {'6', 5, 0, {6, 8, 16, 30, 17, 17, 14}},   {'7', 5, 0, {31, 1, 2, 4, 8, 8, 8}},
    {'8', 5, 0, {14, 17, 17, 14, 17, 17, 14}}, {'9', 5, 0, {14, 17, 17, 15, 1, 2, 12}},
    {'A', 5, 0, {14, 17, 17, 31, 17, 17, 17}}, {'B', 5, 0, {30, 17, 17, 30, 17, 17, 30}},
    {'C', 5, 0, {14, 17, 16, 16, 16, 17, 14}}, {'D', 5, 0, {30, 17, 17, 17, 17, 17, 30}},
    {'E', 5, 0, {31, 16, 16, 30, 16, 16, 31}}, {'F', 5, 0, {31, 16, 16, 30, 16, 16, 16}},
    {'G', 5, 0, {14, 17, 16, 23, 17, 17, 15}}, {'H', 5, 0, {17, 17, 17, 31, 17, 17, 17}},
    {'I', 3, 1, {14, 4, 4, 4, 4, 4, 14}},      {'J', 5, 0, {7, 2, 2, 2, 18, 18, 12}},
    {'K', 5, 0, {17, 18, 20, 24, 20, 18, 17}}, {'L', 5, 0, {16, 16, 16, 16, 16, 16, 31}},
    {'M', 5, 0, {17, 27, 21, 21, 17, 17, 17}}, {'N', 5, 0, {17, 25, 25, 21, 19, 19, 17}},
    {'O', 5, 0, {14, 17, 17, 17, 17, 17, 14}}, {'P', 5, 0, {30, 17, 17, 30, 16, 16, 16}},
    {'Q', 5, 0, {14, 17, 17, 17, 21, 18, 13}}, {'R', 5, 0, {30, 17, 17, 30, 20, 18, 17}},
    {'S', 5, 0, {15, 16, 16, 14, 1, 1, 30}},   {'T', 5, 0, {31, 4, 4, 4, 4, 4, 4}},
    {'U', 5, 0, {17, 17, 17, 17, 17, 17, 14}}, {'V', 5, 0, {17, 17, 17, 17, 17, 10, 4}},
    {'W', 5, 0, {17, 17, 17, 21, 21, 21, 10}}, {'X', 5, 0, {17, 17, 10, 4, 10, 17, 17}},
    {'Y', 5, 0, {17, 17, 10, 4, 4, 4, 4}},     {'Z', 5, 0, {31, 1, 2, 4, 8, 16, 31}},
    {'.', 2, 1, {0, 0, 0, 0, 0, 12, 12}},      {':', 2, 1, {0, 12, 12, 0, 12, 12, 0}},
    {',', 2, 1, {0, 0, 0, 0, 12, 4, 8}},       {'!', 1, 2, {4, 4, 4, 4, 4, 0, 4}},
    {'?', 5, 0, {14, 17, 1, 2, 4, 0, 4}},      {'/', 5, 0, {1, 2, 2, 4, 8, 8, 16}},
    {'-', 5, 0, {0, 0, 0, 31, 0, 0, 0}},       {'+', 5, 0, {0, 4, 4, 31, 4, 4, 0}},
    {'%', 5, 0, {25, 26, 2, 4, 8, 11, 19}},    {'^', 4, 0, {12, 18, 18, 12, 0, 0, 0}},  // degree sign
    {'>', 5, 0, {0, 4, 2, 31, 2, 4, 0}},       {'<', 5, 0, {0, 4, 8, 31, 8, 4, 0}},
    {'(', 3, 1, {4, 8, 8, 8, 8, 8, 4}},        {')', 3, 1, {4, 2, 2, 2, 2, 2, 4}},
    {'\'', 1, 2, {4, 4, 0, 0, 0, 0, 0}},       {'#', 5, 0, {10, 31, 10, 10, 31, 10, 0}},
    {'&', 5, 0, {12, 18, 20, 8, 21, 18, 13}},  {'=', 5, 0, {0, 0, 31, 0, 31, 0, 0}},
    {'_', 5, 0, {0, 0, 0, 0, 0, 0, 31}},       {'*', 5, 0, {0, 21, 14, 31, 14, 21, 0}},
    {'|', 1, 2, {4, 4, 4, 4, 4, 4, 4}},        {'~', 5, 0, {0, 0, 8, 21, 2, 0, 0}},
    {'@', 2, 1, {0, 0, 0, 12, 12, 0, 0}},  // middle dot (separator)
    {' ', 3, 0, {0, 0, 0, 0, 0, 0, 0}},
};

inline const Glyph *glyph(char c) {
  for (const auto &g : FONT)
    if (g.c == c) return &g;
  for (const auto &g : FONT)
    if (g.c == '?') return &g;
  return &FONT[0];
}

// Upper-case ASCII; degree sign and common Latin accents fold to readable glyphs.
inline std::string fold(const std::string &v) {
  std::string r;
  for (size_t i = 0; i < v.size(); ++i) {
    uint8_t c = v[i];
    if (c < 128) r += char(toupper(c));
    else if (c == 0xC2 && i + 1 < v.size() && uint8_t(v[i + 1]) == 0xB0) { r += '^'; ++i; }
    else if (c == 0xC2 && i + 1 < v.size() && uint8_t(v[i + 1]) == 0xB7) { r += '@'; ++i; }
    else if (c == 0xC3 && i + 1 < v.size()) {
      uint8_t q = uint8_t(v[++i]) | 0x20;
      r += q >= 0xA0 && q <= 0xA5 ? 'A' : q == 0xA7 ? 'C' : q <= 0xAB ? 'E' : q <= 0xAF ? 'I' : q == 0xB1 ? 'N'
           : q <= 0xB6 ? 'O' : q == 0xBF ? 'S' : q <= 0xBC ? 'U' : 'Y';
    } else if (c == 0xE2 && i + 2 < v.size() && uint8_t(v[i + 1]) == 0x86 && uint8_t(v[i + 2]) == 0x92) { r += '>'; i += 2; }
    else {
      r += '?';
      while (i + 1 < v.size() && (uint8_t(v[i + 1]) & 0xC0) == 0x80) ++i;
    }
  }
  return r;
}

inline float text_width(const std::string &s, float pitch) {
  float w = 0;
  for (char c : s) w += (glyph(c)->w + 1) * pitch;
  return s.empty() ? 0 : w - pitch;
}

// Draws centred text with LED dots. Too-wide text is cut with "." or, with
// scroll_ms set, scrolls as a marquee. Returns the drawn width.
inline float text(Canvas &cv, const std::string &raw, float cx, float y, float pitch, Rgb c, float maxw = 200,
                  int32_t scroll_ms = -1, Rgb off = BLACK) {
  std::string s = fold(raw);
  if (text_width(s, pitch) > maxw) {
    if (scroll_ms >= 0) {
      std::string loop = s + "   ";
      int advance = scroll_ms < 1500 ? 0 : (scroll_ms - 1500) / 200;
      size_t off_i = size_t(advance) % loop.size();
      std::string t = (loop + loop).substr(off_i);
      s.clear();
      for (char ch : t) {
        if (text_width(s + ch, pitch) > maxw) break;
        s += ch;
      }
    } else {
      while (!s.empty() && text_width(s + ".", pitch) > maxw) s.pop_back();
      s += ".";
    }
  }
  float x = cx - text_width(s, pitch) / 2;
  const bool unlit = off.r || off.g || off.b;
  for (char ch : s) {
    const Glyph *g = glyph(ch);
    for (int r = 0; r < 7; r++)
      for (int k = 0; k < g->w; k++) {
        bool on = (g->rows[r] >> (4 - k - g->shift)) & 1;
        if (on) led(cv, x + k * pitch + pitch / 2, y + r * pitch + pitch / 2, pitch, c);
        else if (unlit) led(cv, x + k * pitch + pitch / 2, y + r * pitch + pitch / 2, pitch, off, false);
      }
    x += (g->w + 1) * pitch;
  }
  return text_width(s, pitch);
}

// ---------------------------------------------------------------- icons
struct Icon {
  const char *name;
  uint8_t w, h;
  const char *rows[13];
};
static const Icon ICONS[] = {
    // 11x11 menu icons
    {"big_light", 11, 11, {"...XXXXX...", "..XXXXXXX..", ".XXXXXXXXX.", ".XXXXXXXXX.", ".XXXXXXXXX.", "..XXXXXXX..",
                          "...XXXXX...", "...........", "...XXXXX...", "....XXX....", "..........."}},
    {"big_music", 11, 11, {"....XXXXXXX", "....XXXXXXX", "....X.....X", "....X.....X", "....X.....X", "....X.....X",
                          "..XXX...XXX", ".XXXX..XXXX", ".XXXX..XXXX", "..XX....XX.", "..........."}},
    {"big_heat", 11, 11, {".....X.....", "....XX.....", "....XXX....", "...XXXX.X..", "...XXXXXX..", "..XXXXXXXX.",
                         "..XXX.XXXX.", "..XX...XXX.", "..XX...XX..", "...XX.XX...", "....XXX...."}},
    {"big_more", 11, 11, {"XXXX...XXXX", "XXXX...XXXX", "XXXX...XXXX", "XXXX...XXXX", "...........", "...........",
                         "...........", "XXXX...XXXX", "XXXX...XXXX", "XXXX...XXXX", "XXXX...XXXX"}},
    // 9x9 icons (names match those sent by the blueprints)
    {"light", 9, 9, {"..XXXXX..", ".XXXXXXX.", "XXXXXXXXX", "XXXXXXXXX", ".XXXXXXX.", "..XXXXX..", "...XXX...", "...XXX...", "....X...."}},
    {"music", 9, 9, {"...XXXXXX", "...XXXXXX", "...X....X", "...X....X", "...X....X", ".XXX..XXX", "XXXX.XXXX", "XXXX.XXXX", ".XX...XX."}},
    {"flame", 9, 9, {"....X....", "...XX....", "...XXX.X.", "..XXXXXX.", ".XXXXXXXX", ".XXX.XXXX", ".XX...XXX", "..XX.XXX.", "...XXXX.."}},
    {"home", 9, 9, {"....X....", "...XXX...", "..XXXXX..", ".XXXXXXX.", "XXXXXXXXX", ".XX...XX.", ".XX...XX.", ".XX...XX.", ".XX...XX."}},
    {"power", 9, 9, {"....X....", ".X..X..X.", "X...X...X", "X...X...X", "X.......X", "X.......X", ".X.....X.", "..XXXXX..", "........."}},
    {"play", 9, 9, {".XX......", ".XXXX....", ".XXXXXX..", ".XXXXXXXX", ".XXXXXXXX", ".XXXXXX..", ".XXXX....", ".XX......", "........."}},
    {"pause", 9, 9, {".XXX.XXX.", ".XXX.XXX.", ".XXX.XXX.", ".XXX.XXX.", ".XXX.XXX.", ".XXX.XXX.", ".XXX.XXX.", ".XXX.XXX.", "........."}},
    {"prev", 9, 9, {"X.....XX.", "X...XXXX.", "X.XXXXXX.", "XXXXXXXX.", "X.XXXXXX.", "X...XXXX.", "X.....XX.", ".........", "........."}},
    {"next", 9, 9, {".XX.....X", ".XXXX...X", ".XXXXXX.X", ".XXXXXXXX", ".XXXXXX.X", ".XXXX...X", ".XX.....X", ".........", "........."}},
    {"window", 9, 9, {"XXXXXXXXX", "X...X...X", "X...X...X", "X...X...X", "XXXXXXXXX", "X...X...X", "X...X...X", "X...X...X", "XXXXXXXXX"}},
    {"door", 9, 9, {".XXXXXXX.", ".X.....X.", ".X.....X.", ".X.....X.", ".X...X.X.", ".X.....X.", ".X.....X.", ".X.....X.", "XXXXXXXXX"}},
    {"water", 9, 9, {"....X....", "....X....", "...XXX...", "...XXX...", "..XXXXX..", ".XXXXXXX.", ".XXXXXXX.", ".XXXXXXX.", "..XXXXX.."}},
    {"washer", 9, 9, {"XXXXXXXXX", "X.X.....X", "XXXXXXXXX", "X..XXX..X", "X.X...X.X", "X.X...X.X", "X..XXX..X", "X.......X", "XXXXXXXXX"}},
    {"lock", 9, 9, {"..XXXXX..", ".X.....X.", ".X.....X.", "XXXXXXXXX", "XXXXXXXXX", "XXXX.XXXX", "XXXX.XXXX", "XXXXXXXXX", "XXXXXXXXX"}},
    {"motion", 9, 9, {"....XX...", "....XX...", "..XXX....", ".X.XXX...", "...XX.X..", "...XX....", "..X..X...", ".X....X..", "X.....X.."}},
    {"battery", 9, 9, {"...XXX...", "..XXXXX..", "..X...X..", "..XXXXX..", "..XXXXX..", "..XXXXX..", "..XXXXX..", "..XXXXX..", "..XXXXX.."}},
    {"alert", 9, 9, {"....X....", "...XXX...", "...X.X...", "..XX.XX..", "..XX.XX..", ".XXXXXXX.", ".XXX.XXX.", "XXXXXXXXX", "XXXXXXXXX"}},
    {"minus", 9, 9, {".........", ".........", ".........", ".........", "XXXXXXXXX", ".........", ".........", ".........", "........."}},
    {"plus", 9, 9, {"....X....", "....X....", "....X....", "....X....", "XXXXXXXXX", "....X....", "....X....", "....X....", "....X...."}},
    {"palette", 9, 9, {"..XXXXX..", ".XXXXXXX.", "XX.XXX.XX", "XXXXXXXXX", "XX.XXXXX.", "XXXXXX...", ".XXXXX.X.", "..XXXX...", "........."}},
    {"bell", 9, 9, {"....X....", "..XXXXX..", ".XXXXXXX.", ".XXXXXXX.", ".XXXXXXX.", ".XXXXXXX.", "XXXXXXXXX", ".........", "...XXX..."}},
    {"timer", 9, 9, {"...XXX...", "....X....", "..XXXXX..", ".X..X..X.", "X...X...X", "X...XX..X", "X.......X", ".X.....X.", "..XXXXX.."}},
    {"star", 9, 9, {"....X....", "....X....", "...XXX...", "XXXXXXXXX", ".XXXXXXX.", "..XXXXX..", "..XXXXX..", ".XX...XX.", "X.......X"}},
    {"moon", 9, 9, {"..XXX....", ".XXX.....", "XXX......", "XXX......", "XXX......", "XXX.....X", ".XXX..XX.", "..XXXXX..", "...XXX..."}},
    {"bed", 9, 9, {".........", "X........", "X.XX.....", "XXXXXXXXX", "XXXXXXXXX", "XXXXXXXXX", "X.......X", "X.......X", "........."}},
    {"sofa", 9, 9, {".XXXXXXX.", ".X.....X.", ".X.....X.", "XXXXXXXXX", "XX.....XX", "XXXXXXXXX", "XXXXXXXXX", ".X.....X.", "........."}},
    {"vacuum", 9, 9, {"..XXXXX..", ".XXXXXXX.", "XXX...XXX", "XX.....XX", "XX..X..XX", "XX.....XX", "XXX...XXX", ".XXXXXXX.", "..XXXXX.."}},
    {"fan", 9, 9, {"...XX....", "...XXX...", "....XX...", "XX..X..XX", "XXXX.XXXX", "XX..X..XX", "...XX....", "...XXX...", "....XX..."}},
    {"tv", 9, 9, {".X.....X.", "..X...X..", "XXXXXXXXX", "X.......X", "X.......X", "X.......X", "X.......X", "XXXXXXXXX", "...XXX..."}},
    {"garage", 9, 9, {"....X....", "..XXXXX..", "XXXXXXXXX", "X.......X", "XXXXXXXXX", "X.......X", "XXXXXXXXX", "X.......X", "XXXXXXXXX"}},
    {"package", 9, 9, {"..XXXXX..", ".X..X..X.", "XXXXXXXXX", "X...X...X", "X...X...X", "X...X...X", "X...X...X", "X...X...X", "XXXXXXXXX"}},
    {"info", 9, 9, {"....X....", ".........", "...XX....", "....X....", "....X....", "....X....", "....X....", "....X....", "...XXX..."}},
    {"check", 9, 9, {".........", "........X", ".......XX", "......XX.", "X....XX..", "XX..XX...", ".XXXX....", "..XX.....", "........."}},
    {"mail", 9, 9, {".........", "XXXXXXXXX", "XX.....XX", "X.X...X.X", "X..X.X..X", "X...X...X", "X.......X", "XXXXXXXXX", "........."}},
    {"coffee", 9, 9, {".X..X....", "..X..X...", ".X..X....", "XXXXXXX..", "XXXXXXXXX", "XXXXXXX.X", "XXXXXXXXX", ".XXXXX...", "XXXXXXXX."}},
    {"sun", 9, 9, {"X...X...X", ".X..X..X.", "...XXX...", "..XXXXX..", "XXXXXXXXX", "..XXXXX..", "...XXX...", ".X..X..X.", "X...X...X"}},
    {"speaker", 9, 9, {"...X.....", "..XX...X.", "XXXX..X..", "XXXX.X..X", "XXXX.X..X", "XXXX.X..X", "XXXX..X..", "..XX...X.", "...X....."}},
    {"dnd", 9, 9, {"..XXXXX..", ".XXXXXXX.", "XXXXXXXXX", "XXXXXXXXX", "X.......X", "XXXXXXXXX", "XXXXXXXXX", ".XXXXXXX.", "..XXXXX.."}},
    {"phones", 9, 9, {"..XXXXX..", ".X.....X.", "X.......X", "X.......X", "XXX...XXX", "XXX...XXX", "XXX...XXX", "XXX...XXX", "........."}},
    {"hourglass", 9, 9, {"XXXXXXXXX", ".X.....X.", "..X...X..", "...XXX...", "....X....", "...X.X...", "..X.XXX..", ".XXXXXXX.", "XXXXXXXXX"}},
    {"heating", 9, 9, {"X.X.X.X.X", "X.X.X.X.X", "XXXXXXXXX", "X.X.X.X.X", "X.X.X.X.X", "X.X.X.X.X", "XXXXXXXXX", "X.X.X.X.X", "X.......X"}},
    // added in 1.1.3 for the radial menu
    {"unlock", 9, 9, {"..XXXXX..", ".X.....X.", ".X.......", "XXXXXXXXX", "XXXXXXXXX", "XXXX.XXXX", "XXXX.XXXX", "XXXXXXXXX", "XXXXXXXXX"}},
    {"blinds", 9, 9, {"XXXXXXXXX", "X.......X", "XXXXXXXXX", "X.......X", "XXXXXXXXX", "X.......X", "XXXXXXXXX", "....X....", "....X...."}},
    {"camera", 9, 9, {".........", "..XXX....", "XXXXXXXXX", "X..XXX..X", "X.X...X.X", "X.X...X.X", "X..XXX..X", "XXXXXXXXX", "........."}},
    {"car", 9, 9, {".........", "..XXXXX..", ".X.....X.", "X.......X", "XXXXXXXXX", "XX.XXX.XX", "XXXXXXXXX", ".XX...XX.", "........."}},
    {"plug", 9, 9, {"..X...X..", "..X...X..", ".XXXXXXX.", ".XXXXXXX.", ".XXXXXXX.", "..XXXXX..", "...XXX...", "....X....", "....X...."}},
    {"thermo", 9, 9, {"...XXX...", "...X.X...", "...X.X...", "...XXX...", "...XXX...", "..XXXXX..", ".XXXXXXX.", ".XXXXXXX.", "..XXXXX.."}},
    {"snow", 9, 9, {"....X....", ".X..X..X.", "..X.X.X..", "...XXX...", "XXXXXXXXX", "...XXX...", "..X.X.X..", ".X..X..X.", "....X...."}},
    {"shield", 9, 9, {"XXXXXXXXX", "X.......X", "X...X...X", "X..XXX..X", ".X.XXX.X.", ".X..X..X.", "..X...X..", "...X.X...", "....X...."}},
    {"bolt", 9, 9, {"....XXX..", "...XXX...", "..XXX....", ".XXXXXXX.", "....XXX..", "...XXX...", "..XXX....", ".XX......", ".X......."}},
    {"plant", 9, 9, {"...X.X...", "..XX.XX..", ".XXX.XXX.", "..XX.XX..", "....X....", ".XXXXXXX.", ".XXXXXXX.", "..XXXXX..", "..XXXXX.."}},
    {"scene", 9, 9, {"......X..", ".....XXX.", "......X..", ".....X...", "....X....", "...X.....", "..X......", ".X.......", "X........"}},
    {"away", 9, 9, {"XXXXX....", "X...X....", "X...X..X.", "X..XXXXXX", "X...X..X.", "X...X....", "X...X....", "X...X....", "XXXXX...."}},
    {"wifi", 9, 7, {".XXXXXXX.", "X.......X", "..XXXXX..", ".X.....X.", "...XXX...", "..X...X..", "....X...."}},
    {"bat_h", 9, 5, {"XXXXXXXX.", "X......XX", "X......XX", "X......XX", "XXXXXXXX."}},
    {"chev_up", 7, 4, {"...X...", "..XXX..", ".XX.XX.", "XX...XX"}},
    {"chev_down", 7, 4, {"XX...XX", ".XX.XX.", "..XXX..", "...X..."}},
    // 11x9 weather
    {"w_sun", 11, 9, {".....X.....", "..X.....X..", "....XXX....", "...XXXXX...", "X.XXXXXXX.X", "...XXXXX...", "....XXX....", "..X.....X..", ".....X....."}},
    {"w_moon", 11, 9, {"...XXXX....", ".XXXXX.....", ".XXXX......", "XXXX.......", "XXXX.......", "XXXXX.....X", ".XXXXXX.XX.", "..XXXXXXXX.", "....XXXX..."}},
    {"w_part", 11, 9, {"..X........", "X...XX.....", "..XXXX.....", ".XXXX.XXX..", "XXXX.XXXXX.", "..X.XXXXXXX", ".X.XXXXXXXX", "...XXXXXXXX", "....XXXXXX."}},
    {"w_cloud", 11, 9, {"...........", "....XXX....", "..XXXXXXX..", ".XXXXXXXXX.", "XXXXXXXXXXX", "XXXXXXXXXXX", ".XXXXXXXXX.", "...........", "..........."}},
    {"w_rain", 11, 9, {"....XXX....", "..XXXXXXX..", ".XXXXXXXXX.", "XXXXXXXXXXX", ".XXXXXXXXX.", "...........", "..X...X...X", ".X...X...X.", "X...X...X.."}},
    {"w_snow", 11, 9, {"....XXX....", "..XXXXXXX..", ".XXXXXXXXX.", "XXXXXXXXXXX", ".XXXXXXXXX.", "...........", ".X...X...X.", "...........", "...X...X..."}},
    {"w_bolt", 11, 9, {"....XXX....", "..XXXXXXX..", ".XXXXXXXXX.", "XXXXXXXXXXX", ".XXXXXXXXX.", "....XXX....", "...XXXXXX..", "......XX...", ".....X....."}},
    {"w_fog", 11, 9, {"...........", "XXXXXXXXXXX", "...........", ".XXXXXXXXX.", "...........", "XXXXXXXXXXX", "...........", "..XXXXXXX..", "..........."}},
    {"w_wind", 11, 9, {"......XX...", "XXXXXX..X..", "........X..", "XXXXXXXXX..", "...........", "XXXXXXX....", ".......X...", "......X....", "..........."}},
};

inline const Icon *find_icon(const std::string &raw) {
  // Aliases for names used by the blueprints and older firmware.
  std::string n = raw == "lightbulb" ? "light" : raw == "fire" ? "flame" : raw == "night" ? "moon"
                  : raw == "movie" ? "tv" : raw == "status" ? "info" : raw == "climate" ? "flame" : raw;
  for (const auto &i : ICONS)
    if (n == i.name) return &i;
  for (const auto &i : ICONS)
    if (std::string("alert") == i.name) return &i;
  return &ICONS[0];
}

inline void icon(Canvas &cv, const std::string &name, float cx, float y, float pitch, Rgb c) {
  const Icon *ic = find_icon(name);
  const float x0 = cx - ic->w * pitch / 2;
  for (int r = 0; r < ic->h; r++)
    for (int k = 0; k < ic->w; k++)
      if (ic->rows[r][k] == 'X') led(cv, x0 + k * pitch + pitch / 2, y + r * pitch + pitch / 2, pitch, c);
}

// Ring of LEDs around the edge; `fraction` of them lit, clockwise from 12 o'clock.
inline void edge_ring(Canvas &cv, float fraction, Rgb on, int n = 60, float radius = 114, float pitch = 4.2f) {
  const int lit = int(roundf(std::max(0.0f, std::min(1.0f, fraction)) * n));
  for (int k = 0; k < n; k++) {
    float a = (-90.0f + k * 360.0f / n) * 0.0174532925f;
    led(cv, 120 + cosf(a) * radius, 120 + sinf(a) * radius, pitch, k < lit ? on : GHOST, k < lit);
  }
}

// Open dial (270 degrees, gap at the bottom) used by brightness/volume/temperature.
inline void dial(Canvas &cv, float fraction, Rgb on) {
  const int n = 40, lit = int(roundf(std::max(0.0f, std::min(1.0f, fraction)) * n));
  for (int k = 0; k < n; k++) {
    float a = (135.0f + k * 270.0f / (n - 1)) * 0.0174532925f;
    led(cv, 120 + cosf(a) * 109, 120 + sinf(a) * 109, 5.0f, k < lit ? on : GHOST, k < lit);
  }
}

// ---------------------------------------------------------------- state
enum Screen {
  S_CLOCK = 0, S_MENU = 1, S_SLOT = 2, S_HOME = 3, S_HEAT_LIST = 4, S_LIST = 5, S_MORE = 6, S_STATUS = 7,
  S_MODES = 9, S_MUSIC_OPT = 10, S_ALERTS = 11, S_TIMER = 12, S_QUICK = 14, S_PLANE = 15, S_FORECAST = 16, S_NOW_PLAYING = 17, S_RADIAL = 18,
};

struct SlotView {
  std::string eid, label, state, title, artist, action, unit, modes, presets, sources, repeat;
  bool available = false, stale = true, dimmable = false, color = false, white = false, muted = false, shuffle = false;
  int pending = 0, features = 0, bri = 0;
  float expected = 0, vol = 0, tnow = -1000, ttarget = 0, tmin = 5, tmax = 30, step = 0.5f;
  bool is(const char *prefix) const { return eid.rfind(prefix, 0) == 0; }
  bool light() const { return is("light.") || is("switch."); }
  bool music() const { return is("media_player."); }
  bool climate() const { return is("climate."); }
};

struct StatusView {
  std::string label, icon, value, target;
  bool active = false;
  int priority = 0;
};

struct UiState {
  int screen = S_CLOCK;
  Rgb accent = rgb(THEME_COLORS[THEME_DEFAULT].rgb);
  uint32_t scroll_ms = 0, anim_ms = 0;  // ms since the screen opened; free-running animation clock
  bool ha_online = true, wifi_online = true, api_online = true;
  std::string message;

  // clock
  bool time_valid = true;
  int hour = 12, minute = 0;
  bool weather_ok = false;
  std::string weather_cond;
  float weather_temp = 0;
  int weather_frame = 0;
  int status_row = -1;  // status shown instead of weather on the clock
  // hourly forecast (next hours, from the blueprint)
  int fc_n = 0;
  int fc_hour[5] = {};
  float fc_temp[5] = {};
  int fc_rain[5] = {};  // precipitation probability, %
  std::string fc_cond[5];
  std::string desk_temp, desk_rh;
  bool music_line = false;
  std::string music_player;  // player name on the song-change banner
  std::string music_title, music_artist;
  int alert_count = 0;
  float battery = NAN;
  bool charging = false, battery_ring = true, battery_low = false;
  int brightness_feedback = -1;  // % while turning the rim on the clock

  // home data
  SlotView slots[6];
  int cur_slot = -1, category = 0, list_offset = 0, status_detail = -1;
  StatusView status[6];
  bool dragging = false;
  float dial_value = 0;
  int volume_limit = 100;

  // notifications
  std::string n_title, n_msg, n_icon, n_age;
  uint32_t n_color = 0xFFB000;
  bool n_urgent = false;
  int n_count = 0, n_line = 0;
  int n_lines_total = 0;  // out: wrapped line count, so touch can clamp scrolling

  // timer
  int timer_left = 300, timer_set_min = 5;
  bool timer_running = false, timer_done = false;

  // radial action menu
  std::string radial_labels[8], radial_keys[8];
  int radial_sel = -1;

  // quick settings
  int q_brightness = 100, q_volume = 50;
  bool q_night = false, q_dnd = false, q_usb = false;

  // plane
  std::string p_model, p_flight, p_airline, p_origin, p_dest, p_callsign;
  bool p_takeoff = true;
  float p_remaining = 1;  // fraction of the 15 s left
  int p_index = 0, p_count = 1;

  // now-playing artwork (RGB565, native byte order)
  const uint8_t *art = nullptr;
  int art_w = 0, art_h = 0;
  bool art_led = true;
  float progress = -1;  // 0..1 track progress, -1 unknown
};

// Why HA counts as offline: no Wi-Fi, no API link, or linked but the blueprint sends nothing.
inline const char *offline_text(const UiState &s) {
  return !s.wifi_online ? "WIFI OFFLINE" : !s.api_online ? "HA OFFLINE" : "NO HA DATA";
}

inline std::string fmt(float v, int decimals) {
  char b[24];
  snprintf(b, sizeof(b), decimals >= 2 ? "%.2f" : decimals == 1 ? "%.1f" : "%.0f", v);
  return b;
}

// ---------------------------------------------------------------- screens
inline void footer(Canvas &cv, const UiState &s) { icon(cv, "home", 120, 210, 2, GREY); }

inline void top_title(Canvas &cv, const UiState &s, const std::string &t) { text(cv, t, 120, 24, 3, s.accent, 170); }

// HA weather condition -> 11x9 weather icon name.
inline const char *weather_icon(const std::string &c) {
  return c == "sunny" ? "w_sun" : c == "clear-night" ? "w_moon" : c == "partlycloudy" ? "w_part"
         : (c == "rainy" || c == "pouring") ? "w_rain" : (c == "lightning" || c == "lightning-rainy") ? "w_bolt"
         : (c == "snowy" || c == "snowy-rainy" || c == "hail") ? "w_snow" : c == "fog" ? "w_fog"
         : (c == "windy" || c == "windy-variant") ? "w_wind" : "w_cloud";
}

inline void draw_clock(Canvas &cv, const UiState &s) {
  const Rgb a = s.accent, off = scale(s.accent, 0.16f);
  if (s.battery_ring && !s.charging && !std::isnan(s.battery))
    edge_ring(cv, s.battery / 100.0f, s.battery <= 20 ? ALARM : s.battery <= 50 ? rgb(0xFFB000) : OK_GREEN);
  // top row: connection problem, status, or weather
  if (!s.ha_online) {
    text(cv, offline_text(s), 120, 46, 3, GREY, 170);
  } else if (s.status_row >= 0 && s.status_row < 6 && s.status[s.status_row].active) {
    const auto &st = s.status[s.status_row];
    icon(cv, st.icon, 52, 44, 3, a);
    text(cv, st.label, 136, 46, 3, WHITE, 140, int32_t(s.scroll_ms));
  } else if (s.weather_ok) {
    const std::string &c = s.weather_cond;
    const char *k = weather_icon(c);
    std::string t = fmt(s.weather_temp, 0) + "^";
    const float ip = 3.2f, iw = 11 * ip;  // 11x9 icon, centred on the 3x text
    float tw = text_width(t, 3), total = iw + 8 + tw;
    float x0 = 120 - total / 2;
    icon(cv, k, x0 + iw / 2 + (s.weather_frame % 2), 39 + (s.weather_frame >= 2 ? 1 : 0), ip, a);
    text(cv, t, x0 + iw + 8 + tw / 2, 46, 3, a);
  }
  // time
  char hm[8];
  if (s.time_valid) snprintf(hm, sizeof(hm), "%02d:%02d", s.hour, s.minute);
  else snprintf(hm, sizeof(hm), "--:--");
  text(cv, hm, 120, 82, 8, a, 230, -1, off);
  // second line
  if (s.timer_running) {
    char b[24];
    snprintf(b, sizeof(b), "TIMER %02d:%02d", s.timer_left / 60, s.timer_left % 60);
    text(cv, b, 120, 152, 3, WHITE, 196);
  } else {
    auto num = [](const std::string &v, int d) -> std::string {
      char *end = nullptr;
      float f = strtof(v.c_str(), &end);
      if (end == v.c_str() || !std::isfinite(f)) return "--";
      return fmt(f, d);
    };
    text(cv, num(s.desk_temp, 1) + "^ " + num(s.desk_rh, 0) + "%", 120, 152, 3, WHITE, 196);
  }
  // bottom: brightness feedback, alerts, low battery or music
  if (s.brightness_feedback >= 0) {
    text(cv, std::to_string(s.brightness_feedback) + "%", 120, 186, 3, a);
  } else if (s.alert_count > 0) {
    icon(cv, "bell", 120 - text_width(std::to_string(s.alert_count) + " NEW", 2) / 2 - 12, 186, 2, a);
    text(cv, std::to_string(s.alert_count) + " NEW", 132, 188, 2, a);
  } else if (s.battery_low) {
    text(cv, "LOW BATTERY", 120, 188, 2, ALARM);
  } else if (s.music_line) {
    icon(cv, "music", 48, 184, 2, a);
    text(cv, s.music_title.empty() ? s.music_artist : s.music_title, 134, 188, 2, LIGHT, 124, int32_t(s.scroll_ms));
  }
}

inline void draw_menu(Canvas &cv, const UiState &s) {
  struct It { const char *ic, *label; float cx, y; Rgb c; };
  const It items[] = {{"big_light", "LIGHTS", 74, 46, s.accent}, {"big_music", "MUSIC", 166, 46, s.accent},
                      {"big_heat", "HEATING", 74, 120, s.accent}, {"big_more", "MORE", 166, 120, WHITE}};
  for (const auto &i : items) {
    icon(cv, i.ic, i.cx, i.y, 4, i.c);
    text(cv, i.label, i.cx, i.y + 50, 2, WHITE, 90);
  }
  footer(cv, s);
}

inline void draw_more(Canvas &cv, const UiState &s) {
  top_title(cv, s, "MORE");
  struct It { const char *ic, *label; float cx, y; };
  const It items[] = {{"bell", "ALERTS", 76, 56}, {"timer", "TIMER", 164, 56}, {"scene", "ACTIONS", 76, 122}, {"info", "STATUS", 164, 122}};
  for (const auto &i : items) {
    icon(cv, i.ic, i.cx, i.y, 4, s.accent);
    text(cv, i.label, i.cx, i.y + 42, 2, WHITE, 90);
  }
  footer(cv, s);
}

inline void list_arrows(Canvas &cv, const UiState &s, bool up, bool down) {
  if (up) icon(cv, "chev_up", 120, 48, 2, GREY);
  if (down) icon(cv, "chev_down", 120, 194, 2, s.accent);
}

inline void draw_heat_list(Canvas &cv, const UiState &s, std::vector<int> rows, int offset) {
  top_title(cv, s, "HEATING");
  for (int r = 0; r < 2 && offset + r < int(rows.size()); r++) {
    const auto &q = s.slots[rows[offset + r]];
    const float y = 60 + r * 67;
    text(cv, q.label, 120, y, 2, WHITE, 182, int32_t(s.scroll_ms));
    std::string v = q.available ? fmt(q.tnow, 1) + "^ > " + fmt(q.ttarget, 1) + "^" : "UNAVAILABLE";
    text(cv, v, 120, y + 20, 3, q.available ? s.accent : GREY, 196);
    if (q.action == "heating") icon(cv, "flame", 120, y + 46, 1.6f, s.accent);
  }
  if (rows.empty()) text(cv, "ADD THERMOSTATS", 120, 110, 2, GREY);
  list_arrows(cv, s, offset > 0, offset + 2 < int(rows.size()));
  footer(cv, s);
}

inline void draw_list(Canvas &cv, const UiState &s, const std::vector<int> &rows, int offset) {
  top_title(cv, s, s.category == 0 ? "LIGHTS" : "MUSIC");
  for (int r = 0; r < 2 && offset + r < int(rows.size()); r++) {
    const auto &q = s.slots[rows[offset + r]];
    const float y = 64 + r * 62;
    text(cv, q.label, 120, y, 2, WHITE, 182, int32_t(s.scroll_ms));
    std::string st = q.available ? q.state : "UNAVAILABLE";
    text(cv, st, 120, y + 20, 3, q.available && (st == "on" || st == "playing") ? s.accent : GREY, 180);
  }
  if (rows.empty()) text(cv, "NO ENTITIES", 120, 110, 2, GREY);
  list_arrows(cv, s, offset > 0, offset + 2 < int(rows.size()));
  footer(cv, s);
}

inline void draw_home(Canvas &cv, const UiState &s) {
  top_title(cv, s, "HOME");
  int lights_on = 0;
  for (const auto &q : s.slots)
    if (q.is("light.") && q.available && q.state == "on") lights_on++;
  icon(cv, "light", 52, 60, 2.4f, s.accent);
  text(cv, std::to_string(lights_on) + " LIGHTS ON", 136, 64, 2, WHITE, 140);
  int r = 0;
  for (const auto &q : s.slots)
    if (q.climate() && r < 2) {
      std::string v = q.label + " " + (q.available && q.tnow > -100 ? fmt(q.tnow, 1) + "^" : "--");
      icon(cv, "flame", 52, 96 + r * 31, 2.4f, s.accent);
      text(cv, v, 136, 100 + r * 31, 2, WHITE, 140);
      r++;
    }
  int active = -1;
  for (int i = 0; i < 6; i++)
    if (s.status[i].active && (active < 0 || s.status[i].priority > s.status[active].priority)) active = i;
  for (int x = 52; x < 190; x += 6) led(cv, x, 170, 2, GHOST, false);
  if (active >= 0) {
    icon(cv, s.status[active].icon, 52, 180, 2.4f, s.accent);
    text(cv, s.status[active].label, 136, 184, 2, s.accent, 140);
  } else
    text(cv, "ALL CLEAR", 120, 184, 2, GREY);
  footer(cv, s);
}

inline void draw_status(Canvas &cv, const UiState &s) {
  int q = s.status_detail;
  if (q < 0 || q >= 6) { footer(cv, s); return; }
  const auto &st = s.status[q];
  icon(cv, st.icon, 120, 30, 5, s.accent);
  text(cv, st.label, 120, 84, 3, WHITE, 196, int32_t(s.scroll_ms));
  text(cv, st.value, 120, 116, 4, st.active ? s.accent : WHITE, 190);
  if (!st.target.empty()) text(cv, "TAP TO OPEN", 120, 166, 2, GREY);
  footer(cv, s);
}

inline std::vector<std::string> split_bar(const std::string &a) {
  std::vector<std::string> out;
  size_t start = 0;
  while (start < a.size()) {
    size_t p = a.find('|', start);
    auto v = a.substr(start, p == std::string::npos ? p : p - start);
    if (!v.empty()) out.push_back(v);
    if (p == std::string::npos) break;
    start = p + 1;
  }
  return out;
}

inline void draw_options(Canvas &cv, const UiState &s, const char *title, const std::vector<std::string> &rows) {
  top_title(cv, s, title);
  for (int r = 0; r < 3; r++) {
    int idx = s.list_offset + r;
    if (idx < int(rows.size())) text(cv, rows[idx], 120, 70 + r * 39, 2, WHITE, 176);
  }
  if (rows.empty()) text(cv, "NOTHING HERE", 120, 110, 2, GREY);
  list_arrows(cv, s, s.list_offset > 0, s.list_offset + 3 < int(rows.size()));
  footer(cv, s);
}

inline void draw_artwork(Canvas &cv, const UiState &s, int cx, int y, int size) {
  if (s.art == nullptr || s.art_w <= 0) return;
  auto px = [&](int x, int yy) -> Rgb {
    uint16_t v;
    memcpy(&v, s.art + (size_t(yy) * s.art_w + x) * 2, 2);
    return Rgb{uint8_t((v >> 11) << 3), uint8_t(((v >> 5) & 63) << 2), uint8_t((v & 31) << 3)};
  };
  if (s.art_led) {
    const int n = 22;
    const float pitch = float(size) / n;
    for (int j = 0; j < n; j++)
      for (int i = 0; i < n; i++) {
        // average the source block for each LED
        int sx0 = i * s.art_w / n, sx1 = std::max(sx0 + 1, (i + 1) * s.art_w / n);
        int sy0 = j * s.art_h / n, sy1 = std::max(sy0 + 1, (j + 1) * s.art_h / n);
        uint32_t r = 0, g = 0, b = 0, c = 0;
        for (int yy = sy0; yy < sy1; yy += 2)
          for (int xx = sx0; xx < sx1; xx += 2) { Rgb p = px(xx, yy); r += p.r; g += p.g; b += p.b; c++; }
        Rgb avg{uint8_t(r / c), uint8_t(g / c), uint8_t(b / c)};
        led(cv, cx - size / 2.0f + i * pitch + pitch / 2, y + j * pitch + pitch / 2, pitch, avg, false);
      }
  } else {
    // full artwork, nearest-neighbour scaled, with rounded corners
    const int r = 10;
    for (int yy = 0; yy < size; yy++)
      for (int xx = 0; xx < size; xx++) {
        int dx = std::max({0, r - xx, xx - (size - 1 - r)}), dy = std::max({0, r - yy, yy - (size - 1 - r)});
        if (dx * dx + dy * dy > r * r) continue;
        cv.blit_rgb(cx - size / 2 + xx, y + yy, px(xx * s.art_w / size, yy * s.art_h / size));
      }
  }
}

// Sample the RGB565 artwork at normalised coordinates (0..1).
inline Rgb art_at(const UiState &s, float u, float v) {
  const int x = std::min(s.art_w - 1, std::max(0, int(u * s.art_w))), y = std::min(s.art_h - 1, std::max(0, int(v * s.art_h)));
  uint16_t p;
  memcpy(&p, s.art + (size_t(y) * s.art_w + x) * 2, 2);
  return Rgb{uint8_t((p >> 11) << 3), uint8_t(((p >> 5) & 63) << 2), uint8_t((p & 31) << 3)};
}

// Artwork across the upper half, fading out at the bottom: the middle band of
// the cover, either smooth or as LED dots (Artwork Style). `dim` darkens it
// (paused).
inline void art_hero(Canvas &cv, const UiState &s, float dim = 1.0f) {
  if (s.art_led) {
    const float p = 7.0f;
    for (int j = 0; j < 17; j++)
      for (int i = 0; i < 34; i++) {
        const float x = i * p + p / 2 + 1, y = j * p + p / 2, dx = x - 120, dy = y - 120;
        if (dx * dx + dy * dy > 116 * 116) continue;
        const float fade = j < 13 ? 1.0f : 1.0f - (j - 12) * 0.2f;
        led(cv, x, y, p, scale(art_at(s, x / 240, 0.2f + y / 240), fade * dim), false);
      }
  } else {
    for (int y = 0; y < 124; y++)
      for (int x = 0; x < 240; x++) {
        const float fade = y < 96 ? 1.0f : 1.0f - (y - 96) / 28.0f;
        cv.blit_rgb(x, y, scale(art_at(s, x / 240.0f, 0.2f + y / 240.0f), fade * dim));
      }
  }
}

// Shared by the song-change banner and the player screen: artwork (or a note),
// scrolling title and artist, and the player name small at the bottom.
inline void music_hero(Canvas &cv, const UiState &s, const std::string &title, const std::string &artist,
                       const std::string &player, float dim = 1.0f) {
  const Rgb a = s.accent;
  if (s.art != nullptr && s.art_w > 0) art_hero(cv, s, dim);
  else icon(cv, "music", 120, 40, 6, scale(a, dim));
  text(cv, title, 120, 130, 3, WHITE, 172, int32_t(s.scroll_ms));
  text(cv, artist, 120, 157, 2, a, 168, int32_t(s.scroll_ms));
  text(cv, player, 120, 197, 1.6f, GREY, 120);
}

inline void draw_slot(Canvas &cv, const UiState &s) {
  if (s.cur_slot < 0 || s.cur_slot >= 6) return;
  const auto &q = s.slots[s.cur_slot];
  const Rgb a = s.accent;
  if (q.light()) {
    float pct = s.dragging ? s.dial_value : (q.pending == 2 ? q.expected : q.bri * 100.0f / 255.0f);
    bool on = q.pending == 1 ? q.expected > 0 : q.pending == 2 ? true : q.state == "on";
    if (q.dimmable) dial(cv, on ? pct / 100 : 0, a);
    std::string v = !q.available ? "--" : !on ? "OFF" : q.dimmable ? fmt(pct, 0) + "%" : "ON";
    text(cv, v, 120, 62, 7, on ? WHITE : GREY, 190);
    text(cv, q.label, 120, 124, 2, LIGHT, 172, int32_t(s.scroll_ms));
    icon(cv, "power", 90, 162, 4, on ? a : WHITE);
    if (q.color || q.white) icon(cv, "palette", 150, 162, 4, a);
  } else if (q.music()) {
    // Same layout as the song-change banner plus the volume/progress ring. The
    // lower half is the control surface: swipe left/right for previous/next,
    // double-tap to play/pause. The rim sets the volume.
    float vol = s.dragging ? s.dial_value : q.pending == 3 ? q.expected : q.vol * 100;
    bool playing = q.pending == 5 ? q.expected > 0 : q.state == "playing";
    music_hero(cv, s, q.title.empty() ? q.state : q.title, q.artist, q.label, playing ? 1.0f : 0.45f);
    edge_ring(cv, vol / 100, a, 60, 115, 3.6f);
  } else {
    float target = s.dragging ? s.dial_value : q.pending == 4 ? q.expected : q.ttarget;
    dial(cv, (target - q.tmin) / std::max(1.0f, q.tmax - q.tmin), a);
    text(cv, q.label, 120, 40, 2, GREY, 160);
    text(cv, q.available ? fmt(target, q.step < 0.5f ? 2 : 1) + "^" : "--", 120, 66, 6, WHITE, 196);
    text(cv, "NOW " + (q.available && q.tnow > -100 ? fmt(q.tnow, 1) + "^" : std::string("--")), 120, 118, 2, a, 178);
    icon(cv, "minus", 68, 156, 3, WHITE);
    icon(cv, "plus", 172, 150, 3, WHITE);
    icon(cv, "flame", 120, 146, 3, q.action == "heating" ? a : DIM);
    text(cv, q.action == "heating" ? "HEATING" : q.state, 120, 184, 2, q.action == "heating" ? a : GREY, 150);
  }
  if (!q.available) text(cv, q.stale ? "OFFLINE" : "UNAVAILABLE", 120, 196, 2, GREY, 170);
  if (!q.music()) footer(cv, s);
}

// Word-wraps to lines of at most `cols` characters.
inline std::vector<std::string> wrap(std::string msg, size_t cols) {
  msg = fold(msg);
  std::vector<std::string> lines;
  while (!msg.empty()) {
    size_t cut = std::min(cols, msg.size());
    if (cut < msg.size()) {
      auto sp = msg.rfind(' ', cut);
      if (sp != std::string::npos && sp > 0) cut = sp;
    }
    lines.push_back(msg.substr(0, cut));
    msg.erase(0, cut);
    while (!msg.empty() && msg[0] == ' ') msg.erase(0, 1);
  }
  return lines;
}

inline void draw_alerts(Canvas &cv, UiState &s) {
  if (s.n_count == 0) {
    icon(cv, "check", 120, 70, 5, s.accent);
    text(cv, "ALL CLEAR", 120, 130, 3, WHITE);
    footer(cv, s);
    return;
  }
  const Rgb ink = s.n_urgent ? ALARM : rgb(s.n_color);
  std::string head = std::to_string(1) + "/" + std::to_string(s.n_count) + (s.n_age.empty() ? "" : " @ " + s.n_age);
  text(cv, head, 120, 22, 2, GREY, 170);
  icon(cv, s.n_icon, 120, 42, 3, ink);
  text(cv, s.n_title, 120, 76, 2.6f, WHITE, 196, int32_t(s.scroll_ms));
  auto lines = wrap(s.n_msg, 14);
  s.n_lines_total = int(lines.size());
  int off = std::max(0, std::min(std::max(0, int(lines.size()) - 3), s.n_line));
  for (int r = 0; r < 3 && off + r < int(lines.size()); r++) text(cv, lines[off + r], 120, 106 + r * 20, 2, LIGHT, 190);
  if (off + 3 < int(lines.size())) icon(cv, "chev_down", 120, 168, 2, ink);
  text(cv, s.n_urgent ? "HOLD TO DISMISS" : "TAP TO DISMISS", 120, 190, 2, ink, 180);
}

inline void draw_timer(Canvas &cv, const UiState &s) {
  const bool adjustable = !s.timer_running && !s.timer_done && s.timer_left == s.timer_set_min * 60;
  const float total = std::max(1, s.timer_set_min * 60);
  edge_ring(cv, s.timer_done ? 1.0f : s.timer_left / total, s.timer_done ? ALARM : s.accent);
  text(cv, s.timer_done ? "TIME IS UP" : s.timer_running ? "RUNNING" : adjustable ? "SET MINUTES" : "PAUSED", 120, 36, 2,
       s.timer_done ? ALARM : GREY, 170);
  char b[16];
  int left = std::max(0, s.timer_left);
  snprintf(b, sizeof(b), "%02d:%02d", left / 60, left % 60);
  text(cv, b, 120, 70, 6, WHITE, 210, -1, rgb(0x1C1C1C));
  if (adjustable) {
    icon(cv, "minus", 64, 140, 3, s.accent);
    icon(cv, "plus", 176, 134, 3, s.accent);
    text(cv, "MIN", 120, 138, 2, GREY);
  } else
    text(cv, "HOLD TO RESET", 120, 138, 2, GREY);
  text(cv, s.timer_done ? "STOP" : s.timer_running ? "PAUSE" : adjustable ? "START" : "RESUME", 120, 174, 4, s.accent, 160);
  footer(cv, s);
}

inline void draw_quick(Canvas &cv, const UiState &s) {
  // status line: battery, Wi-Fi, HA link, time
  const Rgb bat = s.charging ? OK_GREEN : s.battery_low ? ALARM : OK_GREEN;
  icon(cv, "bat_h", 60, 32, 2, bat);
  if (!std::isnan(s.battery)) text(cv, std::to_string(int(s.battery)) + "%", 98, 30, 2, WHITE);
  icon(cv, "wifi", 140, 28, 2, s.wifi_online ? WHITE : DIM);
  led(cv, 166, 35, 7, s.ha_online ? OK_GREEN : ALARM);
  char hm[8];
  if (s.time_valid) snprintf(hm, sizeof(hm), "%02d:%02d", s.hour, s.minute);
  else snprintf(hm, sizeof(hm), "--:--");
  text(cv, hm, 120, 50, 2, GREY);
  struct Tile { const char *ic; std::string val; bool on; };
  const Tile tiles[] = {{"sun", std::to_string(s.q_brightness) + "%", true},
                        {"speaker", std::to_string(s.q_volume) + "%", s.q_volume > 0},
                        {"moon", s.q_night ? "ON" : "OFF", s.q_night},
                        {"dnd", "DND", s.q_dnd},
                        {s.q_usb ? "phones" : "speaker", s.q_usb ? "USB" : "SPK", true},
                        {"hourglass", s.timer_running ? std::to_string((s.timer_left + 59) / 60) + "M" : "TIMER", true}};
  for (int i = 0; i < 6; i++) {
    const float cx = 60 + (i % 3) * 60, y = 78 + (i / 3) * 66;
    const Rgb c = tiles[i].on ? s.accent : GREY;
    for (int k = 0; k < 18; k++) {
      float a = k * 20.0f * 0.0174532925f;
      led(cv, cx + cosf(a) * 25, y + 16 + sinf(a) * 25, 2.4f, tiles[i].on ? scale(s.accent, 0.35f) : GHOST, false);
    }
    icon(cv, tiles[i].ic, cx, y + 5, 2.8f, c);
    text(cv, tiles[i].val, cx, y + 46, 2, tiles[i].on ? c : GREY, 58);
  }
}

// Departure-board tag: top-down jet (nose up; flipped for arrivals) plus DEPARTING / ARRIVING.
static const char *PLANE_TOP[9] = {"....X....", "...XXX...", "...XXX...", ".XXXXXXX.", "XXXXXXXXX",
                                   "...XXX...", "...XXX...", "..XXXXX..", ".XX.X.XX."};
inline void draw_plane_badge(Canvas &cv, const UiState &s, Rgb a) {
  const char *lbl = s.p_takeoff ? "DEPARTING" : "ARRIVING";
  const float w = text_width(lbl, 2);
  const float x = 120 - (w + 30) / 2.0f;
  for (int r = 0; r < 9; r++) {
    const char *row = PLANE_TOP[s.p_takeoff ? r : 8 - r];
    for (int k = 0; k < 9; k++) if (row[k] == 'X') led(cv, x + k * 2.6f + 1.3f, 38 + r * 2.6f + 1.3f, 2.6f, a);
  }
  text(cv, lbl, x + 30 + w / 2.0f, 41, 2, a, 190);
}

inline void draw_plane(Canvas &cv, const UiState &s) {
  const Rgb a = s.accent;
  edge_ring(cv, s.p_remaining, a);
  draw_plane_badge(cv, s, a);
  text(cv, s.p_model.empty() ? "UNKNOWN" : s.p_model, 120, 74, 3, a, 196, int32_t(s.scroll_ms));
  text(cv, s.p_flight.empty() ? "NO CALLSIGN" : s.p_flight, 120, 104, 3, WHITE, 196);
  text(cv, s.p_airline, 120, 134, 2, GREY, 190, int32_t(s.scroll_ms));
  std::string route = (s.p_origin.empty() ? "---" : s.p_origin) + " > " + (s.p_dest.empty() ? "---" : s.p_dest);
  text(cv, route, 120, 156, 3, a, 196);
  text(cv, s.p_callsign, 120, 188, 2, GREY, 150);  // registration
  if (s.p_count > 1)
    for (int p = 0; p < s.p_count && p < 8; p++) led(cv, 120 - (std::min(s.p_count, 8) - 1) * 5 + p * 10, 210, 4, p == s.p_index ? WHITE : DIM, p == s.p_index);
}

// Next hours as LED columns: hour, icon, temperature, a bar scaled across the
// shown range, and rain chance as blue dots (one per 20%).
inline void draw_forecast(Canvas &cv, const UiState &s) {
  const Rgb a = s.accent, rain = rgb(0x3AA0FF);
  text(cv, "NEXT HOURS", 120, 26, 2, GREY);
  const int n = std::min(s.fc_n, 5);
  if (n == 0) { text(cv, "NO FORECAST", 120, 112, 3, GREY, 190); return; }
  float lo = s.fc_temp[0], hi = s.fc_temp[0];
  for (int i = 1; i < n; i++) { lo = std::min(lo, s.fc_temp[i]); hi = std::max(hi, s.fc_temp[i]); }
  const float dx = 34, x0 = 120 - (n - 1) * dx / 2;
  for (int i = 0; i < n; i++) {
    const float x = x0 + i * dx;
    char h[4];
    snprintf(h, sizeof(h), "%02d", s.fc_hour[i] % 24);
    text(cv, h, x, 46, 2, i == 0 ? WHITE : GREY);
    icon(cv, weather_icon(s.fc_cond[i]), x, 64, 2.4f, a);
    text(cv, fmt(s.fc_temp[i], 0), x, 92, 2, WHITE);
    // temperature bar: 2..8 dots, bottom-aligned
    const int lit = hi - lo < 0.5f ? 5 : 2 + int(roundf((s.fc_temp[i] - lo) / (hi - lo) * 6));
    for (int k = 0; k < 8; k++) led(cv, x, 158 - k * 6.2f, 4.6f, k < lit ? a : GHOST, k < lit);
    const int drops = (s.fc_rain[i] + 10) / 20;
    for (int k = 0; k < 5; k++) led(cv, x, 172 + k * 5.4f, 3.6f, k < drops ? rain : GHOST, k < drops);
  }
}

// Shown for 10 s when the song changes.
inline void draw_now_playing(Canvas &cv, const UiState &s) {
  music_hero(cv, s, s.music_title, s.music_artist, s.music_player);
}

// Radial action menu over the clock: up to 8 actions on a ring, clockwise from
// the top. The selected one is white with its edge segment lit; its label shows
// in the middle.
inline void draw_radial(Canvas &cv, const UiState &s) {
  const Rgb a = s.accent;
  int count = 0;
  for (int k = 0; k < 8; k++) if (!s.radial_labels[k].empty()) count++;
  for (int k = 0; k < 60; k++) {
    const float ang = (-90.0f + k * 6.0f) * 0.0174532925f;
    const int slice = ((k + 4) % 60) / 7.5f;  // 7.5 LEDs per slice, centred on its icon
    const bool on = slice == s.radial_sel && !s.radial_labels[slice].empty();
    led(cv, 120 + cosf(ang) * 114, 120 + sinf(ang) * 114, 4.2f, on ? a : GHOST, on);
  }
  for (int k = 0; k < 8; k++) {
    if (s.radial_labels[k].empty()) continue;
    const float ang = (-90.0f + k * 45.0f) * 0.0174532925f;
    const bool sel = k == s.radial_sel;
    const float p = sel ? 3.2f : 2.6f;
    icon(cv, s.radial_keys[k], 120 + cosf(ang) * 80, 120 + sinf(ang) * 80 - 4.5f * p, p, sel ? WHITE : a);
  }
  if (count == 0) {
    text(cv, "NO ACTIONS", 120, 104, 2, GREY, 120);
    text(cv, "SET IN BLUEPRINT", 120, 124, 1.6f, DIM, 120);
  } else if (s.radial_sel >= 0 && !s.radial_labels[s.radial_sel].empty()) {
    text(cv, s.radial_labels[s.radial_sel], 120, 112, 2, WHITE, 104);
  } else {
    led(cv, 120, 120, 6, DIM, false);
  }
}

// Draws the whole frame for the current screen.
inline void draw_frame(Canvas &cv, UiState &s) {
  cv.clear();
  std::vector<int> rows;
  switch (s.screen) {
    case S_CLOCK: draw_clock(cv, s); break;
    case S_MENU: draw_menu(cv, s); break;
    case S_MORE: draw_more(cv, s); break;
    case S_HOME: draw_home(cv, s); break;
    case S_HEAT_LIST:
      for (int i = 0; i < 6; i++) if (s.slots[i].climate()) rows.push_back(i);
      draw_heat_list(cv, s, rows, std::max(0, std::min(std::max(0, int(rows.size()) - 2), s.list_offset)));
      break;
    case S_LIST:
      for (int i = 0; i < 6; i++)
        if (s.category == 0 ? s.slots[i].light() : s.slots[i].music()) rows.push_back(i);
      draw_list(cv, s, rows, std::max(0, std::min(std::max(0, int(rows.size()) - 2), s.list_offset)));
      break;
    case S_STATUS: draw_status(cv, s); break;
    case S_SLOT: draw_slot(cv, s); break;
    case S_MODES: {
      std::vector<std::string> c;
      if (s.cur_slot >= 0 && s.cur_slot < 6) { c = split_bar(s.slots[s.cur_slot].modes); auto p = split_bar(s.slots[s.cur_slot].presets); c.insert(c.end(), p.begin(), p.end()); }
      draw_options(cv, s, "HEAT MODE", c);
      break;
    }
    case S_MUSIC_OPT: {
      std::vector<std::string> c;
      if (s.cur_slot >= 0 && s.cur_slot < 6) {
        const auto &q = s.slots[s.cur_slot];
        if (q.features & 8) c.push_back(q.muted ? "UNMUTE" : "MUTE");
        if (q.features & 32768) c.push_back(q.shuffle ? "SHUFFLE OFF" : "SHUFFLE ON");
        if (q.features & 262144) c.push_back("REPEAT " + q.repeat);
        if (q.features & 2048) { auto v = split_bar(q.sources); c.insert(c.end(), v.begin(), v.end()); }
      }
      draw_options(cv, s, "OPTIONS", c);
      break;
    }
    case S_ALERTS: draw_alerts(cv, s); break;
    case S_TIMER: draw_timer(cv, s); break;
    case S_QUICK: draw_quick(cv, s); break;
    case S_PLANE: draw_plane(cv, s); break;
    case S_FORECAST: draw_forecast(cv, s); break;
    case S_NOW_PLAYING: draw_now_playing(cv, s); break;
    case S_RADIAL: draw_radial(cv, s); break;
  }
  if (s.screen != S_CLOCK && s.screen != S_QUICK && s.screen != S_PLANE && s.screen != S_NOW_PLAYING && !s.ha_online)
    text(cv, offline_text(s), 120, 15, 1.6f, GREY, 110);
  if (!s.message.empty()) {
    fill_rect(cv, 30, 194, 180, 16, BLACK);
    text(cv, s.message, 120, 196, 1.6f, WHITE, 176);
  }
}

}  // namespace draw
}  // namespace pixizhi
