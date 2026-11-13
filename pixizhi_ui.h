// PixiZhi SpotPear shared UI constants and helpers.
// Included from pixizhi.yaml (esphome: includes). Keep this header free of
// ESPHome id() references so it can also be compiled on the host for tests.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace pixizhi {

// Values stored in the `overlay` global.
enum Overlay : int {
  OVERLAY_NONE = 0,
  OVERLAY_NOTIFICATIONS = 1,
  OVERLAY_TIMER = 2,
};

// Values stored in the `ui_view` global.
enum View : int {
  VIEW_STANDBY = 0,      // clock
  VIEW_MENU = 1,         // Lights / Music / Heating / More
  VIEW_SLOT = 2,         // one light, player or thermostat
  VIEW_HEATING_HOME = 3,
  VIEW_HEATING_LIST = 4,
  VIEW_CATEGORY_LIST = 5,
  VIEW_MORE = 6,
  VIEW_STATUS_DETAIL = 7,
  VIEW_COLORS = 8,       // colour / white picker of a light
  VIEW_CLIMATE_MODES = 9,
  VIEW_MUSIC_OPTIONS = 10,
  VIEW_QUICK = 14,       // quick settings (swipe down on the clock)
  VIEW_PLANE = 15,       // what's-that-plane
  VIEW_FORECAST = 16,    // hourly forecast (tap the weather on the clock)
  VIEW_NOW_PLAYING = 17, // song-change banner
  VIEW_RADIAL = 18,      // radial action menu (long-press the clock)
};

// Special `cur_slot` values.
constexpr int SLOT_NONE = -1;   // standby clock
constexpr int SLOT_COUNT = 6;

constexpr int RADIAL_SLICES = 8;
// Blueprint action N (1-based) -> ring position, clockwise from the top.
constexpr int RADIAL_POS[RADIAL_SLICES] = {0, 2, 4, 6, 1, 3, 5, 7};

// A slot or the HA link is considered stale after this long without data.
constexpr uint32_t HA_STALE_MS = 45000;

// Notification persistence: records separated by RS, fields by US.
constexpr char NQ_RECORD_SEP = '\x1e';
constexpr char NQ_FIELD_SEP = '\x1f';
constexpr size_t NQ_STORE_SIZE = 1536;
constexpr size_t NQ_MAX = 5;

struct StoredNotification {
  std::string title, msg, icon;
  uint32_t color = 0;
  bool urgent = false;
  uint32_t epoch = 0;  // UTC seconds, 0 when the clock was not valid
};

inline std::string strip_separators(std::string v, size_t max_len) {
  for (auto &c : v)
    if (c == NQ_RECORD_SEP || c == NQ_FIELD_SEP) c = ' ';
  if (v.size() > max_len) v.resize(max_len);
  return v;
}

// Serialize into a fixed buffer; oldest-first entries that do not fit are dropped
// from the end (normal entries are stored after urgent ones by queue order).
inline void nq_serialize(const std::vector<StoredNotification> &items, char *out, size_t cap) {
  std::string s;
  for (const auto &n : items) {
    std::string rec = strip_separators(n.title, 48) + NQ_FIELD_SEP + strip_separators(n.msg, 160) + NQ_FIELD_SEP +
                      strip_separators(n.icon, 16) + NQ_FIELD_SEP + std::to_string(n.color) + NQ_FIELD_SEP +
                      (n.urgent ? "1" : "0") + NQ_FIELD_SEP + std::to_string(n.epoch) + NQ_RECORD_SEP;
    if (s.size() + rec.size() + 1 > cap) break;
    s += rec;
  }
  size_t n = s.size() < cap ? s.size() : cap - 1;
  for (size_t i = 0; i < n; i++) out[i] = s[i];
  out[n] = '\0';
}

inline std::vector<StoredNotification> nq_deserialize(const char *in, size_t cap) {
  std::vector<StoredNotification> items;
  std::string s;
  for (size_t i = 0; i < cap && in[i] != '\0'; i++) s += in[i];
  size_t start = 0;
  while (start < s.size() && items.size() < NQ_MAX) {
    size_t end = s.find(NQ_RECORD_SEP, start);
    if (end == std::string::npos) break;
    std::vector<std::string> f;
    size_t a = start;
    while (a <= end) {
      size_t b = s.find(NQ_FIELD_SEP, a);
      if (b == std::string::npos || b > end) b = end;
      f.push_back(s.substr(a, b - a));
      a = b + 1;
    }
    if (f.size() == 6 && !f[0].empty()) {
      StoredNotification n;
      n.title = f[0];
      n.msg = f[1];
      n.icon = f[2];
      n.color = (uint32_t) strtoul(f[3].c_str(), nullptr, 10);
      n.urgent = f[4] == "1";
      n.epoch = (uint32_t) strtoul(f[5].c_str(), nullptr, 10);
      items.push_back(n);
    }
    start = end + 1;
  }
  return items;
}

// "NOW", "12M AGO", "3H AGO", "2D AGO"; empty when unknown.
inline std::string age_label(uint32_t then, uint32_t now) {
  if (then == 0 || now == 0 || now < then) return "";
  uint32_t d = now - then;
  if (d < 60) return "NOW";
  if (d < 3600) return std::to_string(d / 60) + "M AGO";
  if (d < 86400) return std::to_string(d / 3600) + "H AGO";
  return std::to_string(d / 86400) + "D AGO";
}

// Kitchen timer remaining seconds from an absolute end time.
inline int timer_remaining(uint32_t end_epoch, uint32_t now_epoch) {
  if (end_epoch == 0 || now_epoch == 0) return -1;
  return now_epoch >= end_epoch ? 0 : (int) (end_epoch - now_epoch);
}

// Compare dotted versions ("0.9.10" > "0.9.9"); a leading 'v' is ignored.
inline int compare_versions(std::string a, std::string b) {
  if (!a.empty() && (a[0] == 'v' || a[0] == 'V')) a.erase(0, 1);
  if (!b.empty() && (b[0] == 'v' || b[0] == 'V')) b.erase(0, 1);
  size_t i = 0, j = 0;
  while (i < a.size() || j < b.size()) {
    long x = 0, y = 0;
    while (i < a.size() && a[i] != '.') { if (a[i] >= '0' && a[i] <= '9') x = x * 10 + (a[i] - '0'); else break; i++; }
    while (j < b.size() && b[j] != '.') { if (b[j] >= '0' && b[j] <= '9') y = y * 10 + (b[j] - '0'); else break; j++; }
    if (x != y) return x < y ? -1 : 1;
    while (i < a.size() && a[i] != '.') i++;
    while (j < b.size() && b[j] != '.') j++;
    if (i < a.size()) i++;
    if (j < b.size()) j++;
  }
  return 0;
}

}  // namespace pixizhi
