#!/bin/sh
#
# sys: system information.  The kernel release and the resource
# counters must be numeric, not merely present.

register_case "sys" '^ *[A-Z][A-Z ]+: ' 8 check_sys

check_sys()
{
	expect '^ *RELEASE: [0-9]+\.[0-9]+' &&
	expect '^ *CPUS: [0-9]+' &&
	expect '^ *TASKS: [0-9]+' &&
	expect '^ *MEMORY: [0-9]+' &&
	expect '^ *MACHINE: [^ ]'
}
