// SPDX-License-Identifier: MIT
// Copyright (c) 2025 InstaChord Corp.

#include "task_serial_listener.hpp"
#include "driver/uart.h"
#include <M5Unified.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace kanplay_ns {

// Helper to convert Frequency to MIDI Note
int task_serial_listener_t::freq_to_note(float freq) {
  if (freq <= 0)
    return 0;
  // MIDI Note 69 is A4 (440Hz)
  return round(69 + 12 * log2(freq / 440.0));
}

void task_serial_listener_t::note_on(uint8_t ch, uint8_t note, uint8_t vel) {
  if (note > 127)
    note = 127;
  // Send to system registry queue
  // Use MIDI Out Control directly
  system_registry->midi_out_control.setNoteVelocity(ch, note, vel);
}

void task_serial_listener_t::note_off(uint8_t ch, uint8_t note) {
  if (note > 127)
    note = 127;
  system_registry->midi_out_control.setNoteVelocity(ch, note, 0);
}

void task_serial_listener_t::play_tone(float freq, int duration_ms) {
  int note = freq_to_note(freq);
  if (note <= 0 || note > 127)
    return;

  // Use Channel 0 (Index 0) for melody
  uint8_t ch = def::midi::channel_1;

  note_on(ch, note, 100);
  // Use vTaskDelay
  vTaskDelay((duration_ms) / portTICK_PERIOD_MS);
  note_off(ch, note);
}

void task_serial_listener_t::power_chord(float base_freq, int duration_ms) {
  int root = freq_to_note(base_freq);
  int fifth = root + 7;
  int octave = root + 12;

  uint8_t ch = def::midi::channel_1; // Distortion Guitar

  // Arpeggio effect in Python was done by looping 3ms per tone.
  // Here we can just play them all simultaneously for a real power chord
  // OR replicate the arpeggio effect?
  // The Python code:
  // while time < duration: play(base, 3); play(fifth, 3); play(octave, 3);
  // This is essentially playing them extremely fast in sequence, creating a
  // rough texture. On MIDI synth, playing them together (polyphony) sounds
  // better. Let's play them together.

  note_on(ch, root, 100);
  note_on(ch, fifth, 90);
  note_on(ch, octave, 80);

  vTaskDelay((duration_ms) / portTICK_PERIOD_MS);

  note_off(ch, root);
  note_off(ch, fifth);
  note_off(ch, octave);
}

void task_serial_listener_t::drum_hit(const char *type) {
  uint8_t ch = def::midi::channel_10; // Drums

  if (strcmp(type, "kick") == 0) {
    // Bass Drum 1
    note_on(ch, 36, 127);
    vTaskDelay((50) / portTICK_PERIOD_MS);
    note_off(ch, 36);
  } else if (strcmp(type, "snare") == 0) {
    // Acoustic Snare
    note_on(ch, 38, 127);
    vTaskDelay((30) / portTICK_PERIOD_MS);
    note_off(ch, 38);
    // Double hit effect from Python code?
    // Python: play(400, 30); play(300, 30);
    // Let's create a flam effect
    vTaskDelay((10) / portTICK_PERIOD_MS);
    note_on(ch, 38, 100);
    vTaskDelay((30) / portTICK_PERIOD_MS);
    note_off(ch, 38);
  }
  vTaskDelay((20) / portTICK_PERIOD_MS);
}

void task_serial_listener_t::set_color(uint8_t r, uint8_t g, uint8_t b) {
  // Fill screen with color
  M5.Display.fillScreen(m5gfx::color565(r, g, b));

  // Also try to set LEDs if M5Unified supports them on this board
  // M5Stack Core2 has SK6812 (NeoPixel compatible) on bottom
  // but M5Unified usually handles them via specific calls if enabled.
  // For now, Screen is enough feedback.
}

void task_serial_listener_t::clear_color() { M5.Display.fillScreen(0); }

// RockBank Routines
void task_serial_listener_t::play_ready() {
  set_color(0, 255, 0); // Green

  // Sweep effect: 200 to 400 step 20
  // range(200, 400, 20) -> 10 steps
  uint8_t ch = def::midi::channel_1;
  for (float f = 200; f < 400; f += 20) {
    play_tone(f, 10);
  }

  power_chord(440, 600); // A4 Power Chord
  clear_color();
}

