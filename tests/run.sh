#!/bin/sh
#
# Basic smoke tests for the crash utility.
#
# This file is the driver: command-line handling, target selection, and
# running the cases.  The shared pass/fail logic lives in case.sh; each
# command's checks live in cases/<command>.sh.
#
# To add a test, drop a new tests/cases/*.sh that calls register_case();
# this file does not need to change.
#
# See tests/README.md for the expected layout.

set -u

PROG=$(basename "$0")
SCRIPT_DIR=$(CDPATH= cd "$(dirname "$0")" && pwd)
REPO_DIR=$(dirname "$SCRIPT_DIR")
ARCH=${ARCH:-$(uname -m)}
# Prefer the in-tree build.  When crash has not been built, fall back
# to the system-installed crash.
if [ -z "${CRASH:-}" ]; then
	if [ -x "$REPO_DIR/crash" ]; then
		CRASH=$REPO_DIR/crash
	else
		CRASH=$(command -v crash 2>/dev/null) || CRASH=
	fi
fi
VMCORE_DIR=${VMCORE_DIR:-$REPO_DIR/vmcore/$ARCH}
VMLINUX=${VMLINUX:-}
VMCORE=${VMCORE:-}
LIVE=0
LIVE_VMLINUX=${LIVE_VMLINUX:-}
LIVE_IMAGE=${LIVE_IMAGE:-/proc/kcore}
TIMEOUT=${TIMEOUT:-300}

PASS=0
FAIL=0
SKIP=0
TIMEOUT_CMD=
MODE=
TARGET_VMLINUX=
TARGET_VMCORE=

usage()
{
	cat <<EOF
Usage: $PROG [options]

Run basic crash commands against a vmcore/vmlinux pair and, with
--live, against the running kernel through /proc/kcore.

By default the dump is read from:

    $VMCORE_DIR/vmlinux
    $VMCORE_DIR/vmcore

Options:
  --crash PATH         crash binary (default: in-tree ./crash,
                       else crash from PATH)
  --vmcore-dir DIR     directory holding vmlinux and vmcore
                       (default: $REPO_DIR/vmcore/\$ARCH)
  --vmlinux FILE       explicit vmlinux
  --vmcore FILE        explicit vmcore
  --arch ARCH          arch subdirectory name (default: uname -m)
  --live               also test the live kernel through /proc/kcore
  --live-vmlinux FILE  vmlinux for the live test (default: search
                       /usr/lib/debug, /boot and /lib/modules)
  --live-image FILE    live memory image (default: /proc/kcore)
  -h, --help           this help
EOF
}

while [ $# -gt 0 ]; do
	case $1 in
	--crash)
		[ $# -ge 2 ] || { echo "$PROG: --crash needs an argument" >&2; exit 2; }
		CRASH=$2
		shift 2
		;;
	--vmcore-dir)
		[ $# -ge 2 ] || { echo "$PROG: --vmcore-dir needs an argument" >&2; exit 2; }
		VMCORE_DIR=$2
		shift 2
		;;
	--vmlinux)
		[ $# -ge 2 ] || { echo "$PROG: --vmlinux needs an argument" >&2; exit 2; }
		VMLINUX=$2
		shift 2
		;;
	--vmcore)
		[ $# -ge 2 ] || { echo "$PROG: --vmcore needs an argument" >&2; exit 2; }
		VMCORE=$2
		shift 2
		;;
	--arch)
		[ $# -ge 2 ] || { echo "$PROG: --arch needs an argument" >&2; exit 2; }
		ARCH=$2
		shift 2
		;;
	--live)
		LIVE=1
		shift
		;;
	--live-vmlinux)
		[ $# -ge 2 ] || { echo "$PROG: --live-vmlinux needs an argument" >&2; exit 2; }
		LIVE_VMLINUX=$2
		shift 2
		;;
	--live-image)
		[ $# -ge 2 ] || { echo "$PROG: --live-image needs an argument" >&2; exit 2; }
		LIVE_IMAGE=$2
		shift 2
		;;
	-h | --help)
		usage
		exit 0
		;;
	*)
		echo "$PROG: unknown option: $1" >&2
		usage >&2
		exit 2
		;;
	esac
done

pass()
{
	PASS=$((PASS + 1))
	printf 'PASS: %s\n' "$1"
}

