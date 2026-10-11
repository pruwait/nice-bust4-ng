#include "test_util.h"

TEST(all_vectors_decode_and_reencode) {
  for (unsigned i = 0; i < kVectorCount; i++) {
    auto b = hex(kVectors[i].hex);
    Frame f;
    ParseError e = decode(b.data(), b.size(), f);
    if (e != ParseError::OK) { printf("  вектор %u (%s): ошибка %d\n", i, kVectors[i].label, (int) e); g_failures++; continue; }
    CHECK(enc(f) == b);
  }
  CHECK(kVectorCount >= 150);
}

TEST(any_single_byte_corruption_is_detected) {
  for (unsigned i = 0; i < kVectorCount; i++) {
    auto b = hex(kVectors[i].hex);
    for (size_t k = 0; k < b.size(); k++) {
      auto c = b; c[k] ^= 0x01;
      Frame f;
      if (decode(c.data(), c.size(), f) == ParseError::OK) { printf("  вектор %u байт %zu не обнаружен\n", i, k); g_failures++; }
    }
  }
}

TEST(builders_reproduce_vectors) {
  Address ctl(0, 3), esp(0, 0x66), ov(0, 0x81), all(0, 0xFF);
  CHECK(enc(make_cmd(all, esp, cmd::SBS)) == V("SBS (шаг за шагом)"));
  CHECK(enc(make_cmd(all, esp, cmd::CLOSE)) == hex("55 0C 00 FF 00 66 01 05 9D 01 82 04 64 E3 0C"));
  CHECK(enc(make_dmp(ctl, ov, 0x04, 0x80, op::GET)) == V("GET автозакрывание (80)"));
  uint8_t on = 1;
  CHECK(enc(make_dmp(ctl, ov, 0x04, 0xA2, op::SET, &on, 1)) == V("SET замедление вкл"));
  uint8_t pos[2] = {0x07, 0xD8};
  CHECK(enc(make_dmp(ctl, ov, 0x04, 0x21, op::SET, pos, 2)) == V("SET частичное открывание 1 = 07D8"));
  uint8_t z3[3] = {0, 0, 0};
  CHECK(enc(make_dmp(ctl, ov, 0x04, 0x18, op::GET, z3, 3)) == V("GET регистр 18, индекс 0"));
  uint8_t four = 4;
  CHECK(enc(make_dmp(ctl, ov, 0x00, 0x10, op::GET, &four, 1, 0x20)) == V("запрос, offset 20"));
  CHECK(enc(make_dmp(Address(0xFF, 0xFF), Address(0x50, 0x90), 0x00, 0xF0, op::SET)) == V("перезагрузка в загрузчик"));
  CHECK(enc(make_dmp(all, ov, 0x00, 0x04, op::GET)) == V("«кто в сети» на 00 FF"));
}

TEST(encode_limits) {
  Frame f = make_dmp(Address(0, 3), Address(0, 0x81), 4, 0x80, op::SET, nullptr, 0);
  uint8_t data[kMaxMsg - 5];
  memset(data, 0xAB, sizeof(data));
  Frame big = make_dmp(Address(0, 3), Address(0, 0x81), 4, 0x80, op::SET, data, sizeof(data));
  CHECK_EQ(big.msg_len, kMaxMsg);
  auto b = enc(big);
  CHECK_EQ(b.size(), kMaxFrame);
  Frame back;
  CHECK(decode(b.data(), b.size(), back) == ParseError::OK);
  CHECK(back == big);
  uint8_t too_big[kMaxMsg - 4];
  memset(too_big, 1, sizeof(too_big));
  Frame bad = make_dmp(Address(0, 3), Address(0, 0x81), 4, 0x80, op::SET, too_big, sizeof(too_big));
  CHECK(enc(bad).empty());
  CHECK(enc(Frame()).empty());
  uint8_t small[4];
  CHECK_EQ(encode(f, small, sizeof(small)), 0u);
}

