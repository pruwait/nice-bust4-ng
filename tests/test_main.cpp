#include "test_util.h"

int g_failures = 0;
std::vector<TestCase> &registry() { static std::vector<TestCase> r; return r; }

int main() {
  int failed_tests = 0;
  for (auto &t : registry()) {
    int before = g_failures;
    t.fn();
    printf("%s %s\n", g_failures == before ? "ok  " : "FAIL", t.name);
    if (g_failures != before) failed_tests++;
  }
  printf("\n%zu tests, %d failed, %d checks failed\n", registry().size(), failed_tests, g_failures);
  return g_failures ? 1 : 0;
}
