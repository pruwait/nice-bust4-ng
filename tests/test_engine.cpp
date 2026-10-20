#include "test_util.h"
#include "t4_chunk.h"
#include "t4_engine.h"

struct FakeTransport : Transport {
  struct Tx { uint32_t t; bool brk; std::vector<uint8_t> data; };
  uint32_t t = 1000;
  std::deque<uint8_t> rx;
  std::vector<Tx> log;
  uint32_t now_ms() override { return t; }
  void send_break() override { log.push_back({t, true, {}}); }
  void write(const uint8_t *d, size_t n) override { log.push_back({t, false, std::vector<uint8_t>(d, d + n)}); }
  int read_byte() override { if (rx.empty()) return -1; int b = rx.front(); rx.pop_front(); return b; }
  void inject(const std::vector<uint8_t> &v, bool brk = true) { if (brk) rx.push_back(0); for (uint8_t b : v) rx.push_back(b); }
  std::vector<Tx> frames() const { std::vector<Tx> r; for (auto &x : log) if (!x.brk) r.push_back(x); return r; }
  size_t breaks() const { size_t n = 0; for (auto &x : log) if (x.brk) n++; return n; }
};

struct Rec {
  std::vector<Result> results;
  std::vector<Frame> frames;
  std::vector<uint32_t> times;
  FakeTransport *tr = nullptr;
};
static void on_result(void *ctx, Result r, const Frame *f) {
  Rec *rec = static_cast<Rec *>(ctx);
  rec->results.push_back(r);
  rec->times.push_back(rec->tr ? rec->tr->t : 0);
  if (f) rec->frames.push_back(*f);
}
static void run_for(T4Engine &e, FakeTransport &tr, uint32_t ms, uint32_t step = 10) {
  for (uint32_t i = 0; i < ms; i += step) { tr.t += step; e.loop(); }
}

static const Address kSelf(0, 0x81), kCtl(0, 3);

TEST(engine_sends_immediately_when_bus_idle) {
  FakeTransport tr; T4Engine e(tr, kSelf); Rec rec; rec.tr = &tr;
  CHECK(e.submit(F(V("GET автозакрывание (80)")), RequestOptions(), on_result, &rec));
  e.loop();
  CHECK_EQ(tr.breaks(), 1u);
  auto fr = tr.frames();
  CHECK_EQ(fr.size(), 1u);
  CHECK(fr[0].data == V("GET автозакрывание (80)"));
  CHECK_EQ(fr[0].t, 1000u);
}

TEST(engine_response_completes_request_and_spaces_next_frame) {
  FakeTransport tr; T4Engine e(tr, kSelf); Rec r1, r2; r1.tr = r2.tr = &tr;
  e.submit(F(V("GET автозакрывание (80)")), RequestOptions(), on_result, &r1);
  e.submit(F(V("GET замедление (A2)")), RequestOptions(), on_result, &r2);
  e.loop();
  tr.inject(V("Ответ: автозакрывание включено"));
  e.loop();
  CHECK_EQ(r1.results.size(), 1u);
  CHECK(r1.results[0] == Result::RESPONSE);
  CHECK_EQ(r1.frames[0].msg[5], 1);
  CHECK_EQ(tr.frames().size(), 1u);                  // второй кадр ждёт паузы на шине
  run_for(e, tr, 200);
  auto fr = tr.frames();
  CHECK_EQ(fr.size(), 2u);
  CHECK_EQ(fr[1].t, 1100u);                          // ровно через 100 мс после последнего приёма
  CHECK(fr[1].data == V("GET замедление (A2)"));
}

TEST(engine_retries_then_times_out) {
  FakeTransport tr; T4Engine e(tr, kSelf); Rec rec; rec.tr = &tr;
  e.submit(F(V("GET автозакрывание (80)")), RequestOptions(), on_result, &rec);
  e.loop();
  run_for(e, tr, 890);
  CHECK_EQ(rec.results.size(), 0u);
  CHECK_EQ(tr.frames().size(), 3u);
  run_for(e, tr, 10);
  CHECK_EQ(rec.results.size(), 1u);
  CHECK(rec.results[0] == Result::TIMEOUT);
  CHECK_EQ(rec.times[0], 1900u);
  auto fr = tr.frames();
  CHECK_EQ(fr.size(), 3u);
  CHECK_EQ(fr[0].t, 1000u); CHECK_EQ(fr[1].t, 1300u); CHECK_EQ(fr[2].t, 1600u);
  CHECK_EQ(e.stats().tx_retries, 2u); CHECK_EQ(e.stats().tx_timeouts, 1u);
}

