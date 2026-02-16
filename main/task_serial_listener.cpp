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
  M5.Display.setTextSize(4); // Large text
  M5.Display.setTextDatum(m5gfx::datum_t::middle_center);
  M5.Display.drawString(text, M5.Display.width() / 2, M5.Display.height() / 2);
}

void task_serial_listener_t::draw_ros2_ui(const char *status) {
  M5.Display.fillScreen(BLACK);

  // Header with clean line
  M5.Display.setTextColor(BLUE);
  M5.Display.setTextSize(1.5, 1.5);
  M5.Display.setTextDatum(m5gfx::datum_t::top_left);
  M5.Display.drawString("KANTAN Play", 10, 5);
  M5.Display.drawFastHLine(0, 25, 320, BLUE);

  // Status section (Balanced size)
  M5.Display.setTextSize(2.0);
  M5.Display.setTextColor(WHITE);
  M5.Display.setTextDatum(m5gfx::datum_t::top_left);
  M5.Display.drawString("STATUS:", 15, 40);

  M5.Display.setTextSize(4.0); // Reduced from 6.0
  M5.Display.setTextColor(CYAN);
  M5.Display.drawString(status, 25, 65);

  // Command List section (Increased size for readability)
  M5.Display.setTextSize(1.5);
  M5.Display.setTextColor(YELLOW);
  M5.Display.drawString("AVAILABLE COMMANDS:", 15, 120);

  M5.Display.setTextColor(LIGHTGREY);
  M5.Display.setTextSize(1.5);
  M5.Display.setCursor(25, 145);
  M5.Display.println("ready, start_seq, move, grip,");
  M5.Display.setCursor(25, 170);
  M5.Display.println("thinking, alert, finish");

  // Secondary info at bottom
  M5.Display.setTextSize(1.0);
  M5.Display.setTextColor(BLUE);
  M5.Display.setTextDatum(m5gfx::datum_t::bottom_left);
  M5.Display.drawString("ROS2 BRIDGE ACTIVE", 10, 215);

  // Footer / Exit Hint
  M5.Display.drawFastHLine(0, 220, 320, DARKGREY);
  M5.Display.setTextColor(LIGHTGREY);
  M5.Display.setTextSize(1.0);
  M5.Display.setCursor(10, 226);
  M5.Display.println("Hold SIDE BUTTON to EXIT");
}

void task_serial_listener_t::action_ready() {
  set_visual(GREEN, "SOUND TEST");
  program_change(CH_GTR, 30);
  note_on(CH_GTR, 45, 120);
  note_on(CH_GTR, 52, 110);
  vTaskDelay(pdMS_TO_TICKS(500));
  note_off(CH_GTR, 45);
  note_off(CH_GTR, 52);
  draw_ros2_ui("READY");
}

void task_serial_listener_t::action_start() {
  set_visual(BLUE, "START");
  for (int i = 0; i < 4; i++) {
    note_on(CH_DRM, 38, 120);
    vTaskDelay(pdMS_TO_TICKS(500));
    note_off(CH_DRM, 38);
  }
  draw_ros2_ui("RUNNING");
}

void task_serial_listener_t::action_move() {
  program_change(CH_BAS, 33);
  note_on(CH_BAS, 28, 100);
  vTaskDelay(pdMS_TO_TICKS(80));
  note_off(CH_BAS, 28);
  draw_ros2_ui("RUNNING");
}

void task_serial_listener_t::action_grip() {
  set_visual(WHITE, "GRIP!");
  note_on(CH_DRM, 38, 127);
  vTaskDelay(pdMS_TO_TICKS(150));
  note_off(CH_DRM, 38);
  draw_ros2_ui("RUNNING");
}

void task_serial_listener_t::action_thinking() {
  program_change(CH_GTR, 12); // Vibraphone
  for (int i = 0; i < 6; i++) {
    char thinking_text[16];
    snprintf(thinking_text, sizeof(thinking_text), "THINKING%s",
             (i % 3 == 0)   ? "."
             : (i % 3 == 1) ? ".."
                            : "...");
    set_visual(CYAN, thinking_text);

    note_on(CH_GTR, 72 + (i % 3) * 4, 80); // High pitched soft notes
    vTaskDelay(pdMS_TO_TICKS(400));
    note_off(CH_GTR, 72 + (i % 3) * 4);
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  draw_ros2_ui("IDLE");
}

void task_serial_listener_t::action_alert() {
  for (int i = 0; i < 5; i++) {
    // Shake effect
    int offset_x = (i % 2 == 0) ? 10 : -10;
    M5.Display.fillScreen(BLACK);
    M5.Display.setTextColor(RED);
    M5.Display.setTextSize(6);
    M5.Display.setTextDatum(middle_center);
    M5.Display.drawString("ALERT!!", M5.Display.width() / 2 + offset_x,
                          M5.Display.height() / 2);

    note_on(CH_GTR, 64, 127);
    note_on(CH_GTR, 65, 127);
    vTaskDelay(pdMS_TO_TICKS(150));
    note_off(CH_GTR, 64);
    note_off(CH_GTR, 65);

    M5.Display.fillScreen(BLACK);
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  draw_ros2_ui("IDLE");
}

void task_serial_listener_t::action_finish() {
  set_visual(YELLOW, "FINISHED");
  program_change(CH_GTR, 30);
  uint8_t notes[] = {45, 47, 40};
  for (uint8_t n : notes) {
    note_on(CH_GTR, n, 120);
    vTaskDelay(pdMS_TO_TICKS(200));
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
  vTaskDelay(pdMS_TO_TICKS(500));

  system_registry->user_setting.setMIDIMasterVolume(127);
  system_registry->midi_out_control.setControlChange(CH_GTR, 7, 120);
  system_registry->midi_out_control.setControlChange(CH_BAS, 7, 110);
  system_registry->midi_out_control.setControlChange(CH_DRM, 7, 120);

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
      if (exit_counter >
          100) { // 10ms * 100 = 1s (Adjusted to be snappy but safe)
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
        vTaskDelay(pdMS_TO_TICKS(1000));
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
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

} // namespace kanplay_ns