fail()
{
	FAIL=$((FAIL + 1))
	printf 'FAIL: %s\n' "$1"
}

skip()
{
	SKIP=$((SKIP + 1))
	printf 'SKIP: %s\n' "$1"
}

run_crash()
{
	if [ -n "$TIMEOUT_CMD" ]; then
		"$TIMEOUT_CMD" "$TIMEOUT" "$CRASH" -s "$@"
	else
		"$CRASH" -s "$@"
	fi
}

if command -v timeout >/dev/null 2>&1; then
	TIMEOUT_CMD=timeout
fi

. "$SCRIPT_DIR/case.sh"
for case_file in "$SCRIPT_DIR"/cases/*.sh; do
	[ -r "$case_file" ] || continue
	. "$case_file"
done

if [ -z "$REGISTERED_CASES" ]; then
	echo "$PROG: no test cases found in $SCRIPT_DIR/cases" >&2
	exit 2
fi

VMLINUX=${VMLINUX:-$VMCORE_DIR/vmlinux}
VMCORE=${VMCORE:-$VMCORE_DIR/vmcore}

if [ -z "$CRASH" ]; then
	echo "$PROG: crash is not built and not installed" >&2
	echo "Build or install crash, or use --crash PATH." >&2
	exit 2
fi

if [ ! -x "$CRASH" ]; then
	echo "$PROG: $CRASH is not executable" >&2
	echo "Build crash first, or use --crash PATH." >&2
	exit 2
fi

TMPBASE=${TMPDIR:-/tmp}
WORKDIR=$(mktemp -d "$TMPBASE/crash-tests.XXXXXX") || exit 2
trap 'rm -rf "$WORKDIR"' 0 1 2 15

run_target()
{
	MODE=$1
	TARGET_VMLINUX=$2
	TARGET_VMCORE=$3
	run_registered_cases
}

# A valid vmlinux/vmcore pair must initialize without error.  A failure
# here is a real crash failure on valid input, so the command cases are
# not run against a target that cannot even start.
check_init()
{
	log=$WORKDIR/init
	: >"$log"

	if printf 'quit\n' | run_crash "$1" "$2" >"$log" 2>&1; then
		rc=0
	else
		rc=$?
	fi

	if [ "$rc" -eq 0 ]; then
		pass "init: $1 $2"
		return 0
	fi

	fail "init: crash failed to initialize $1 $2 (exit $rc)"
	sed 's/^/    /' "$log" | tail -n 5
	return 1
}

printf 'crash tests: %s\n' "$CRASH"

if [ -r "$VMLINUX" ] && [ -r "$VMCORE" ]; then
	printf 'dump: %s %s\n' "$VMLINUX" "$VMCORE"
	if check_init "$VMLINUX" "$VMCORE"; then
		run_target dump "$VMLINUX" "$VMCORE"
	else
		skip "dump: command cases skipped after init failure"
	fi
else
	skip "dump: $VMLINUX or $VMCORE not readable"
fi

if [ "$LIVE" -eq 1 ]; then
	if [ ! -r "$LIVE_IMAGE" ]; then
		skip "live: $LIVE_IMAGE not readable (root required?)"
	else
		if [ -z "$LIVE_VMLINUX" ]; then
			for candidate in \
				"/usr/lib/debug/lib/modules/$(uname -r)/vmlinux" \
				"/usr/lib/debug/boot/vmlinux-$(uname -r)" \
				"/boot/vmlinux-$(uname -r)" \
				"/lib/modules/$(uname -r)/vmlinux"; do
				if [ -r "$candidate" ]; then
					LIVE_VMLINUX=$candidate
					break
				fi
			done
		fi

		if [ -z "$LIVE_VMLINUX" ]; then
			skip "live: no vmlinux for $(uname -r) (use --live-vmlinux)"
		else
			printf 'live: %s %s\n' "$LIVE_VMLINUX" "$LIVE_IMAGE"
			if check_init "$LIVE_VMLINUX" "$LIVE_IMAGE"; then
				run_target live "$LIVE_VMLINUX" "$LIVE_IMAGE"
			else
				skip "live: command cases skipped after init failure"
			fi
		fi
	fi
fi

printf '\n%d passed, %d failed, %d skipped\n' "$PASS" "$FAIL" "$SKIP"
[ "$FAIL" -eq 0 ] || exit 1
