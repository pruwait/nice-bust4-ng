#!/usr/bin/env python3
"""tests/vectors.json -> tests/test_vectors.h (проверенные кадры для тестов кодека)."""
import json, pathlib
root = pathlib.Path(__file__).resolve().parent.parent
v = json.load(open(root / "tests" / "vectors.json", encoding="utf-8"))
esc = lambda s: s.replace("\\", "\\\\").replace('"', '\\"')
lines = ["// Файл создан tools/gen_test_vectors.py, вручную не править.", "#pragma once", "",
         "struct Vector { const char* section; const char* label; const char* hex; };", "static const Vector kVectors[] = {"]
lines += [f'    {{"{esc(x["section"])}", "{esc(x["label"])}", "{x["hex"]}"}},' for x in v]
lines += ["};", f"static const unsigned kVectorCount = {len(v)};", ""]
open(root / "tests" / "test_vectors.h", "w", encoding="utf-8").write("\n".join(lines))
print("generated test_vectors.h:", len(v))
