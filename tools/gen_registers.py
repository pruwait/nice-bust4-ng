#!/usr/bin/env python3
"""data/registers.json и data/dep_commands.json -> t4_registers.h и t4_cmds.h."""
import json, re, pathlib
root = pathlib.Path(__file__).resolve().parent.parent
out = root / "components" / "nice_t4"
GROUPS = [(4, "Reg", "блок управления (DEVICE 04)"), (0, "StdReg", "общее устройство (DEVICE 00)"),
          (0x0A, "RadioReg", "приёмник (DEVICE 0A)"), (0x11, "PsuReg", "блок питания (DEVICE 11)")]
HDR = "// Файл создан tools/gen_registers.py, вручную не править.\n#pragma once\n#include <stdint.h>\n\nnamespace esphome {\nnamespace nice_t4 {\n\n"
FTR = "}  // namespace nice_t4\n}  // namespace esphome\n"

def ident(s):
    s = re.sub(r"[^A-Za-z0-9_]", "_", s).upper()
    return s if re.match(r"[A-Z]", s) else "X_" + s

regs = json.load(open(root / "data" / "registers.json", encoding="utf-8"))
txt = HDR
for dev, name, comment in GROUPS:
    rows = sorted((r for r in regs if r["device"] == dev), key=lambda r: r["code"])
    seen = set(); codes = set()
    txt += f"// {comment}, {len(rows)} регистров\nenum class {name} : uint8_t {{\n"
    for r in rows:
        c = ident(r["const"])
        assert c not in seen and r["code"] not in codes, (name, c)
        seen.add(c); codes.add(r["code"])
        txt += f"  {c} = 0x{r['code']:02X},\n"
    txt += "};\n\n"
open(out / "t4_registers.h", "w", encoding="utf-8").write(txt + FTR)

dep = json.load(open(root / "data" / "dep_commands.json", encoding="utf-8"))
txt = HDR + f"// Команды DEP (T4ActionCode), {len(dep)} команд\nenum class DepCmd : uint8_t {{\n"
seen = set()
for r in sorted(dep, key=lambda r: r["cmd"]):
    c = ident(r["const"]); assert c not in seen; seen.add(c)
    txt += f"  {c} = 0x{r['cmd']:02X},\n"
open(out / "t4_cmds.h", "w", encoding="utf-8").write(txt + "};\n\n" + FTR)

# command_codes.py: имена команд DEP для yaml (button.command)
names = {}
for r in sorted(dep, key=lambda r: r["cmd"]):
    n = re.sub(r"^CTRL_", "", r["const"]).lower()
    assert n not in names, n
    names[n] = r["cmd"]
body = "# Файл создан tools/gen_registers.py, вручную не править.\nDEP_COMMANDS = {\n"
body += "".join(f'    "{n}": 0x{c:02X},\n' for n, c in names.items()) + "}\n"
open(out / "command_codes.py", "w", encoding="utf-8").write(body)
print("generated t4_registers.h, t4_cmds.h, command_codes.py")
