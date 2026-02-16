// SPDX-License-Identifier: MIT
// Copyright (c) 2025 InstaChord Corp.

#ifndef KANPLAY_TASK_SERIAL_LISTENER_HPP
#define KANPLAY_TASK_SERIAL_LISTENER_HPP

#include "system_registry.hpp"

namespace kanplay_ns {

/**
 * @brief ROS2との通信を司るブリッジタスク
 * sample.py のロジックを KANTAN Play 向けに最適化して移植
 */
class task_serial_listener_t {
public:
  void start(void);

private:
  static void task_func(void *arg);

  // シリアルコマンドハンドラ
  void handle_command(const char *cmd);

  // --- 各サウンドアクション (sample.py からの移植) ---
  void action_ready();    // 起動
  void action_start();    // 開始カウント
  void action_move();     // 動作中（ベース）
  void action_grip();     // グリップ（スネア）
  void action_thinking(); // 考え中 (LLM)
  void action_alert();    // 警告（不協和音）
  void action_finish();   // 完了（エンディング）

  // --- 内部ヘルパー ---
  void note_on(uint8_t ch, uint8_t note, uint8_t vel);
  void note_off(uint8_t ch, uint8_t note);
  void program_change(uint8_t ch, uint8_t prg);
  void set_visual(uint32_t color, const char *text);
  void draw_ros2_ui(const char *status);
};

}; // namespace kanplay_ns

#endif
