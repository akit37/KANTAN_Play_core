// SPDX-License-Identifier: MIT
// Copyright (c) 2025 InstaChord Corp.

#include <M5Unified.h>
#include <stdio.h>

#include "system_registry.hpp"
#include "task_commander.hpp"
#include "task_http_client.hpp"
#include "task_i2c.hpp"
#include "task_i2s.hpp"
#include "task_kantanplay.hpp"
#include "task_midi.hpp"
#include "task_operator.hpp"
#include "task_serial_listener.hpp"
#include "task_spi.hpp"
#include "task_wifi.hpp"

namespace kanplay_ns {

static void startup_ros2_mode(void) {
  // 既に M5.begin は setup で呼ばれている前提
  Serial.println("Starting ROS2 Bridge Mode...");

  // CoreS3 Power management (SAM2695 enable)
  {
    static constexpr const uint8_t AW9523_ADDR = 0x58;
    uint8_t buf[2];
    M5.In_I2C.readRegister(AW9523_ADDR, 0x02, buf, 2, 100000);
    uint8_t bus_en = buf[0] | 0b10;
    uint8_t boost = buf[1] | 0b10000000;
    M5.In_I2C.writeRegister8(AW9523_ADDR, 0x02, bus_en & ~0b10, 100000);
    M5.In_I2C.writeRegister8(AW9523_ADDR, 0x03, boost, 100000);
    for (int i = 0; i < 128; ++i) {
      M5.In_I2C.writeRegister8(AW9523_ADDR, 0x02, bus_en, 400000);
      m5gfx::delayMicroseconds(i);
    }
  }
  M5.Power.setChargeCurrent(200);

  // タスク生成 (system_registry は setup で生成済み)
  auto task_i2c = new task_i2c_t();
  auto task_i2s = new task_i2s_t();
  auto task_midi = new task_midi_t();
  auto task_commander = new task_commander_t();
  auto task_operator = new task_operator_t();
  auto task_kantanplay = new task_kantanplay_t();
  auto task_serial = new task_serial_listener_t();

  task_i2s->start();
  task_i2c->start();
  task_midi->start();
  task_kantanplay->start();
  task_commander->start();
  task_operator->start();

  M5.delay(100);
  system_registry->operator_command.addQueue(
      {def::command::system_control, def::command::sc_boot});
  M5.delay(1000);
  task_serial->start();
}

static void startup_instrument_mode(void) {
  Serial.println("Starting Instrument Mode...");

  auto task_spi = new task_spi_t();
  auto task_i2c = new task_i2c_t();
  auto task_i2s = new task_i2s_t();
  auto task_midi = new task_midi_t();
  auto task_commander = new task_commander_t();
  auto task_operator = new task_operator_t();
  auto task_wifi = new task_wifi_t();
  auto task_http_client = new task_http_client_t();
  auto task_kantanplay = new task_kantanplay_t();

  task_spi->start();
  task_i2s->start();
  task_i2c->start();
  task_midi->start();
  task_kantanplay->start();
  task_commander->start();
  task_operator->start();
  task_wifi->start();
  task_http_client->start();

  M5.delay(100);
  system_registry->operator_command.addQueue(
      {def::command::system_control, def::command::sc_boot});
}

}; // namespace kanplay_ns

void setup() {
  auto cfg = M5.config();
  cfg.output_power = false; // Important for power-on safety
  cfg.internal_spk = false;
  M5.begin(cfg);
  Serial.begin(115200);

  M5.Display.setRotation(0);
  M5.Display.setTextSize(2);
  M5.Display.printf("KANTAN Play\nver%lu.%lu.%lu\n\nboot",
                    def::app::app_version_major, def::app::app_version_minor,
                    def::app::app_version_patch);

  // Give some time for FS and Serial to stabilize
  M5.delay(500);
  Serial.println("\n--- KANTAN Play Booting ---");

  kanplay_ns::system_registry = new kanplay_ns::system_registry_t();
  kanplay_ns::system_registry->init();

  uint8_t run_mode = kanplay_ns::system_registry->user_setting.getAppRunMode();
  Serial.printf("Booting with Run Mode: %d (%s)\n", run_mode,
                (run_mode == 1 ? "ROS2 Bridge" : "Instrument"));

  if (run_mode == 1) {
    kanplay_ns::startup_ros2_mode();
  } else {
    kanplay_ns::startup_instrument_mode();
  }
}

void loop() {
  M5.delay(100);
  M5.update(); // Maintain button state update for both modes
}

#if !defined(M5UNIFIED_PC_BUILD) && !defined(ARDUINO)
extern "C" void app_main(void) {
  setup();
  for (;;) {
    loop();
  }
}
#endif
