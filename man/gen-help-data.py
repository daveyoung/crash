#!/usr/bin/env python3
"""Generate the C help data from the crash command man pages.

The man pages in this directory are the source of truth for the command
help.  This script renders each man/crash-<cmd>.8 to plain text and emits

  - help_man.c                    (core commands, linked into crash)
  - extensions/<name>_help.h      (in-tree extension commands)

The generated files are committed, so a normal build does not need groff.
Run "make man" after changing a man page.

Requires: man(1) (or mandoc/groff), col(1).
"""
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)

META_RE = re.compile(r'^\.\\"\s+crash-(command|symbol|include):\s*(.*)$')
HEADER_RE = re.compile(r"^\s*[A-Z0-9][-A-Z0-9]*\(\d\)")


def render(path):
    env = dict(os.environ)
    env.update({"MANWIDTH": "80", "MANPAGER": "cat", "PAGER": "cat",
                "GROFF_NO_SGR": "1", "LC_ALL": "C"})
    r = subprocess.run(
        ["man", "--no-hyphenation", "--no-justification", "-l", path],
        env=env, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
    text = r.stdout
    try:
        c = subprocess.run(["col", "-bx"], input=text, stdout=subprocess.PIPE)
        text = c.stdout
    except FileNotFoundError:
        text = text.replace(b"\x08", b"")
    return text.decode("utf-8", "replace").split("\n")


def strip_page(lines):
    while lines and not lines[-1].strip():
        lines.pop()
    if lines and HEADER_RE.match(lines[0]):
        lines.pop(0)
    while lines and not lines[0].strip():
        lines.pop(0)
    if lines and HEADER_RE.match(lines[-1]):
        lines.pop()
    while lines and not lines[-1].strip():
        lines.pop()
    return lines


def synopsis_of(lines, command):
    for i, line in enumerate(lines):
        if line.strip() == "SYNOPSIS":
            parts = []
            for nxt in lines[i + 1:]:
                if not nxt.strip():
                    break
                parts.append(nxt.strip())
            text = re.sub(r"\s+", " ", " ".join(parts)).strip()
            if text.startswith(command):
                text = text[len(command):].strip()
            return text
    return " "


def cstr(s):
    return '"%s"' % s.replace("\\", "\\\\").replace('"', '\\"')


def read_meta(path):
    meta = {"command": None, "symbol": None, "include": None}
    with open(path, encoding="utf-8", errors="replace") as f:
        for line in f:
            if not line.startswith('.\\"'):
                break
            m = META_RE.match(line.rstrip("\n"))
            if m:
                meta[m.group(1)] = m.group(2).strip()
    return meta


def emit_array(symbol, synopsis, lines, static):
    qual = "static " if static else ""
    out = ["%schar *%s[] = {" % (qual, symbol),
           cstr("@MANPAGE@") + ",",
           cstr(synopsis) + ","]
    out += [cstr(l) + "," for l in lines]
    out.append("NULL")
    out.append("};")
    return "\n".join(out)


def main():
    core = []
    ext = {}
    for name in sorted(os.listdir(HERE)):
        if not name.startswith("crash-") or not name.endswith(".8"):
            continue
        path = os.path.join(HERE, name)
        meta = read_meta(path)
        command = meta["command"]
        symbol = meta["symbol"]
        if not command or not symbol:
            print("skipping %s: missing crash-command/crash-symbol" % name,
                  file=sys.stderr)
            continue
        lines = strip_page(render(path))
        synopsis = synopsis_of(lines, command)
        if meta["include"]:
            ext.setdefault(meta["include"], []).append(
                (symbol, synopsis, lines))
        else:
            core.append((symbol, synopsis, lines))

    banner = ("/* Generated from man/*.8 by man/gen-help-data.py.\n"
              " * Do not edit; run \"make man\" after changing a man page.\n"
              " */\n")

    out = os.path.join(ROOT, "help_man.c")
    with open(out, "w", encoding="utf-8") as f:
        f.write(banner)
        f.write('#include "defs.h"\n\n')
        for symbol, synopsis, lines in core:
            f.write(emit_array(symbol, synopsis, lines, static=False))
            f.write("\n\n")
    print("wrote %s (%d commands)" % (out, len(core)))

    for include, entries in sorted(ext.items()):
        out = os.path.join(ROOT, include)
        os.makedirs(os.path.dirname(out), exist_ok=True)
        with open(out, "w", encoding="utf-8") as f:
            f.write(banner)
            for symbol, synopsis, lines in entries:
                f.write(emit_array(symbol, synopsis, lines, static=True))
                f.write("\n\n")
        print("wrote %s (%d commands)" % (out, len(entries)))


if __name__ == "__main__":
    main()
