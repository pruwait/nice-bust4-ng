#include "test_util.h"
#include "t4_rx.h"

static std::vector<Frame> feed_all(T4Rx &rx, const std::vector<uint8_t> &bytes, uint32_t &t, uint32_t dt = 1) {
  std::vector<Frame> out;
  for (uint8_t b : bytes) {
    t += dt;
    if (rx.feed(b, t)) { out.push_back(rx.frame()); while (rx.next()) out.push_back(rx.frame()); }
  }
  return out;
}

TEST(rx_all_vectors_with_breaks) {
  T4Rx rx; uint32_t t = 0;
  unsigned n = 0;
  for (unsigned i = 0; i < kVectorCount; i++) {
    auto b = hex(kVectors[i].hex);
    std::vector<uint8_t> stream; stream.push_back(0x00); stream.insert(stream.end(), b.begin(), b.end());
    auto fr = feed_all(rx, stream, t);
    if (fr.size() != 1 || !(fr[0] == F(b))) { printf("  вектор %u не принят\n", i); g_failures++; } else n++;
    t += 120;
  }
  CHECK_EQ(n, kVectorCount);
  CHECK_EQ(rx.stats().frames, kVectorCount);
  CHECK_EQ(rx.stats().bad_crc + rx.stats().bad_frame + rx.stats().resyncs, 0u);
}

TEST(rx_ignores_noise_before_sync) {
  T4Rx rx; uint32_t t = 0;
  std::vector<uint8_t> s = {0x00, 0x00, 0x12, 0x34, 0xFF};
  auto b = V("GET автозакрывание (80)");
  s.insert(s.end(), b.begin(), b.end());
  auto fr = feed_all(rx, s, t);
  CHECK_EQ(fr.size(), 1u);
}

TEST(rx_bad_crc_is_dropped_next_frame_ok) {
  T4Rx rx; uint32_t t = 0;
  auto bad = V("GET автозакрывание (80)"); bad[8] ^= 0x10;
  auto good = V("Ответ: автозакрывание включено");
  std::vector<uint8_t> s = bad; s.insert(s.end(), good.begin(), good.end());
  auto fr = feed_all(rx, s, t);
  CHECK_EQ(fr.size(), 1u);
  CHECK(fr[0] == F(good));
  CHECK_EQ(rx.stats().bad_crc, 1u);
}

TEST(rx_resync_after_truncated_frame) {
  T4Rx rx; uint32_t t = 0;
  auto a = V("GET автозакрывание (80)");
  auto good = V("Ответ: автозакрывание включено");
  std::vector<uint8_t> s(a.begin(), a.begin() + 5);        // оборванный кадр
  s.insert(s.end(), good.begin(), good.end());
  auto fr = feed_all(rx, s, t);
  CHECK_EQ(fr.size(), 1u);
  CHECK(fr[0] == F(good));
  CHECK(rx.stats().resyncs >= 1u);
}

TEST(rx_returns_leftover_frames_via_next) {
  T4Rx rx; uint32_t t = 0;
  auto a = V("GET автозакрывание (80)");
  std::vector<uint8_t> s = {0x55, 0x3F, 0x00, 0x00};       // заявлен кадр на 66 байт
  for (int i = 0; i < 4; i++) s.insert(s.end(), a.begin(), a.end());
  auto fr = feed_all(rx, s, t);
  CHECK_EQ(fr.size(), 4u);
  for (auto &f : fr) CHECK(f == F(a));
}

TEST(rx_byte_timeout_drops_partial_frame) {
  T4Rx rx; uint32_t t = 0;
  auto a = V("GET автозакрывание (80)");
  std::vector<uint8_t> head(a.begin(), a.begin() + 6);
  feed_all(rx, head, t);
  t += 50;
  auto good = V("Ответ: автозакрывание включено");
  auto fr = feed_all(rx, good, t);
  CHECK_EQ(fr.size(), 1u);
  CHECK_EQ(rx.stats().timeouts, 1u);
}

TEST(rx_back_to_back_frames_without_break) {
  T4Rx rx; uint32_t t = 0;
  auto a = V("GET автозакрывание (80)"), b = V("Ответ: автозакрывание включено");
  std::vector<uint8_t> s = a; s.insert(s.end(), b.begin(), b.end());
  auto fr = feed_all(rx, s, t);
  CHECK_EQ(fr.size(), 2u);
  CHECK(fr[0] == F(a)); CHECK(fr[1] == F(b));
}
