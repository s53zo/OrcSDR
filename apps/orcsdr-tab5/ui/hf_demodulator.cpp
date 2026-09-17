#include "hf_demodulator.hpp"
#include <cmath>
#include <cstring>

namespace orcsdr::shortwave {
namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr uint32_t kIntermediateRate = 12000;
// Defined conversion from modular CIC arithmetic to a signed sample.
float signed_sample(uint32_t value) {
  return float(value <= INT32_MAX ? int64_t(value) : int64_t(value) - (int64_t(1) << 32));
}
}

bool Demodulator::configure(Mode mode, uint32_t rate, uint32_t bandwidth) {
  if ((rate != 960000 && rate != 2400000) || mode == Mode::am ||
      (mode != Mode::usb && mode != Mode::lsb && mode != Mode::cw)) {
    rate_ = decimation_ = 0;
    return false;
  }
  bandwidth = clamp_bandwidth(mode, bandwidth);
  if (mode == mode_ && rate == rate_ && bandwidth == bandwidth_) return true;
  mode_ = mode; rate_ = rate; bandwidth_ = bandwidth;
  decimation_ = rate / kIntermediateRate;
  gain_ = 1.0f / (float(decimation_) * decimation_ * decimation_);
  const double center = mode == Mode::cw ? 0 : (bandwidth + 300.0) / 2 * (mode == Mode::usb ? 1 : -1);
  const double cutoff = (mode == Mode::cw ? bandwidth / 2.0 : (bandwidth - 300.0) / 2) / kIntermediateRate;
  double sum = 0;
  for (size_t n = 0; n < kTaps; ++n) {
    const double t = double(n) - (kTaps - 1) / 2;
    const double h = (t == 0 ? 2 * cutoff : std::sin(2 * kPi * cutoff * t) / (kPi * t)) *
                     (0.54 - 0.46 * std::cos(2 * kPi * n / (kTaps - 1)));
    const double angle = 2 * kPi * center * t / kIntermediateRate;
    real_[n] = float(h * std::cos(angle));
    imag_[n] = float(h * std::sin(angle));
    sum += h;
  }
  for (size_t n = 0; n < kTaps; ++n) { real_[n] /= float(sum); imag_[n] /= float(sum); }
  reset();
  return true;
}

void Demodulator::reset() {
  std::memset(integrator_, 0, sizeof(integrator_));
  std::memset(comb_, 0, sizeof(comb_));
  std::memset(i_, 0, sizeof(i_)); std::memset(q_, 0, sizeof(q_));
  std::memset(output_x_, 0, sizeof(output_x_)); std::memset(output_y_, 0, sizeof(output_y_));
  ring_ = phase_ = 0; pending_i_ = false; previous_ = 0;
  oscillator_cos_ = 1; oscillator_sin_ = 0;
}

float Demodulator::interpolate_filter(float value) {
  // Two Butterworth biquads at 3.8 kHz suppress 12 kHz interpolation images.
  constexpr float b0 = 0.0453053269f, b1 = 0.0906106538f, b2 = b0;
  constexpr float a1 = -1.3142151492f, a2 = 0.4954364569f;
  for (unsigned stage = 0; stage < 2; ++stage) {
    const float out = b0 * value + b1 * output_x_[stage][0] + b2 * output_x_[stage][1] -
                      a1 * output_y_[stage][0] - a2 * output_y_[stage][1];
    output_x_[stage][1] = output_x_[stage][0]; output_x_[stage][0] = value;
    output_y_[stage][1] = output_y_[stage][0]; output_y_[stage][0] = out;
    value = out;
  }
  return value;
}

void Demodulator::process(const uint8_t* iq, size_t bytes, Output output, void* context) {
  if (!iq || !output || !decimation_) return;
  constexpr float rotation_cos = 0.9335804265f; // 700 Hz at 12 kHz
  constexpr float rotation_sin = 0.3583679495f;
  for (size_t b = 0; b < bytes; ++b) {
    if (!pending_i_) { input_i_ = iq[b]; pending_i_ = true; continue; }
    pending_i_ = false;
    const int32_t inputs[] = {int32_t(input_i_) - 128, int32_t(iq[b]) - 128};
    for (unsigned c = 0; c < 2; ++c) {
      integrator_[c][0] += uint32_t(inputs[c]);
      integrator_[c][1] += integrator_[c][0];
      integrator_[c][2] += integrator_[c][1];
    }
    if (++phase_ != decimation_) continue;
    phase_ = 0;
    float sample[2];
    for (unsigned c = 0; c < 2; ++c) {
      uint32_t value = integrator_[c][2];
      for (unsigned stage = 0; stage < 3; ++stage) {
        const uint32_t next = value - comb_[c][stage];
        comb_[c][stage] = value; value = next;
      }
      sample[c] = signed_sample(value) * gain_;
    }
    i_[ring_] = sample[0]; q_[ring_] = sample[1];
    float fi = 0, fq = 0;
    size_t index = ring_;
    for (size_t n = 0; n < kTaps; ++n) {
      fi += real_[n] * i_[index] - imag_[n] * q_[index];
      if (mode_ == Mode::cw) fq += real_[n] * q_[index];
      index = index ? index - 1 : kTaps - 1;
    }
    ring_ = (ring_ + 1) % kTaps;
    if (mode_ == Mode::cw) {
      fi = fi * oscillator_cos_ - fq * oscillator_sin_;
      const float next_cos = oscillator_cos_ * rotation_cos - oscillator_sin_ * rotation_sin;
      oscillator_sin_ = oscillator_sin_ * rotation_cos + oscillator_cos_ * rotation_sin;
      oscillator_cos_ = next_cos;
      // Keep the oscillator bounded during long continuous reception.
      const float correction = 1.5f - 0.5f * (oscillator_cos_ * oscillator_cos_ + oscillator_sin_ * oscillator_sin_);
      oscillator_cos_ *= correction; oscillator_sin_ *= correction;
    }
    for (unsigned n = 1; n <= 4; ++n)
      output(interpolate_filter(previous_ + (fi - previous_) * (n * 0.25f)), context);
    previous_ = fi;
  }
}
}  // namespace orcsdr::shortwave
