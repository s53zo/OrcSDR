#pragma once
#include "shortwave_model.hpp"
#include <cstddef>
#include <cstdint>

namespace orcsdr::shortwave {
// Receive-only CU8 -> 48 kHz mono. One DSP task owns this object. The caller
// resets it on tuning, IQ gaps and mode changes; no allocation occurs here.
class Demodulator {
 public:
  using Output = void (*)(float sample, void* context);
  bool configure(Mode mode, uint32_t sample_rate, uint32_t bandwidth);
  void reset();
  void process(const uint8_t* iq, size_t bytes, Output output, void* context);
 private:
  static constexpr size_t kTaps = 257;
  Mode mode_ = Mode::am;
  uint32_t rate_ = 0, bandwidth_ = 0, decimation_ = 0, phase_ = 0;
  uint32_t integrator_[2][3]{}, comb_[2][3]{};
  float real_[kTaps]{}, imag_[kTaps]{}, i_[kTaps]{}, q_[kTaps]{};
  size_t ring_ = 0;
  bool pending_i_ = false;
  uint8_t input_i_ = 0;
  float gain_ = 0, previous_ = 0, oscillator_cos_ = 0, oscillator_sin_ = 0;
  float output_x_[2][2]{}, output_y_[2][2]{};
  float interpolate_filter(float value);
};
}  // namespace orcsdr::shortwave
