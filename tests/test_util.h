#pragma once
#include <stdio.h>
#include <string.h>

#include <deque>
#include <string>
#include <vector>

#include "test_vectors.h"
#include "t4_frame.h"

using namespace esphome::nice_t4;

struct TestCase { const char *name; void (*fn)(); };
std::vector<TestCase> &registry();
extern int g_failures;
struct Reg { Reg(const char *n, void (*f)()) { registry().push_back({n, f}); } };

#define TEST(name) static void name(); static Reg reg_##name(#name, name); static void name()
#define CHECK(c) do { if (!(c)) { printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); g_failures++; } } while (0)
#define CHECK_EQ(a, b) do { long long _a = (long long)(a), _b = (long long)(b); if (_a != _b) { printf("  FAIL %s:%d: %s == %s (%lld != %lld)\n", __FILE__, __LINE__, #a, #b, _a, _b); g_failures++; } } while (0)

inline std::vector<uint8_t> hex(const char *s) {
  std::vector<uint8_t> v;
  unsigned cur = 0; int n = 0;
  for (; *s; s++) {
    int d = (*s >= '0' && *s <= '9') ? *s - '0' : (*s >= 'A' && *s <= 'F') ? *s - 'A' + 10 : (*s >= 'a' && *s <= 'f') ? *s - 'a' + 10 : -1;
    if (d < 0) continue;
    cur = cur * 16 + d;
    if (++n == 2) { v.push_back((uint8_t) cur); cur = 0; n = 0; }
  }
  return v;
}

// Кадр из набора векторов по подстроке описания (должна быть уникальной).
inline std::vector<uint8_t> V(const char *label_part) {
  int found = -1, count = 0;
  for (unsigned i = 0; i < kVectorCount; i++)
    if (strstr(kVectors[i].label, label_part)) { found = (int) i; count++; }
  if (count != 1) { printf("  FAIL vector '%s': найдено %d\n", label_part, count); g_failures++; return std::vector<uint8_t>(); }
  return hex(kVectors[found].hex);
}

inline Frame F(const std::vector<uint8_t> &b) {
  Frame f;
  if (decode(b.data(), b.size(), f) != ParseError::OK) { printf("  FAIL decode helper\n"); g_failures++; }
  return f;
}

inline std::vector<uint8_t> enc(const Frame &f) {
  uint8_t buf[kMaxFrame];
  size_t n = encode(f, buf, sizeof(buf));
  return std::vector<uint8_t>(buf, buf + n);
}
