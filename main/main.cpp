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
  M5.begin(cfg);
  Serial.begin(115200);

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

static void init(void) {
#if !defined(M5UNIFIED_PC_BUILD)

  // メモリブロックの断片化への対策として、最大領域を先回りして確保しておく。(これにより小さい断片化領域から使用させることができる)
  auto dummy =
      m5gfx::heap_alloc_dma(heap_caps_get_largest_free_block(MALLOC_CAP_DMA));

#endif
  log_memory(0);
  auto cfg = M5.config();
  cfg.output_power = false;
  cfg.internal_spk = false;
  M5.begin(cfg);

  M5.Display.setRotation(0);

#if defined(CONFIG_IDF_TARGET_ESP32S3)
  // CoreS3内蔵音源AW88298への電力供給を止める
  M5.Power.Axp2101.setALDO1(0);
  M5.Power.Axp2101.setBLDO2(0);

  // CoreS3内蔵カメラへの電力供給を止める
  M5.Power.Axp2101.setALDO3(0);
  M5.Power.Axp2101.setBLDO1(0);
#endif

  M5.Display.setTextSize(2);
  M5.Display.printf("KANTAN Play\nver%lu.%lu.%lu\n\nboot",
                    def::app::app_version_major, def::app::app_version_minor,
                    def::app::app_version_patch);

  {
    static constexpr const uint8_t aw9523_i2c_addr = 0x58;
    static constexpr const uint8_t port0_reg = 0x02;
    static constexpr const uint8_t port1_reg = 0x03;
    static constexpr const uint32_t port0_bitmask_bus_en = 0b00000010; // BUS EN
    static constexpr const uint32_t port1_bitmask_boost =
        0b10000000; // BOOST_EN

    uint8_t buf[2];
    M5.In_I2C.readRegister(aw9523_i2c_addr, port0_reg, buf, sizeof(buf),
                           100000);
    uint8_t bus_en_on = buf[0] | port0_bitmask_bus_en;
    uint8_t boost_on = buf[1] | port1_bitmask_boost;
    uint8_t bus_en_off = bus_en_on & ~port0_bitmask_bus_en;

    M5.In_I2C.writeRegister8(aw9523_i2c_addr, port0_reg, bus_en_off, 100000);
    M5.In_I2C.writeRegister8(aw9523_i2c_addr, port1_reg, boost_on, 100000);

    // BUS_ENをオンにした直後に短時間に電流が大量に流れるのを避けるため、
    // 高速にオンオフを繰り返して、電流の立ち上がりを抑える。
    // これをしないとバッテリ電圧が低い状況下では起動に失敗することがある。
    for (int i = 0; i < 256; ++i) {
      M5.In_I2C.writeRegister8(aw9523_i2c_addr, port0_reg, bus_en_off, 400000);
      M5.In_I2C.writeRegister8(aw9523_i2c_addr, port0_reg, bus_en_on, 400000);
      m5gfx::delayMicroseconds(i);
    }
  }

  M5.Power.setChargeCurrent(200);

  log_memory(1);

  system_registry = new system_registry_t();
  auto task_spi = new task_spi_t();
  auto task_i2c = new task_i2c_t();
  auto task_i2s = new task_i2s_t();
  auto task_midi = new task_midi_t();
  auto task_wifi = new task_wifi_t();
  auto task_port_a = new task_port_a_t();
  auto task_port_b = new task_port_b_t();
  auto task_operator = new task_operator_t();
  auto task_commander = new task_commander_t();
  auto task_kantanplay = new task_kantanplay_t();
  auto task_serial_listener = new task_serial_listener_t();

  log_memory(2);
  M5.delay(8);
  M5.Display.print(".");
  system_registry->init();
  log_memory(3);
  M5.delay(8);
  M5.Display.print(".");
  task_i2s->start();
  log_memory(4);
  M5.delay(8);
  M5.Display.print(".");
  if (!task_i2c->start()) {
    M5.Display.print("\nhardware not found.\n");
    M5.delay(4096);
    M5.Power.powerOff();
  }
  log_memory(5);
  M5.delay(8);
  M5.Display.print(".");
  task_midi->start();
  log_memory(6);
  M5.delay(8);
  M5.Display.print(".");
  task_kantanplay->start();
  log_memory(7);
  M5.delay(8);
  M5.Display.print(".");
  task_operator->start();
  log_memory(8);
  M5.delay(8);
  M5.Display.print(".");
  task_commander->start();
  log_memory(9);
  M5.delay(8);
  M5.Display.print(".");
  task_port_a->start();
  log_memory(10);
  M5.delay(8);
  M5.Display.print(".");
  task_port_b->start();
  log_memory(10);
  M5.delay(8);
  M5.Display.print(".");
  task_serial_listener->start();

#if !defined(M5UNIFIED_PC_BUILD)
  log_memory(11);
  m5gfx::heap_free(dummy);
#endif

  log_memory(11);
  M5.delay(8);
  M5.Display.print(".");
  task_wifi->start();
  log_memory(12);
  M5.delay(8);
  M5.Display.print(".");
  task_spi->start();
  log_memory(13);

  system_registry->operator_command.addQueue(
      {def::command::system_control, def::command::sc_boot});
}
}; // namespace kanplay_ns

void setup() { kanplay_ns::init(); }

void loop() {
  M5.delay(100);
  M5.update(); // ROS2モードでもボタン入力を拾えるようにする
}
