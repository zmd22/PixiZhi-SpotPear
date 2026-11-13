// Host tests for the pure helpers in pixizhi_ui.h and pixizhi_draw.h.
//
//   g++ -std=c++17 -Wall -I.. test_helpers.cpp -o test_helpers && ./test_helpers
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "pixizhi_ui.h"
#include "pixizhi_draw.h"

static int failures = 0;
#define CHECK(cond) \
  do { if (!(cond)) { printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); failures++; } } while (0)

using namespace pixizhi;

static void test_notifications() {
  std::vector<StoredNotification> in(2);
  in[0].title = "Door"; in[0].msg = "Front door\x1e open"; in[0].icon = "door"; in[0].color = 0xFF0000; in[0].urgent = true; in[0].epoch = 1700000000;
  in[1].title = "Laundry"; in[1].msg = std::string(400, 'x'); in[1].icon = "washer";
  char buf[NQ_STORE_SIZE];
  nq_serialize(in, buf, sizeof(buf));
  auto out = nq_deserialize(buf, sizeof(buf));
  CHECK(out.size() == 2);
  CHECK(out[0].title == "Door" && out[0].urgent && out[0].color == 0xFF0000 && out[0].epoch == 1700000000);
  CHECK(out[0].msg == "Front door  open");  // separators are replaced, not stored
  CHECK(out[1].msg.size() == 160 && !out[1].urgent);

  // Records that do not fit are dropped whole; the buffer stays terminated.
  std::vector<StoredNotification> many(40, in[1]);
  nq_serialize(many, buf, sizeof(buf));
  CHECK(strlen(buf) < sizeof(buf));
  CHECK(nq_deserialize(buf, sizeof(buf)).size() == NQ_MAX);

  char empty[NQ_STORE_SIZE] = {};
  CHECK(nq_deserialize(empty, sizeof(empty)).empty());
  CHECK(nq_deserialize("garbage without separators", 26).empty());
}

static void test_time_helpers() {
  CHECK(age_label(0, 100).empty());
  CHECK(age_label(100, 0).empty());
  CHECK(age_label(200, 100).empty());
  CHECK(age_label(100, 130) == "NOW");
  CHECK(age_label(100, 100 + 12 * 60) == "12M AGO");
  CHECK(age_label(100, 100 + 3 * 3600) == "3H AGO");
  CHECK(age_label(100, 100 + 2 * 86400) == "2D AGO");
  CHECK(timer_remaining(0, 100) == -1);
  CHECK(timer_remaining(100, 0) == -1);
  CHECK(timer_remaining(160, 100) == 60);
  CHECK(timer_remaining(100, 160) == 0);
}

static void test_versions() {
  CHECK(compare_versions("1.1.4", "1.1.3") == 1);
  CHECK(compare_versions("0.9.9", "0.9.10") == -1);
  CHECK(compare_versions("v1.2", "1.2.0") == 0);
  CHECK(compare_versions("1.10", "1.9.9") == 1);
}

static void test_fold() {
  using draw::fold;
  CHECK(fold("Midnight City") == "MIDNIGHT CITY");
  CHECK(fold("21.5\xC2\xB0") == "21.5^");                                   // degree sign
  CHECK(fold("Don\xE2\x80\x99t Stop") == "DON'T STOP");                     // right single quote
  CHECK(fold("\xE2\x80\x9CHeroes\xE2\x80\x9D") == "\"HEROES\"");            // curly double quotes
  CHECK(fold("A \xE2\x80\x93 B \xE2\x80\x94 C\xE2\x80\xA6") == "A - B - C...");  // dashes, ellipsis
  CHECK(fold("Caf\xC3\xA9 M\xC3\xBCnchen Stra\xC3\x9F" "e") == "CAFE MUNCHEN STRASE");
  CHECK(fold("\xC5\x81\xC3\xB3\x64\xC5\xBA") == "LODZ");                     // Polish
  CHECK(fold("\xC4\x8C\x65sk\xC3\xA1 \xC5\x99\x65ka") == "CESKA REKA");      // Czech
  CHECK(fold("\xC5\x9Ei\xC5\x9F\x6C\x69 \xC4\x9F") == "SISLI G");           // Turkish
  CHECK(fold("FRA \xE2\x86\x92 MUC") == "FRA > MUC");
  CHECK(fold("\xE6\x97\xA5\xE6\x9C\xAC") == "??");                           // unsupported scripts: one ? each
  CHECK(fold("\xC5") == "?");                                               // truncated sequence does not overrun
}

static void test_swatches() {
  using namespace draw;
  for (int i = 0; i < SWATCH_COUNT; i++) {
    const Swatch w = swatch(i);
    CHECK(swatch_at(int(roundf(w.x)), int(roundf(w.y))) == i);  // every swatch can be hit at its centre
    const float dx = w.x - 120, dy = w.y - 120;
    CHECK(dx * dx + dy * dy < 116 * 116);                        // and sits inside the round screen
    CHECK((w.kelvin != 0) == (i >= SWATCH_HUES + SWATCH_PASTELS));
  }
  CHECK(swatch_at(120, 210) == -1);  // the home icon stays free
  CHECK(hsv_rgb(0, 1) == 0xFF0000 && hsv_rgb(120, 1) == 0x00FF00 && hsv_rgb(240, 1) == 0x0000FF);
}

// Every screen must draw without touching memory outside the canvas.
static void test_render() {
  using namespace draw;
  static uint8_t buf[240 * 240 * 3 + 64];
  memset(buf, 0xAB, sizeof(buf));
  Canvas cv{buf + 32, 240, 240, 240 * 3, RGB888, false};
  const int screens[] = {S_CLOCK, S_MENU, S_SLOT, S_HOME, S_HEAT_LIST, S_LIST, S_MORE, S_STATUS, S_COLORS, S_MODES,
                         S_MUSIC_OPT, S_ALERTS, S_TIMER, S_QUICK, S_PLANE, S_FORECAST, S_NOW_PLAYING, S_RADIAL};
  for (int night = 0; night < 2; night++)
    for (int sc : screens)
      for (int slot = -1; slot < 6; slot++) {
        UiState s;
        s.screen = sc;
        s.cur_slot = slot;
        s.slots[0].eid = "light.a"; s.slots[1].eid = "media_player.b"; s.slots[2].eid = "climate.c";
        s.status_detail = slot;
        s.n_count = 1; s.n_msg = "A long message that needs wrapping over several lines to fit";
        cv.night = night;
        draw_frame(cv, s);
        if (night) {
          bool red_only = true;
          for (int i = 0; i < 240 * 240; i++) red_only &= cv.data[i * 3 + 1] == 0 && cv.data[i * 3 + 2] == 0;
          CHECK(red_only);
        }
      }
  bool guard = true;
  for (int i = 0; i < 32; i++) guard &= buf[i] == 0xAB && buf[sizeof(buf) - 1 - i] == 0xAB;
  CHECK(guard);
}

int main() {
  test_notifications();
  test_time_helpers();
  test_versions();
  test_fold();
  test_swatches();
  test_render();
  if (failures) { printf("%d check(s) failed\n", failures); return 1; }
  printf("All helper tests passed\n");
  return 0;
}