TEST(engine_ignores_own_echo) {
  FakeTransport tr; T4Engine e(tr, kSelf); Rec rec; rec.tr = &tr;
  int events = 0;
  e.set_event_callback([](void *c, const Frame &) { (*static_cast<int *>(c))++; }, &events);
  e.submit(F(V("GET автозакрывание (80)")), RequestOptions(), on_result, &rec);
  e.loop();
  tr.inject(V("GET автозакрывание (80)"));           // эхо: from == self
  e.loop();
  CHECK_EQ(e.stats().echoes, 1u);
  CHECK_EQ(events, 0);
  CHECK_EQ(rec.results.size(), 0u);
  tr.inject(V("Ответ: автозакрывание включено"));
  e.loop();
  CHECK_EQ(rec.results.size(), 1u);
}

TEST(engine_delivers_unmatched_frames_as_events) {
  FakeTransport tr; T4Engine e(tr, kSelf);
  std::vector<Frame> evs;
  e.set_event_callback([](void *c, const Frame &f) { static_cast<std::vector<Frame> *>(c)->push_back(f); }, &evs);
  tr.inject(V("EVT sub 40: движение при закрывании"));
  e.loop();
  CHECK_EQ(evs.size(), 1u);
  CHECK(evs[0].from == kCtl);
  CHECK(classify_dep(evs[0]) == DepKind::EVT);
}

TEST(engine_fire_and_forget_reports_sent) {
  FakeTransport tr; T4Engine e(tr, kSelf); Rec rec; rec.tr = &tr;
  RequestOptions o; o.expect_response = false;
  e.submit(make_cmd(Address(0, 0xFF), Address(0, 0x66), cmd::SBS), o, on_result, &rec);
  e.loop();
  CHECK_EQ(rec.results.size(), 1u);
  CHECK(rec.results[0] == Result::SENT);
  CHECK(tr.frames()[0].data == V("SBS (шаг за шагом)"));
  CHECK_EQ(e.queued(), 0u);
}

TEST(engine_collects_broadcast_responses_until_window_ends) {
  FakeTransport tr; T4Engine e(tr, kSelf); Rec rec; rec.tr = &tr;
  RequestOptions o; o.multi_response = true; o.retries = 0; o.timeout_ms = 300;
  e.submit(F(V("«кто в сети» на 00 FF")), o, on_result, &rec);
  e.loop();
  tr.inject(V("Ответ приёмника OXI"));
  tr.inject(V("Ответ блока управления"));
  e.loop();
  CHECK_EQ(rec.results.size(), 2u);
  CHECK(rec.frames[0].from == Address(0, 0x0A));
  CHECK(rec.frames[1].from == Address(0, 0x03));
  run_for(e, tr, 300);
  CHECK_EQ(rec.results.size(), 3u);
  CHECK(rec.results[2] == Result::WINDOW_END);
}

TEST(engine_queue_limit) {
  FakeTransport tr; T4Engine e(tr, kSelf);
  Frame f = F(V("GET автозакрывание (80)"));
  for (int i = 0; i < T4Engine::kQueueSize; i++) CHECK(e.submit(f, RequestOptions(), nullptr, nullptr));
  CHECK(!e.submit(f, RequestOptions(), nullptr, nullptr));
  CHECK_EQ(e.queued(), T4Engine::kQueueSize);
}

TEST(engine_survives_corrupt_frame_and_accepts_next) {
  FakeTransport tr; T4Engine e(tr, kSelf); Rec rec; rec.tr = &tr;
  e.submit(F(V("GET автозакрывание (80)")), RequestOptions(), on_result, &rec);
  e.loop();
  auto bad = V("Ответ: автозакрывание включено"); bad[12] ^= 0x40;
  tr.inject(bad);
  tr.inject(V("Ответ: автозакрывание включено"));
  e.loop();
  CHECK_EQ(rec.results.size(), 1u);
  CHECK_EQ(e.rx_stats().bad_crc, 1u);
}

struct Chain { T4Engine *e; Rec *rec; int n; };
static void chain_cb(void *ctx, Result r, const Frame *) {
  Chain *c = static_cast<Chain *>(ctx);
  c->rec->results.push_back(r);
  if (c->n++ == 0) c->e->submit(F(V("GET замедление (A2)")), RequestOptions(), chain_cb, c);   // повторный submit из колбэка
}

