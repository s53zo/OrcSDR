# HF receive modes

Experimental source implementation: AM, USB, LSB and CW on the Shortwave
receiver. Native ESP-IDF compilation, physical display acceptance and live RF
performance are not yet verified. The existing AM and CB demodulators remain
unchanged.

## Controls

Open **Home → Shortwave**. Tap the mode button beside STEP to cycle
**AM → USB → LSB → CW**. The selected mode is saved across reboots.

- **AM:** existing envelope demodulator; 4, 6 and 9 kHz filter presets.
- **USB / LSB:** selected-sideband audio with a 300 Hz lower edge; FILTER cycles
  the upper edge through 1,800 / 2,400 / 2,700 / 3,000 Hz. Default is 2,700 Hz.
  Tune the displayed frequency to the suppressed carrier frequency.
- **CW:** centered 250 / 500 / 1,000 Hz passband, default 500 Hz. Tune the carrier
  to the displayed frequency to hear a 700 Hz tone. This is audio reception;
  automatic Morse decoding and transmission are not included.

Changing mode chooses its default filter and tuning step (AM 1 kHz, SSB 100 Hz,
CW 10 Hz). STEP cycles 10 / 100 / 500 / 1,000 / 5,000 Hz. Shortwave hot tuning
now sends that actual frequency to the driver instead of rounding the hardware
frequency to the general receiver's 5 kHz grid. Actual RF accuracy still depends
on the dongle oscillator and driver. Spectrum passband markers follow the
selected sideband. The RF Visualizer's separate AM/FM channel-audio control is
unavailable for HF SSB/CW; the Shortwave receiver supplies the audio.

The new modes feed the existing 48 kHz speaker, audio recording and browser-audio
path. The HF mode does not alter the CB mode or clarifier.

## Implementation and validation

`hf_demodulator` is portable receive-only C++17, with no allocation, board calls
or file/network access. A three-stage modular integer CIC decimates 960 kS/s or
2.4 MS/s CU8 to 12 kS/s. Its maximum gain at 2.4 MS/s is 200³; full-scale input
remains within signed 32-bit output range. Integrator overflow uses defined
unsigned arithmetic. A 257-tap complex FIR selects USB/LSB, or a centered FIR
selects CW before a 700 Hz oscillator produces the listening tone. Linear
interpolation and two low-pass biquads produce 48 kHz mono.

The DSP task owns the PSRAM decoder state. Tuning and stream restarts request a
reset; mode/filter changes reconfigure it, and skipped IQ resets its history.
The existing audio shaper provides gain, limiting and fade behavior.

Run `bash tools/test-hf-demodulator.sh`. Optimized and ASan/UBSan host runs cover:

- USB/LSB pitch preservation and >40 dB opposite-sideband rejection for synthetic
  1 kHz tones at both supported input rates.
- Rejection of a 13 kHz input that would otherwise alias into the selected band.
- CW at 700 Hz, keyed carrier silence and all three filter widths.
- Odd CU8 chunk boundaries, reset/reconfiguration and invalid sample rates.
- Mode/filter cycles, bounded text validation for memory/log models, and the
  actual dashboard's mode controls, labels and sideband markers with host stubs.

These tests do not establish RF sensitivity or embedded CPU headroom. Before
release, build using `docs/TAB5_BUILD_POLICY.md`, then check USB/LSB voice and CW
against known signals, tune in 10/100 Hz increments, check filter rejection,
record audio, exercise browser playback, inspect IQ drops, and test mode changes,
reconnect and reboot. Direct sampling/tuner DC spurs can produce a centered CW
tone; antenna and receiver tests must distinguish that from a real carrier.