void task_serial_listener_t::play_start() {
  set_color(0, 0, 255); // Blue
  // 4 counts
  for (int i = 0; i < 4; ++i) {
    drum_hit("snare");
    vTaskDelay((200) / portTICK_PERIOD_MS);
  }
  clear_color();
}

void task_serial_listener_t::play_move() {
  // Bass line
  // 82Hz = E2 (~Note 40)
  uint8_t ch = def::midi::channel_1;
  note_on(ch, 40, 100);
  vTaskDelay((80) / portTICK_PERIOD_MS);
  note_off(ch, 40);

  vTaskDelay((20) / portTICK_PERIOD_MS);

  note_on(ch, 40, 100);
  vTaskDelay((80) / portTICK_PERIOD_MS);
  note_off(ch, 40);
}

void task_serial_listener_t::play_grip() {
  set_color(255, 255, 255); // White
  drum_hit("snare");
  vTaskDelay((100) / portTICK_PERIOD_MS);
  clear_color();
}

void task_serial_listener_t::play_alert() {
  set_color(255, 0, 0); // Red
  uint8_t ch = def::midi::channel_1;

  for (int i = 0; i < 3; ++i) {
    // Dissonance: 300Hz (~62) and 315Hz (~63)
    note_on(ch, 62, 100);
    note_on(ch, 63, 100);
    vTaskDelay((50) / portTICK_PERIOD_MS);
    note_off(ch, 62);
    note_off(ch, 63);

    clear_color();
    vTaskDelay((50) / portTICK_PERIOD_MS);
    set_color(255, 0, 0);
  }
  clear_color();
}

void task_serial_listener_t::play_finish() {
  set_color(255, 255, 0); // Yellow
  power_chord(440, 200);  // A4
  vTaskDelay((50) / portTICK_PERIOD_MS);
  power_chord(554, 200); // C#5
  vTaskDelay((50) / portTICK_PERIOD_MS);
  power_chord(659, 800); // E5
  clear_color();
}

void task_serial_listener_t::start() {
  // Setup Guitar Sound on Channel 1 (Distortion Guitar = 30)
  // Send Program Change
  // Wait for system to be ready?
  vTaskDelay((1000) / portTICK_PERIOD_MS);

  system_registry->midi_out_control.setProgramChange(def::midi::channel_1,
                                                     30); // Distortion Guitar
  system_registry->midi_out_control.setChannelVolume(def::midi::channel_1, 100);

  system_registry->midi_out_control.setChannelVolume(def::midi::channel_10,
                                                     120); // Drums

  // Create Task
  xTaskCreate((TaskFunction_t)task_func, "serial_listener", 4096, this, 1,
              nullptr);
}

void task_serial_listener_t::task_func(void *arg) {
  task_serial_listener_t *me = (task_serial_listener_t *)arg;

  char buffer[128];
  int pos = 0;

  // Default stdin is usually UART0.
  // On ESP-IDF, fgetc(stdin) blocks until a character is available
  // if VFS is configured that way.

  printf("Kanpure Rock System Ready (C++ Port).\n");
  me->play_ready();

  while (true) {
    int c = fgetc(stdin);
    if (c != EOF) {
      if (c == '\n' || c == '\r') {
        if (pos > 0) {
          buffer[pos] = 0;
          // Process command

          if (strcmp(buffer, "ready") == 0)
            me->play_ready();
          else if (strcmp(buffer, "start_seq") == 0)
            me->play_start();
          else if (strcmp(buffer, "move") == 0)
            me->play_move();
          else if (strcmp(buffer, "grip") == 0)
            me->play_grip();
          else if (strcmp(buffer, "alert") == 0)
            me->play_alert();
          else if (strcmp(buffer, "finish") == 0)
            me->play_finish();

          pos = 0;
        }
      } else {
        if (pos < sizeof(buffer) - 1) {
          buffer[pos++] = (char)c;
        }
      }
    } else {
      vTaskDelay((10) / portTICK_PERIOD_MS);
    }
  }
}

}; // namespace kanplay_ns