TEST(engine_allows_submit_from_callback) {
  FakeTransport tr; T4Engine e(tr, kSelf); Rec rec; Chain ch{&e, &rec, 0};
  e.submit(F(V("GET автозакрывание (80)")), RequestOptions(), chain_cb, &ch);
  e.loop();
  tr.inject(V("Ответ: автозакрывание включено"));
  e.loop();
  run_for(e, tr, 150);
  tr.inject(V("Ответ: замедление включено"));
  e.loop();
  CHECK_EQ(rec.results.size(), 2u);
  CHECK_EQ(e.queued(), 0u);
}

TEST(chunk_reader_reads_whole_inf_list) {
  uint8_t buf[256];
  ChunkReader cr(buf, sizeof(buf));
  uint8_t menu = 4;
  CHECK(cr.begin(kCtl, kSelf, 0x00, 0x10, &menu, 1));
  const char *req_labels[] = {"запрос, offset 00", "запрос, offset 20", "запрос, offset 40", "запрос, offset 60", "запрос, offset 80", "запрос, offset A0"};
  const char *rsp_labels[] = {"блок 1", "блок 2", "блок 3", "блок 4", "блок 5", "последний блок"};
  std::vector<uint8_t> expect;
  for (int i = 0; i < 6; i++) {
    CHECK(enc(cr.next_request()) == V(req_labels[i]));
    Frame resp = F(V(rsp_labels[i]));
    DmpResponse r; parse_dmp_response(resp, r);
    expect.insert(expect.end(), r.data, r.data + r.data_len);
    ChunkReader::State s = cr.feed(resp);
    CHECK(s == (i < 5 ? ChunkReader::State::CONTINUE : ChunkReader::State::DONE));
  }
  CHECK_EQ(cr.size(), 164u);
  CHECK(std::vector<uint8_t>(buf, buf + cr.size()) == expect);
}

TEST(chunk_reader_reports_errors) {
  uint8_t buf[16];
  ChunkReader cr(buf, sizeof(buf));
  CHECK(cr.begin(kCtl, kSelf, 0x00, 0x10));
  CHECK(cr.feed(F(V("Список INF: блок 1"))) == ChunkReader::State::ERROR);
  CHECK(cr.error() == ChunkReader::Error::OVERFLOW);
  ChunkReader c2(buf, sizeof(buf));
  CHECK(c2.begin(kCtl, kSelf, 0x00, 0x00));
  CHECK(c2.feed(F(V("Ответ с ошибкой FD"))) == ChunkReader::State::ERROR);
  CHECK(c2.error() == ChunkReader::Error::DEVICE_STATUS);
  CHECK_EQ(c2.device_status(), 0xFD);
  ChunkReader c3(buf, sizeof(buf));
  CHECK(c3.begin(kCtl, kSelf, 0x04, 0x99));
  CHECK(c3.feed(F(V("Ответ: автозакрывание включено"))) == ChunkReader::State::ERROR);   // другой регистр
  CHECK(c3.error() == ChunkReader::Error::BAD_RESPONSE);
}

TEST(engine_sends_dep_command_exactly_once) {
  // Регрессия: команды DEP не имеют ответа DMP, поэтому повторов быть не должно (SBS не «щёлкает» трижды).
  FakeTransport tr; T4Engine e(tr, Address(0, 0x66)); Rec rec; rec.tr = &tr;
  RequestOptions o;                      // по умолчанию expect_response = true, retries = 2
  o.timeout_ms = 500;
  CHECK(e.submit(make_cmd(Address(0, 0xFF), Address(0, 0x66), cmd::SBS), o, on_result, &rec));
  e.loop();
  run_for(e, tr, 3000);
  CHECK_EQ(tr.frames().size(), 1u);
  CHECK(tr.frames()[0].data == V("SBS (шаг за шагом)"));
  CHECK_EQ(rec.results.size(), 1u);
  CHECK(rec.results[0] == Result::SENT);
  CHECK_EQ(e.stats().tx_retries, 0u);
}

TEST(engine_set_self_changes_echo_filter) {
  FakeTransport tr; T4Engine e(tr, Address(0, 0x01));
  int events = 0;
  e.set_event_callback([](void *c, const Frame &) { (*static_cast<int *>(c))++; }, &events);
  e.set_self(Address(0, 0x03));
  tr.inject(V("EVT sub 40: движение при закрывании"));   // from 00 03 теперь «своё» и считается эхом
  e.loop();
  CHECK_EQ(events, 0);
  CHECK_EQ(e.stats().echoes, 1u);
}
