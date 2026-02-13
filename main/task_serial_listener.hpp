// SPDX-License-Identifier: MIT
// Copyright (c) 2025 InstaChord Corp.

#ifndef KANPLAY_TASK_SERIAL_LISTENER_HPP
#define KANPLAY_TASK_SERIAL_LISTENER_HPP

#include "system_registry.hpp"

namespace kanplay_ns {

class task_serial_listener_t {
public:
  void start(void);

private:
  static void task_func(void *arg);

  // RockBank logic
  void play_tone(float freq, int duration_ms);
  void drum_hit(const char *type);
  void power_chord(float base_freq, int duration_ms);

  void set_color(uint8_t r, uint8_t g, uint8_t b);
  void clear_color();

  void play_ready();
  void play_start();
  void play_move();
  void play_grip();
  void play_alert();
  void play_finish();

  // Helpers
  int freq_to_note(float freq);
  void note_on(uint8_t ch, uint8_t note, uint8_t vel);
  void note_off(uint8_t ch, uint8_t note);
};

}; // namespace kanplay_ns

#endif
