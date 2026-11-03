// Renders every screen with the real firmware drawing code (pixizhi_draw.h)
// into 240x240 PPM files, using fictional sample data.
//
//   g++ -std=c++17 -O2 -I.. render_previews.cpp -o render_previews
//   ./render_previews OUT_DIR [ACCENT_INDEX] [night]
#include <cstdio>
#include <string>
#include "pixizhi_draw.h"

using namespace pixizhi::draw;

static void save(const std::string &path, const uint8_t *buf) {
  FILE *f = fopen(path.c_str(), "wb");
  fprintf(f, "P6\n240 240\n255\n");
  fwrite(buf, 1, 240 * 240 * 3, f);
  fclose(f);
}

// Fictional cover art: dusk gradient, sun and hills (RGB565).
static std::vector<uint8_t> cover(int n) {
  std::vector<uint8_t> out(n * n * 2);
  for (int y = 0; y < n; y++)
    for (int x = 0; x < n; x++) {
      float t = float(y) / n;
      int r = 40 + int(200 * t), g = 20 + int(60 * t), b = 120 - int(60 * t);
      float dx = x - n * 0.5f, dy = y - n * 0.45f;
      if (dx * dx + dy * dy < (n * 0.2f) * (n * 0.2f)) { r = 255; g = 200; b = 80; }
      if (y > n * 0.72f - (x % (n / 3)) * 0.25f) { r = 30; g = 10; b = 50; }
      if (y > n * 0.86f) { r = 12; g = 4; b = 24; }
      uint16_t v = uint16_t(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
      memcpy(&out[(y * n + x) * 2], &v, 2);
    }
  return out;
}

int main(int argc, char **argv) {
  std::string dir = argc > 1 ? argv[1] : ".";
  int accent = argc > 2 ? atoi(argv[2]) : THEME_DEFAULT;
  bool night = argc > 3 && std::string(argv[3]) == "night";
  static uint8_t buf[240 * 240 * 3];
  Canvas cv{buf, 240, 240, 240 * 3, RGB888, night};

  UiState base;
  base.accent = rgb(THEME_COLORS[accent].rgb);
  base.hour = 12; base.minute = 48;
  base.weather_ok = true; base.weather_cond = "sunny"; base.weather_temp = 16;
  base.desk_temp = "21.5"; base.desk_rh = "45";
  base.battery = 72; base.battery_ring = true;
  base.scroll_ms = 0;
  auto &l = base.slots[0];
  l.eid = "light.living_room"; l.label = "Living Room"; l.state = "on"; l.available = true; l.stale = false;
  l.dimmable = true; l.color = true; l.bri = 166;
  auto &d = base.slots[1];
  d.eid = "light.desk_lamp"; d.label = "Desk Lamp"; d.state = "off"; d.available = true; d.stale = false;
  auto &m = base.slots[2];
  m.eid = "media_player.kitchen"; m.label = "Kitchen"; m.state = "playing"; m.available = true; m.stale = false;
  m.title = "Midnight City"; m.artist = "Neon Harbor"; m.vol = 0.4f; m.features = 1 | 16 | 32 | 4 | 512;
  auto &c1 = base.slots[3];
  c1.eid = "climate.living_room"; c1.label = "Living Room"; c1.state = "heat"; c1.available = true; c1.stale = false;
  c1.tnow = 21.5f; c1.ttarget = 22.0f; c1.action = "heating"; c1.features = 1;
  auto &c2 = base.slots[4];
  c2.eid = "climate.bedroom"; c2.label = "Bedroom"; c2.state = "heat"; c2.available = true; c2.stale = false;
  c2.tnow = 21.7f; c2.ttarget = 22.0f; c2.action = "idle";
  base.status[0] = {"Front Door", "door", "Open", "", true, 5};

  struct Shot { const char *name; int screen; };
  const Shot shots[] = {{"clock", S_CLOCK}, {"menu", S_MENU}, {"quick", S_QUICK}, {"light", S_SLOT},
                        {"music", S_SLOT}, {"music-led", S_SLOT}, {"plane", S_PLANE}, {"plane-landing", S_PLANE},
                        {"heating", S_HEAT_LIST}, {"thermostat", S_SLOT}, {"alerts", S_ALERTS}, {"timer", S_TIMER},
                        {"status", S_STATUS}, {"more", S_MORE}, {"actions", S_PIZZA}, {"lights", S_LIST}};
  auto art = cover(104);
  for (const auto &sh : shots) {
    UiState s = base;
    s.screen = sh.screen;
    std::string n = sh.name;
    if (n == "clock") { s.music_line = true; s.music_title = "Midnight City"; }
    if (n == "light") s.cur_slot = 0;
    if (n == "music" || n == "music-led") { s.cur_slot = 2; s.art = art.data(); s.art_w = s.art_h = 104; s.art_led = n == "music-led"; s.progress = 0.42f; }
    if (n == "thermostat") s.cur_slot = 3;
    if (n == "quick") { s.q_brightness = 75; s.q_volume = 40; s.q_usb = true; }
    if (n == "plane" || n == "plane-landing") {
      s.p_model = "A320neo"; s.p_flight = "DLH2034"; s.p_airline = "Lufthansa"; s.p_origin = "FRA"; s.p_dest = "MUC";
      s.p_callsign = "D-AINA"; s.p_remaining = 0.66f; s.p_count = 3; s.p_index = 0; s.p_takeoff = n == "plane";
      if (n == "plane-landing") { s.p_model = "B787-9"; s.p_flight = "UAL988"; s.p_airline = "United Airlines"; s.p_origin = "IAD"; s.p_dest = "FRA"; s.p_callsign = "N24976"; s.p_index = 1; s.p_remaining = 0.25f; }
    }
    if (n == "alerts") { s.n_count = 1; s.n_title = "Laundry ready"; s.n_msg = "Your washing has finished. Please empty the machine."; s.n_icon = "washer"; s.n_age = "12M AGO"; }
    if (n == "timer") { s.timer_set_min = 5; s.timer_left = 272; s.timer_running = true; }
    if (n == "status") s.status_detail = 0;
    if (n == "actions") {
      const char *labels[] = {"Cozy", "Music", "Good night", "All off"}, *keys[] = {"light", "music", "moon", "power"};
      for (int k = 0; k < 4; k++) { s.pizza_labels[k] = labels[k]; s.pizza_keys[k] = keys[k]; }
      s.pizza_pages = 2;
    }
    if (n == "lights") s.category = 0;
    draw_frame(cv, s);
    save(dir + "/" + n + ".ppm", buf);
  }
  return 0;
}
