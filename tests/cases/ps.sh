#!/bin/sh
#
# ps: a real row has a numeric PID, PPID, CPU/NUMA, task address and a
# state.  PID 1 must be listed.

register_case "ps" \
	'^[> ]* *[0-9]+ +[0-9]+ +[0-9]+(/[0-9]+)? +[0-9a-f]+ +[A-Z][A-Z] +' \
	5 check_ps

check_ps()
{
	expect '^ *PID +PPID' &&
	expect '^[> ]* *1 +0 +'
}
