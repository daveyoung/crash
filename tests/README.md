Basic crash tests
=================

`tests/run.sh` is a small smoke-test framework for the crash utility.  It
runs basic commands against a dump or live kernel and reports
PASS/FAIL/SKIP for each.  By default it uses the in-tree `./crash` when
it has been built, and otherwise falls back to the crash binary
installed on the system (found in PATH).  Use `--crash PATH` to test a
specific binary.

Layout
------

    tests/run.sh        driver: options, targets, PASS/FAIL/SKIP
    tests/case.sh       shared judge logic: run one command and decide
                        pass/fail from its output
    tests/cases/*.sh    one file per crash command; output checks

Adding a case
-------------

A case file calls

    register_case COMMAND ROWS_REGEX MIN_ROWS CHECK_FUNCTION

and defines CHECK_FUNCTION using `expect` (an ERE that must match the
output; `$log` and `$cmd` are available).  The existing `sys` case, for
example, is:

    register_case "sys" '^ *[A-Z][A-Z ]+: ' 8 check_sys

    check_sys()
    {
        expect '^ *RELEASE: [0-9]+\.[0-9]+' &&
        expect '^ *CPUS: [0-9]+'
    }

`run.sh` picks up every `cases/*.sh` automatically; no change to the
driver is needed.  A file may register more than one command.

Targets
-------

Dump (default):

    vmcore/$(uname -m)/vmlinux
    vmcore/$(uname -m)/vmcore

The directory is relative to the repository and can be changed with
`--vmcore-dir`, `--vmlinux` and `--vmcore`.  `--arch` selects a
different `$(uname -m)` subdirectory.

Live kernel (`--live`):

    crash <vmlinux> /proc/kcore

The live test needs read access to /proc/kcore (normally root) and a
vmlinux matching the running kernel.  The vmlinux is looked up under
/usr/lib/debug, /boot and /lib/modules; use `--live-vmlinux` to point at
one explicitly.  `--live-image` selects a different memory image, for
example /dev/crash.

Usage
-----

    tests/run.sh
    tests/run.sh --vmcore-dir /path/to/vmcore/x86_64/7.3.0-rc6+
    tests/run.sh --live --live-vmlinux /usr/lib/debug/lib/modules/$(uname -r)/vmlinux

What is checked
---------------

Before the command cases, crash is run once with `quit`.  If it cannot
initialize, that is reported as an unexpected failure and the command
cases for that target are skipped.  Once initialized, any crash error,
warning, or missing/malformed field in a command is likewise an
unexpected failure.

Each command is run in its own crash session after that check.  A case
fails when:

  - crash exits non-zero;
  - crash prints a "crash: ..." or "<command>: ..." error, or a
    "WARNING: ..." line;
  - the output has fewer well-formed data rows than expected (for
    example, `bt` must show at least two `#N` frames);
  - a command-specific field is missing or malformed, such as a numeric
    RELEASE in `sys`, PID 1 in `ps`, or the CHRDEV/BLKDEV tables in
    `dev`.

The exit status alone is not enough: after startup crash reports
command failures on the command's own line (for example `dev: ...`)
and still exits 0 when the session ends.  Matching only a header is not
enough either: the checks count and validate data rows, so a command
that prints a header and nothing else fails.

The command list and its checks live in `tests/cases/`; the shared
logic they run through is in `tests/case.sh`.

This is a first step.  Possible follow-ups: a single crash session with
per-command markers (faster on large dumps), TAP output, golden
reference output, more architectures, and hooking the script into
ci-tests/.