TEST(dmp_response_parsing) {
  Frame f = F(V("Ответ: автозакрывание включено"));
  DmpResponse r;
  CHECK(parse_dmp_response(f, r));
  CHECK_EQ(r.device, 4); CHECK_EQ(r.reg, 0x80); CHECK_EQ(r.run, 0x19); CHECK(r.fin());
  CHECK(r.ok()); CHECK_EQ(r.data_len, 1); CHECK_EQ(r.data[0], 1);
  Frame c1 = F(V("Список INF: блок 1"));
  CHECK(parse_dmp_response(c1, r));
  CHECK_EQ(r.run, 0x18); CHECK(!r.fin()); CHECK_EQ(r.next, 0x20); CHECK_EQ(r.data_len, 32);
  Frame last = F(V("последний блок"));
  CHECK(parse_dmp_response(last, r));
  CHECK_EQ(r.run, 0x19); CHECK(r.fin()); CHECK_EQ(r.next, 0xA4); CHECK_EQ(r.data_len, 4);
  Frame err = F(V("Ответ с ошибкой FD"));
  CHECK(parse_dmp_response(err, r));
  CHECK_EQ(r.status, status::UNSUPPORTED); CHECK(!r.ok()); CHECK_EQ(r.data_len, 0);
  Frame req = F(V("GET автозакрывание (80)"));
  CHECK(!parse_dmp_response(req, r));
  DmpRequest q;
  CHECK(parse_dmp_request(req, q)); CHECK_EQ(q.op, op::GET); CHECK_EQ(q.len, 0);
  CHECK(!parse_dmp_request(f, q));
}

TEST(dep_classification) {
  int cmds = 0, rsp = 0, evt = 0, unk = 0;
  for (unsigned i = 0; i < kVectorCount; i++) {
    Frame f = F(hex(kVectors[i].hex));
    if (!f.is_dep()) continue;
    switch (classify_dep(f)) { case DepKind::CMD: cmds++; break; case DepKind::RSP: rsp++; break; case DepKind::EVT: evt++; break; default: unk++; }
  }
  CHECK(cmds >= 5); CHECK(rsp >= 2); CHECK(evt >= 5); CHECK_EQ(unk, 0);
  DepStatusView s;
  CHECK(parse_dep_status(F(V("EVT sub 40: движение при закрывании")), s));
  CHECK_EQ(s.command, 0x40); CHECK_EQ(s.run, 0x84); CHECK_EQ(s.value(), 0x0383); CHECK_EQ(s.status, 0);
  DepCmdView c;
  CHECK(parse_dep_cmd(F(V("SBS (шаг за шагом)")), c));
  CHECK_EQ(c.cmd, cmd::SBS); CHECK_EQ(c.offset, 0x64); CHECK_EQ(c.device, 1);
  CHECK(!parse_dep_cmd(F(V("RSP: команда принята")), c));
  // STA с RUN >= 80 остаётся событием; RSP определяется по маркеру 19
  CHECK(classify_dep(F(V("EVT sub 40: движение при закрывании"))) == DepKind::EVT);
  CHECK(classify_dep(F(V("RSP: команда принята"))) == DepKind::RSP);
  Frame rsp_no_flag = F(V("RSP: команда принята")); rsp_no_flag.msg[2] = 0x04;   // RSP без бита 80 (из логов)
  CHECK(classify_dep(rsp_no_flag) == DepKind::RSP);
}

TEST(response_matching) {
  Frame req = F(V("GET автозакрывание (80)"));
  Frame ok = F(V("Ответ: автозакрывание включено"));
  CHECK(response_matches(req, ok));
  CHECK(!response_matches(req, F(V("откатные (Sliding)"))));              // другой регистр
  CHECK(!response_matches(req, req));                                     // запрос не ответ
  Frame wrong_from = ok; wrong_from.from = Address(0, 9);
  CHECK(!response_matches(req, wrong_from));
  Frame to_other = ok; to_other.to = Address(0, 0x77);
  CHECK(!response_matches(req, to_other));
  Frame wrong_run = ok; wrong_run.msg[2] = 0x29;
  CHECK(!response_matches(req, wrong_run));
  CHECK(response_matches(req, wrong_run, true));
  Frame who = F(V("«кто в сети» на 00 FF"));
  CHECK(response_matches(who, F(V("Ответ приёмника OXI"))));
  CHECK(response_matches(who, F(V("Ответ блока управления"))));
}
