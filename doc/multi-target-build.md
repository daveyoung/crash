# Multi-target crash builds

Implementation notes for the optional debug-target selection:

    make                            # crash for the host
    make target=ARM64               # crash-arm64
    make target=ARM64,LOONGARCH64   # crash-arm64, crash-loongarch64
    make target=X86_64,ARM64        # crash-x86_64, crash-arm64
    make target=all                 # every host-word-size target

Whenever `target=` is used the binary is `crash-<arch>`, including the host; a
plain `make` still produces `crash`.

## 1.  Design

The one thing worth optimising is the embedded gdb: N targets must not cost N
gdb builds.  Everything else is deliberately kept as close to the original
single-target build as possible.

### 1.1  Front end

`Makefile` parses `target=`, maps `all` to the host-word-size list (mirroring
configure.c: on x86_64, `ARM64 PPC64 RISCV64 LOONGARCH64 X86_64`), and runs
one recursive make per target:

    make MULTI=1 TARGET=ARM64 PROGRAM=crash-arm64 TARGET_CFLAGS='...' multi-one

The host, when it is in the list, is built last so the tree ends configured
for it.  `multi-one` generates `build_data.c` (the multi path bypasses
configure) and runs `gdb_merge`.  A list mixing 32-bit and 64-bit targets is
rejected; a single 32-bit target keeps the old per-target gdb path.

### 1.2  One multi-target gdb

Every 64-bit build configures the embedded gdb with
`--enable-targets=<requested targets>` (the host is implicit), not `all`.
`gdb_merge` records the flags in `gdb-16.2/.configured` and reuses the tree
when they match.  The gdb top-level configure does not document the option
but forwards `--enable-*` to its sub-configures, which is confirmed by
`gdb/config.status` and by the linked binary containing
`aarch64_elf64_le_vec` and `loongarch_elf64_vec`.

`--enable-targets` accepts a list of gdb target aliases and builds only those
arch backends, BFD vectors and disassemblers.  The mapping used here is
`ARM64->aarch64-linux`, `PPC64->powerpc64le-linux`,
`RISCV64->riscv64-linux`, `LOONGARCH64->loongarch64-linux` (and `X86_64->
x86_64-linux` when it is a cross target, e.g. on an ARM64 host).  A plain
`make` passes no list and gets the host-only gdb, so its binary stays the
usual size.

Because the flags now depend on the requested set, changing the target set
(e.g. `target=ARM64` then `target=RISCV64`) reconfigures and rebuilds the
embedded gdb once; a single `target=A,B` invocation still configures it once
for both.

### 1.3  Per-target objects

Crash's target is compile-time: `-D<TARGET>` reaches every object through
`defs.h`.  A `.build_target` stamp records the target.  When it differs,
`gdb_merge` removes the crash objects and the three target-dependent gdb
objects (`gdb/gdb/symtab.o`, `gdb/gdb/frame.o`, `crash_target.o`) and the
build recreates them for the new target.

This is the simple "rebuild on switch" model: no per-target object directory,
no per-object caching.  Switching targets costs a recompile of the 72 crash
objects plus the three gdb glue objects, which is small next to the one-time
gdb build.

### 1.4  Passing the per-target values to the link

The final link is done by gdb's link rule, which starts crash's `library`
target via `make -C ../..`.  gdb's top-level Makefile sets `MAKEOVERRIDES=`,
so command-line `TARGET`/`PROGRAM` do not survive that recursion, and crash's
`Makefile.inc` would otherwise win.  `gdb_merge` therefore exports

    CRASH_BUILD_TARGET / CRASH_BUILD_PROGRAM / CRASH_BUILD_TARGET_CFLAGS

and the Makefile, after `-include Makefile.inc`, picks them up:

    TARGET := $(or $(CRASH_BUILD_TARGET),$(TARGET))
    TARGET_CFLAGS := $(or $(CRASH_BUILD_TARGET_CFLAGS),$(TARGET_CFLAGS))
    PROGRAM := $(or $(CRASH_BUILD_PROGRAM),$(PROGRAM))

No change to `gdb-16.2.patch` is needed.  The crash archive is still built
during the gdb link, after gdb's `make` has generated `bfd/config.h` (which
`kernel.o` includes).

## 2.  One binary vs. many

`gdb` is designed to be one multi-architecture binary; crash is not.  Crash's
target is baked in by `-D<TARGET>`, so a single runtime-switching binary would
require converting every arch-conditional in `defs.h` and the arch files into
runtime dispatch — a much larger change than this build feature.

| | one multi-target crash binary | per-target binaries (implemented) |
|---|---|---|
| crash-side change | re-architect all arch-conditional code | build-only change |
| regression risk | high | low: each binary is the normal build |
| runtime cost | dispatch on hot paths | none |
| artifacts | one file | one per target |

The per-target option is the low-risk one; the shared multi-target gdb is
needed either way.

## 3.  Verification

| command | result |
|---|---|
| `make` | `crash` (X86_64) |
| `make target=ARM64` | `crash-arm64` |
| `make target=ARM64,LOONGARCH64` | `crash-arm64`, `crash-loongarch64` |
| `make target=X86_64,ARM64` | `crash-x86_64`, `crash-arm64` |
| `make target=all` | the five host-word-size `crash-<arch>` binaries |
| `make target=X86,ARM64` | rejected (32-bit targets are built one at a time) |

* `build_target` read back from each binary matches its name.
* Switching targets recompiles the objects with the new `-D<TARGET>` and
  refreshes the three gdb glue objects (verified through the per-target
  `frame.o` ARM64 hook and the per-target archives).
* The embedded gdb is reused across invocations (`wipes == 0`), and a plain
  `make` afterwards rebuilds the host `crash` correctly.

## 4.  Limitations

* **32-bit targets are separate builds.**  X86/ARM/MIPS on x86_64 (PPC on
  PPC64) are 32-bit and cannot share the 64-bit gdb, so `target=all` excludes
  them and a mixed list is rejected.  A single 32-bit build reconfigures the
  embedded gdb with `-m32`; it also needs 32-bit GMP/MPFR headers on the host
  (`gmp-i386.h` here), which this test host lacked.
* **Rebuild on target switch** (no per-target object cache); the cost is the
  crash objects, not gdb.
* **Binary size.**  Only the requested gdb backends are linked, so a
  host-only build stays around 14 MB and `target=ARM64,LOONGARCH64` is about
  16.5 MB per binary; `target=all` links five sets and is correspondingly
  larger.  (With `--enable-targets=all` every binary was ~36 MB.)
* **Changing the target set reconfigures gdb.**  `gdb-16.2/.configured`
  records the `--enable-targets` list, so going from one target set to a
  different one rebuilds the embedded gdb once.  A single `target=A,B`
  invocation configures it once for both.
* **Sequential across targets** (shared gdb tree and link).
* **`configure.c` still mis-parses a comma list** if invoked directly; only
  the Makefile front end understands lists.
* **`make lzo target=A,B`** does not apply the new feature flag to the
  per-target builds (the multi path runs configure without the `-x` options);
  run `make lzo` first, then `make target=...`.

## 5.  Environment note

The build runs `gcc` through `ccache`; under a sandbox where
`$HOME/.cache/ccache` is read-only, configure probes fail with
"ccache: ... Read-only file system".  Use `CCACHE_DISABLE=1` in that case.
