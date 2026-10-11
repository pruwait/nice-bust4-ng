#pragma once
// Приёмный конечный автомат: байты -> проверенные кадры, с повторной синхронизацией.
#include "t4_frame.h"

namespace esphome {
namespace nice_t4 {

class T4Rx {
 public:
  struct Stats {
    uint32_t frames = 0;     // принято корректных кадров
    uint32_t bad_crc = 0;    // кадры с неверной суммой
    uint32_t bad_frame = 0;  // неверная длина или хвост
    uint32_t resyncs = 0;    // потеряно байтов и найдена новая синхронизация
    uint32_t timeouts = 0;   // оборванные по паузе кадры
  };

  void set_byte_timeout(uint32_t ms) { byte_timeout_ms_ = ms; }
  // true, если этот байт завершил корректный кадр (он доступен через frame()).
  bool feed(uint8_t b, uint32_t now_ms);
  // Обработка уже накопленных байтов (после resync там мог остаться ещё один кадр).
  bool next();
  const Frame &frame() const { return frame_; }
  const Stats &stats() const { return stats_; }
  void reset() { n_ = 0; }

 private:
  bool process();
  void drop_to_next_sync();

  uint8_t buf_[kMaxFrame];
  size_t n_ = 0;
  uint32_t last_ms_ = 0;
  uint32_t byte_timeout_ms_ = 10;
  Frame frame_;
  Stats stats_;
};

}  // namespace nice_t4
}  // namespace esphome
