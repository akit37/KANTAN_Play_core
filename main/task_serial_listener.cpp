// SPDX-License-Identifier: MIT
// Copyright (c) 2025 InstaChord Corp.

#include "task_serial_listener.hpp"
#include <Arduino.h>
#include <M5Unified.h>

namespace kanplay_ns {

static constexpr uint8_t KAN_ON_FLAG = 0x80;
static constexpr uint8_t CH_GTR = def::midi::channel_1;
static constexpr uint8_t CH_BAS = def::midi::channel_2;
static constexpr uint8_t CH_DRM = def::midi::channel_10;

// UI Layout Constants
namespace UILayout {
constexpr int SCREEN_WIDTH = 320;
constexpr int SCREEN_HEIGHT = 240;

// Header
constexpr int HEADER_X = 10;
constexpr int HEADER_Y = 5;
constexpr int HEADER_LINE_Y = 25;
constexpr float HEADER_TEXT_SIZE = 1.5f;

// Status Section
constexpr int STATUS_LABEL_X = 15;
constexpr int STATUS_LABEL_Y = 40;
constexpr float STATUS_LABEL_SIZE = 2.0f;
constexpr int STATUS_VALUE_X = 25;
constexpr int STATUS_VALUE_Y = 65;
constexpr float STATUS_VALUE_SIZE = 4.0f;

// Command List Section
constexpr int CMD_LIST_HEADER_X = 15;
constexpr int CMD_LIST_HEADER_Y = 120;
constexpr float CMD_LIST_TEXT_SIZE = 1.5f;
constexpr int CMD_LIST_LINE1_X = 25;
constexpr int CMD_LIST_LINE1_Y = 145;
constexpr int CMD_LIST_LINE2_X = 25;
constexpr int CMD_LIST_LINE2_Y = 170;

// Footer
constexpr int FOOTER_LINE_Y = 220;
constexpr int FOOTER_INFO_X = 10;
constexpr int FOOTER_INFO_Y = 215;
constexpr int FOOTER_HINT_X = 10;
constexpr int FOOTER_HINT_Y = 226;
constexpr float FOOTER_TEXT_SIZE = 1.0f;

// Visual Effects
constexpr float LARGE_TEXT_SIZE = 4.0f;
constexpr float ALERT_TEXT_SIZE = 6.0f;
constexpr int SHAKE_OFFSET = 10;
} // namespace UILayout

// MIDI Note Numbers
namespace MIDINotes {
// Guitar notes
constexpr uint8_t GUITAR_LOW_A = 45;
constexpr uint8_t GUITAR_E = 52;
constexpr uint8_t GUITAR_C = 40;
constexpr uint8_t GUITAR_D = 47;

// Drum notes
constexpr uint8_t SNARE_DRUM = 38;
constexpr uint8_t BASS_DRUM = 36;

// Bass notes
constexpr uint8_t BASS_E = 28;

// Dissonance (for alert)
constexpr uint8_t DISSONANCE_LOW = 64;
constexpr uint8_t DISSONANCE_HIGH = 65;

// Thinking animation (vibraphone)
constexpr uint8_t THINKING_BASE = 72;
constexpr uint8_t THINKING_INTERVAL = 4;
} // namespace MIDINotes

// MIDI Program Numbers
namespace MIDIPrograms {
constexpr uint8_t DISTORTION_GUITAR = 30;
constexpr uint8_t VIBRAPHONE = 12;
constexpr uint8_t PICKED_BASS = 33;
} // namespace MIDIPrograms

// Timing Constants (in milliseconds)
namespace Timing {
// Ready action
constexpr int READY_CHORD_DURATION = 500;

// Start action
constexpr int START_SNARE_INTERVAL = 500;

// Move action
constexpr int MOVE_NOTE_DURATION = 80;

// Grip action
constexpr int GRIP_DISPLAY_DURATION = 150;

// Thinking action
constexpr int THINKING_NOTE_DURATION = 400;
constexpr int THINKING_NOTE_GAP = 100;
constexpr int THINKING_LOOP_COUNT = 6;

// Alert action
constexpr int ALERT_FLASH_DURATION = 150;
constexpr int ALERT_FLASH_GAP = 100;
constexpr int ALERT_LOOP_COUNT = 5;

// Finish action
constexpr int FINISH_NOTE_DURATION = 200;

// Task delays
constexpr int TASK_STARTUP_DELAY = 500;
constexpr int TASK_LOOP_DELAY = 10;
constexpr int EXIT_HOLD_THRESHOLD = 100; // 10ms * 100 = 1s
constexpr int EXIT_MESSAGE_DURATION = 1000;
} // namespace Timing

// MIDI Velocities
namespace MIDIVelocity {
constexpr uint8_t SOFT = 80;
constexpr uint8_t MEDIUM = 100;
constexpr uint8_t MEDIUM_HIGH = 110;
constexpr uint8_t VEL_HIGH = 120;
constexpr uint8_t MAXIMUM = 127;
} // namespace MIDIVelocity

// MIDI Control Change Values
namespace MIDIControl {
constexpr uint8_t VOLUME_CC = 7;
constexpr uint8_t GUITAR_VOLUME = 120;
constexpr uint8_t BASS_VOLUME = 110;
constexpr uint8_t DRUM_VOLUME = 120;
constexpr uint8_t MASTER_VOLUME = 127;
} // namespace MIDIControl

void task_serial_listener_t::start() {
  xTaskCreatePinnedToCore((TaskFunction_t)task_func, "serial_bridge", 4096,
                          this, 2, nullptr, 1);
}

void task_serial_listener_t::note_on(uint8_t ch, uint8_t note, uint8_t vel) {
  if (system_registry) {
    system_registry->midi_out_control.setNoteVelocity(
        ch, note, (vel & 0x7F) | KAN_ON_FLAG);
  }
}

void task_serial_listener_t::note_off(uint8_t ch, uint8_t note) {
  if (system_registry) {
    system_registry->midi_out_control.setNoteVelocity(ch, note, 0);
  }
}

void task_serial_listener_t::program_change(uint8_t ch, uint8_t prg) {
  if (system_registry) {
    system_registry->midi_out_control.setProgramChange(ch, prg);
  }
}

void task_serial_listener_t::set_visual(uint32_t color, const char *text) {
  M5.Display.fillScreen(BLACK);
  M5.Display.setTextColor(color);
  M5.Display.setTextSize(UILayout::LARGE_TEXT_SIZE);
  M5.Display.setTextDatum(m5gfx::datum_t::middle_center);
  M5.Display.drawString(text, M5.Display.width() / 2, M5.Display.height() / 2);
}

void task_serial_listener_t::draw_ros2_ui(const char *status) {
  M5.Display.fillScreen(BLACK);

  // Header with clean line
  M5.Display.setTextColor(BLUE);
  M5.Display.setTextSize(UILayout::HEADER_TEXT_SIZE,
                         UILayout::HEADER_TEXT_SIZE);
  M5.Display.setTextDatum(m5gfx::datum_t::top_left);
  M5.Display.drawString("KANTAN Play", UILayout::HEADER_X, UILayout::HEADER_Y);
  M5.Display.drawFastHLine(0, UILayout::HEADER_LINE_Y, UILayout::SCREEN_WIDTH,
                           BLUE);

  // Status section
  M5.Display.setTextSize(UILayout::STATUS_LABEL_SIZE);
  M5.Display.setTextColor(WHITE);
  M5.Display.setTextDatum(m5gfx::datum_t::top_left);
  M5.Display.drawString("STATUS:", UILayout::STATUS_LABEL_X,
                        UILayout::STATUS_LABEL_Y);

  M5.Display.setTextSize(UILayout::STATUS_VALUE_SIZE);
  M5.Display.setTextColor(CYAN);
  M5.Display.drawString(status, UILayout::STATUS_VALUE_X,
                        UILayout::STATUS_VALUE_Y);

  // Command List section
  M5.Display.setTextSize(UILayout::CMD_LIST_TEXT_SIZE);
  M5.Display.setTextColor(YELLOW);
  M5.Display.drawString("AVAILABLE COMMANDS:", UILayout::CMD_LIST_HEADER_X,
                        UILayout::CMD_LIST_HEADER_Y);

  M5.Display.setTextColor(LIGHTGREY);
  M5.Display.setTextSize(UILayout::CMD_LIST_TEXT_SIZE);
  M5.Display.setCursor(UILayout::CMD_LIST_LINE1_X, UILayout::CMD_LIST_LINE1_Y);
  M5.Display.println("ready, start_seq, move, grip,");
  M5.Display.setCursor(UILayout::CMD_LIST_LINE2_X, UILayout::CMD_LIST_LINE2_Y);
  M5.Display.println("thinking, alert, finish");

  // Secondary info at bottom
  M5.Display.setTextSize(UILayout::FOOTER_TEXT_SIZE);
  M5.Display.setTextColor(BLUE);
  M5.Display.setTextDatum(m5gfx::datum_t::bottom_left);
  M5.Display.drawString("ROS2 BRIDGE ACTIVE", UILayout::FOOTER_INFO_X,
                        UILayout::FOOTER_INFO_Y);

  // Footer / Exit Hint
  M5.Display.drawFastHLine(0, UILayout::FOOTER_LINE_Y, UILayout::SCREEN_WIDTH,
                           DARKGREY);
  M5.Display.setTextColor(LIGHTGREY);
  M5.Display.setTextSize(UILayout::FOOTER_TEXT_SIZE);
  M5.Display.setCursor(UILayout::FOOTER_HINT_X, UILayout::FOOTER_HINT_Y);
  M5.Display.println("Hold SIDE BUTTON to EXIT");
}

void task_serial_listener_t::action_ready() {
  set_visual(GREEN, "SOUND TEST");
  program_change(CH_GTR, MIDIPrograms::DISTORTION_GUITAR);
  note_on(CH_GTR, MIDINotes::GUITAR_LOW_A, MIDIVelocity::VEL_HIGH);
  note_on(CH_GTR, MIDINotes::GUITAR_E, MIDIVelocity::MEDIUM_HIGH);
  vTaskDelay(pdMS_TO_TICKS(Timing::READY_CHORD_DURATION));
  note_off(CH_GTR, MIDINotes::GUITAR_LOW_A);
  note_off(CH_GTR, MIDINotes::GUITAR_E);
  draw_ros2_ui("READY");
}

void task_serial_listener_t::action_start() {
  set_visual(BLUE, "START");
  for (int i = 0; i < 4; i++) {
    note_on(CH_DRM, MIDINotes::SNARE_DRUM, MIDIVelocity::VEL_HIGH);
    vTaskDelay(pdMS_TO_TICKS(Timing::START_SNARE_INTERVAL));
    note_off(CH_DRM, MIDINotes::SNARE_DRUM);
  }
  draw_ros2_ui("RUNNING");
}

void task_serial_listener_t::action_move() {
  program_change(CH_BAS, MIDIPrograms::PICKED_BASS);
  note_on(CH_BAS, MIDINotes::BASS_E, MIDIVelocity::MEDIUM);
  vTaskDelay(pdMS_TO_TICKS(Timing::MOVE_NOTE_DURATION));
  note_off(CH_BAS, MIDINotes::BASS_E);
  draw_ros2_ui("RUNNING");
}

void task_serial_listener_t::action_grip() {
  set_visual(WHITE, "GRIP!");
  note_on(CH_DRM, MIDINotes::SNARE_DRUM, MIDIVelocity::MAXIMUM);
  vTaskDelay(pdMS_TO_TICKS(Timing::GRIP_DISPLAY_DURATION));
  note_off(CH_DRM, MIDINotes::SNARE_DRUM);
  draw_ros2_ui("RUNNING");
}

void task_serial_listener_t::action_thinking() {
  program_change(CH_GTR, MIDIPrograms::VIBRAPHONE);
  for (int i = 0; i < Timing::THINKING_LOOP_COUNT; i++) {
    char thinking_text[16];
    snprintf(thinking_text, sizeof(thinking_text), "THINKING%s",
             (i % 3 == 0)   ? "."
             : (i % 3 == 1) ? ".."
                            : "...");
    set_visual(CYAN, thinking_text);

    note_on(CH_GTR,
            MIDINotes::THINKING_BASE + (i % 3) * MIDINotes::THINKING_INTERVAL,
            MIDIVelocity::SOFT);
    vTaskDelay(pdMS_TO_TICKS(Timing::THINKING_NOTE_DURATION));
    note_off(CH_GTR,
             MIDINotes::THINKING_BASE + (i % 3) * MIDINotes::THINKING_INTERVAL);
    vTaskDelay(pdMS_TO_TICKS(Timing::THINKING_NOTE_GAP));
  }
  draw_ros2_ui("IDLE");
}

void task_serial_listener_t::action_alert() {
  for (int i = 0; i < Timing::ALERT_LOOP_COUNT; i++) {
    // Shake effect
    int offset_x =
        (i % 2 == 0) ? UILayout::SHAKE_OFFSET : -UILayout::SHAKE_OFFSET;
    M5.Display.fillScreen(BLACK);
    M5.Display.setTextColor(RED);
    M5.Display.setTextSize(UILayout::ALERT_TEXT_SIZE);
    M5.Display.setTextDatum(middle_center);
    M5.Display.drawString("ALERT!!", M5.Display.width() / 2 + offset_x,
                          M5.Display.height() / 2);

    note_on(CH_GTR, MIDINotes::DISSONANCE_LOW, MIDIVelocity::MAXIMUM);
    note_on(CH_GTR, MIDINotes::DISSONANCE_HIGH, MIDIVelocity::MAXIMUM);
    vTaskDelay(pdMS_TO_TICKS(Timing::ALERT_FLASH_DURATION));
    note_off(CH_GTR, MIDINotes::DISSONANCE_LOW);
    note_off(CH_GTR, MIDINotes::DISSONANCE_HIGH);

    M5.Display.fillScreen(BLACK);
    vTaskDelay(pdMS_TO_TICKS(Timing::ALERT_FLASH_GAP));
  }
  draw_ros2_ui("IDLE");
}

void task_serial_listener_t::action_finish() {
  set_visual(YELLOW, "FINISHED");
  program_change(CH_GTR, MIDIPrograms::DISTORTION_GUITAR);
  uint8_t notes[] = {MIDINotes::GUITAR_LOW_A, MIDINotes::GUITAR_D,
                     MIDINotes::GUITAR_C};
  for (uint8_t n : notes) {
    note_on(CH_GTR, n, MIDIVelocity::VEL_HIGH);
    vTaskDelay(pdMS_TO_TICKS(Timing::FINISH_NOTE_DURATION));
    note_off(CH_GTR, n);
  }
  draw_ros2_ui("IDLE");
}

void task_serial_listener_t::handle_command(const char *cmd) {
  Serial.printf("STDOUT: Executing [%s]\n", cmd);
  if (strcmp(cmd, "ready") == 0)
    action_ready();
  else if (strcmp(cmd, "start_seq") == 0)
    action_start();
  else if (strcmp(cmd, "move") == 0)
    action_move();
  else if (strcmp(cmd, "grip") == 0)
    action_grip();
  else if (strcmp(cmd, "thinking") == 0)
    action_thinking();
  else if (strcmp(cmd, "alert") == 0)
    action_alert();
  else if (strcmp(cmd, "finish") == 0)
    action_finish();
}

void task_serial_listener_t::task_func(void *arg) {
  task_serial_listener_t *me = (task_serial_listener_t *)arg;
  vTaskDelay(pdMS_TO_TICKS(Timing::TASK_STARTUP_DELAY));

  system_registry->user_setting.setMIDIMasterVolume(MIDIControl::MASTER_VOLUME);
  system_registry->midi_out_control.setControlChange(
      CH_GTR, MIDIControl::VOLUME_CC, MIDIControl::GUITAR_VOLUME);
  system_registry->midi_out_control.setControlChange(
      CH_BAS, MIDIControl::VOLUME_CC, MIDIControl::BASS_VOLUME);
  system_registry->midi_out_control.setControlChange(
      CH_DRM, MIDIControl::VOLUME_CC, MIDIControl::DRUM_VOLUME);

  me->draw_ros2_ui("READY");

  char buffer[64];
  int pos = 0;
  int exit_counter = 0;

  while (true) {
    M5.update(); // M5Unified state update

    // KANTAN Play specific button handling via system_registry
    uint32_t btn_mask = system_registry->internal_input.getButtonBitmask();
    if (btn_mask & def::button_bitmask::SIDE_1) {
      exit_counter++;
      if (exit_counter > Timing::EXIT_HOLD_THRESHOLD) {
        Serial.println("FORCE EXIT: SIDE_1 Long Press Detected. Switching to "
                       "Instrument Mode...");
        system_registry->user_setting.setAppRunMode(0);
        system_registry->save();

        M5.Display.fillScreen(WHITE);
        M5.Display.setTextColor(BLACK);
        M5.Display.setTextSize(2);
        M5.Display.setCursor(10, 100);
        M5.Display.println("EXIT ROS2 MODE");
        M5.Display.println("Back to Instrument...");
        vTaskDelay(pdMS_TO_TICKS(Timing::EXIT_MESSAGE_DURATION));
        esp_restart();
      }
    } else {
      exit_counter = 0;
    }

    while (Serial.available()) {
      int c = Serial.read();
      if (c == '\n' || c == '\r') {
        if (pos > 0) {
          buffer[pos] = 0;
          me->handle_command(buffer);
          pos = 0;
        }
      } else if (pos < (int)sizeof(buffer) - 1) {
        buffer[pos++] = (char)c;
      }
    }
    vTaskDelay(pdMS_TO_TICKS(Timing::TASK_LOOP_DELAY));
  }
}

} // namespace kanplay_ns
