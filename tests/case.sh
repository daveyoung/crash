#!/bin/sh
#
# Case framework for the crash smoke tests.
#
# run.sh sources this file, then every tests/cases/*.sh.  Each command
# case registers itself with register_case(); run_registered_cases()
# runs them and applies the checks that are common to all commands
# (exit status, error/warning output, data row count) before calling the
# command-specific validator.
#
# A case file provides:
#
#   register_case COMMAND ROWS_REGEX MIN_ROWS CHECK_FUNCTION
#
# COMMAND        crash command line, e.g. "kmem -n"
# ROWS_REGEX     ERE matching one well-formed data row
# MIN_ROWS       minimum number of matching rows
# CHECK_FUNCTION function that validates command-specific fields with
#                expect(); it may use the $log and $cmd globals.
#
# The registration fields are tab-separated; ROWS_REGEX may contain any
# ERE character other than a tab.

REGISTERED_CASES=
CASES=0

register_case()
{
	REGISTERED_CASES="${REGISTERED_CASES}$1	$2	$3	$4
"
}

run_registered_cases()
{
	while IFS='	' read -r cmd rows min check; do
		[ -n "$cmd" ] || continue
		run_case "$cmd" "$rows" "$min" "$check"
	done <<EOF
$REGISTERED_CASES
EOF
}

# An ERE that must appear in the command output.
expect()
{
	if ! grep -Eq "$1" "$log"; then
		fail "$MODE: $cmd (no match for '$1')"
		return 1
	fi
}

# Run one command and validate its output.  A case fails on a non-zero
# exit status, crash error or warning output, too few well-formed data
# rows, or a failed command-specific check.
run_case()
{
	cmd=$1
	rows=$2
	min=$3
	check=$4
	cmdword=${cmd%% *}
	CASES=$((CASES + 1))

	log=$WORKDIR/output
	: >"$log"

	if printf '%s\nquit\n' "$cmd" |
	    run_crash "$TARGET_VMLINUX" "$TARGET_VMCORE" >"$log" 2>&1; then
		rc=0
	else
		rc=$?
	fi

	if [ "$rc" -ne 0 ]; then
		fail "$MODE: $cmd (unexpected exit status $rc)"
		sed 's/^/    /' "$log" | head -n 5
		return
	fi

	if grep -Eq "^(crash|$cmdword)(: | )" "$log"; then
		fail "$MODE: $cmd (unexpected error output)"
		grep -E "^(crash|$cmdword)(: | )" "$log" |
		    head -n 3 | sed 's/^/    /'
		return
	fi

	if grep -q 'WARNING: ' "$log"; then
		fail "$MODE: $cmd (unexpected warning output)"
		grep 'WARNING: ' "$log" | head -n 3 | sed 's/^/    /'
		return
	fi

	if [ ! -s "$log" ]; then
		fail "$MODE: $cmd (unexpected empty output)"
		return
	fi

	n=$(grep -Ec "$rows" "$log") || n=0
	if [ "$n" -lt "$min" ]; then
		fail "$MODE: $cmd ($n data rows, expected at least $min)"
		return
	fi

	"$check" || return

	pass "$MODE: $cmd"
}
