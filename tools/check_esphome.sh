#!/bin/sh
# Проверка yaml-конфигураций и C++ компонента против настоящих заголовков ESPHome (host-платформа).
# Нужен установленный esphome (pip install esphome) и g++.
set -e
cd "$(dirname "$0")/.."
ESPHOME="${ESPHOME:-esphome}"

for f in esp32 esp8266; do
  $ESPHOME config tests/yaml/$f.yaml > /dev/null
  echo "config ok: $f"
done
for f in bad_text_type bad_uart bad_address; do
  if $ESPHOME config tests/yaml/$f.yaml > /dev/null 2>&1; then
    echo "ОШИБКА: tests/yaml/$f.yaml должен отвергаться"; exit 1
  fi
  echo "отвергнут как и должен: $f"
done

$ESPHOME compile --only-generate tests/yaml/host.yaml > /dev/null
SRC=tests/yaml/.esphome/build/gate/src
LOG=$(mktemp)
FAILED=0
for f in $SRC/esphome/components/nice_t4/*.cpp $SRC/main.cpp; do
  g++ -std=gnu++20 -fsyntax-only -Wall -Wextra -DUSE_HOST -DESPHOME_LOG_LEVEL=5 \
      -I"$SRC" -I"$SRC/esphome/components" "$f" 2> "$LOG" || { cat "$LOG"; FAILED=1; }
  if grep -E "components/nice_t4/.*(warning|error)" "$LOG" > /dev/null; then cat "$LOG"; FAILED=1; fi
done
rm -f "$LOG"
[ "$FAILED" = 0 ] && echo "синтаксис C++ против заголовков ESPHome: ok"
exit $FAILED
