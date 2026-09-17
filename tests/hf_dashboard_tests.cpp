#include "shortwave_dashboard.hpp"
#include <M5Unified.h>
#include <algorithm>
#include <cassert>
#include <cstdio>
namespace orcsdr::audio_header {
void draw_brand(const char*){} void draw_battery(int32_t){} void draw_home_button(){}
void draw_mute_button(bool){} void draw_visualizer_button(bool){} void draw_settings_button(){}
bool home_hit(int32_t,int32_t){return false;} bool settings_hit(int32_t,int32_t){return false;}
}
int main() {
 using namespace orcsdr::shortwave;
 assert(dashboard_self_check());
 for (auto mode : {Mode::am,Mode::usb,Mode::lsb,Mode::cw}) {
  Snapshot s;s.mode=mode;s.filter_bandwidth_hz=default_bandwidth(mode);s.span_hz=12000;
  enter(s);
  assert(std::find(M5.Display.labels.begin(),M5.Display.labels.end(),mode_name(mode))!=M5.Display.labels.end());
  assert(handle_touch(270,270).kind==ActionKind::mode_cycle);
  assert(handle_touch(100,270).kind==ActionKind::step_cycle);
  assert(handle_touch(470,270).kind==ActionKind::filter_cycle);
  assert(handle_touch(710,270).kind==ActionKind::sound_toggle);
  assert(handle_touch(207,270).kind==ActionKind::none); // Gap between controls.
  const float levels[]={-90,-30,-60,-90};
  M5.Display.verticals.clear();draw_spectrum(levels,0,4,-100);
  assert(M5.Display.verticals.size()==3);
  const auto center=M5.Display.verticals[0].x;
  const auto low=M5.Display.verticals[1].x,high=M5.Display.verticals[2].x;
  if(mode==Mode::usb) assert(low==center && high>center);
  else if(mode==Mode::lsb) assert(low<center && high==center);
  else assert(low<center && high>center);
  if(mode==Mode::cw)
   assert(std::find(M5.Display.labels.begin(),M5.Display.labels.end(),"CW  /  700 Hz TONE")!=M5.Display.labels.end());
  leave();assert(handle_touch(270,270).kind==ActionKind::none);
 }
 std::puts("HF dashboard: four mode labels, controls and sideband passbands passed");
}
