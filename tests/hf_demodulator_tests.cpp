#include "hf_demodulator.hpp"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
using namespace orcsdr::shortwave;
constexpr double pi = 3.14159265358979323846;
void append(float sample, void* context) {
  assert(std::isfinite(sample));
  static_cast<std::vector<float>*>(context)->push_back(sample);
}
std::vector<uint8_t> tone(uint32_t rate, double hz, double seconds = 0.15) {
  std::vector<uint8_t> iq;
  for (size_t n = 0; n < size_t(rate * seconds); ++n) {
    const double phase = 2*pi*hz*n/rate;
    iq.push_back(uint8_t(std::lround(128 + 70*std::cos(phase))));
    iq.push_back(uint8_t(std::lround(128 + 70*std::sin(phase))));
  }
  return iq;
}
std::vector<float> decode(Demodulator& d, const std::vector<uint8_t>& iq, size_t chunk = 137) {
  std::vector<float> result;
  for (size_t n = 0; n < iq.size(); n += chunk)
    d.process(iq.data()+n, std::min(chunk,iq.size()-n),append,&result);
  return result;
}
double energy(const std::vector<float>& samples) {
  assert(samples.size()>2400);
  double sum=0;
  for (size_t n=2400;n<samples.size();++n) sum+=samples[n]*samples[n];
  return std::sqrt(sum/(samples.size()-2400));
}
double amplitude(const std::vector<float>& samples, double hz) {
  double re=0,im=0;
  for(size_t n=2400;n<samples.size();++n) {
    re+=samples[n]*std::cos(2*pi*hz*n/48000);
    im+=samples[n]*std::sin(2*pi*hz*n/48000);
  }
  return 2*std::hypot(re,im)/(samples.size()-2400);
}
int main() {
  assert(model_self_check());
  assert(next_mode(Mode::am)==Mode::usb && next_mode(Mode::usb)==Mode::lsb &&
         next_mode(Mode::lsb)==Mode::cw && next_mode(Mode::cw)==Mode::am);
  for (auto mode : {Mode::am,Mode::usb,Mode::lsb,Mode::cw}) {
    assert(clamp_bandwidth(mode,default_bandwidth(mode))==default_bandwidth(mode));
    auto width=default_bandwidth(mode);
    for(int n=0;n<8;++n) {
      width=next_bandwidth(mode,width);
      assert(clamp_bandwidth(mode,width)==width);
    }
    Memory memory{}; memory.frequency_hz=7100000; memory.bandwidth_hz=default_bandwidth(mode);
    std::strcpy(memory.mode,mode_name(mode)); assert(valid(memory));
    std::memset(memory.mode,'X',sizeof(memory.mode));assert(!valid(memory));
    LogEntry log{};log.frequency_hz=7100000;log.timestamp_utc=1789600000;
    log.bandwidth_hz=default_bandwidth(mode);std::strcpy(log.mode,mode_name(mode));assert(valid(log));
    std::memset(log.mode,'X',sizeof(log.mode));assert(!valid(log));
  }
  for(uint32_t rate : {960000u,2400000u}) {
    for(auto mode : {Mode::usb,Mode::lsb}) {
      Demodulator d;
      assert(d.configure(mode,rate,2700));
      const double sign=mode==Mode::usb ? 1 : -1;
      const auto iq=tone(rate,sign*1000);
      auto wanted=decode(d,iq);
      assert(wanted.size()==7200);
      assert(amplitude(wanted,1000)>40); // Preserve pitch, not a shifted BFO tone.
      d.reset();auto rejected=decode(d,tone(rate,-sign*1000));
      assert(energy(rejected)<energy(wanted)*0.01); // >40 dB opposite-sideband rejection.
      d.reset();auto alias=decode(d,tone(rate,sign*13000));
      assert(energy(alias)<energy(wanted)*0.01); // Decimator rejects a same-output-frequency alias.
      d.reset();auto whole=decode(d,iq,iq.size());assert(whole==wanted);
      // A configuration change resets old IQ/filter memory exactly like a fresh receiver.
      assert(d.configure(Mode::cw,rate,500));
      auto cw=decode(d,tone(rate,0));
      assert(amplitude(cw,700)>50);
      d.reset();auto off=decode(d,tone(rate,1000));
      assert(energy(off)<energy(cw)*0.01);
      // Unsupported input rates must not keep processing with stale configuration.
      assert(!d.configure(mode,1000000,2700));
      assert(decode(d,iq).empty());
    }
    for(uint32_t width : {250u,500u,1000u}) {
      Demodulator d;assert(d.configure(Mode::cw,rate,width));
      auto iq=tone(rate,0,0.3);
      // Keyed CW: 100 ms on, 100 ms off, 100 ms on.
      std::fill(iq.begin()+size_t(rate/10)*2,iq.begin()+size_t(rate/5)*2,uint8_t(128));
      auto audio=decode(d,iq);
      double quiet=0; for(size_t i=8000;i<9400;++i) quiet+=audio[i]*audio[i];
      assert(std::sqrt(quiet/1400)<0.01);
      assert(amplitude(std::vector<float>(audio.begin()+9600,audio.end()),700)>50);
    }
  }
  std::puts("HF: USB/LSB pitch and rejection, keyed CW/700 Hz, both rates, gaps/reset and model validation passed");
}
