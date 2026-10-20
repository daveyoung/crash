#!/bin/sh
#
# bt: the header must name a task and at least two stack frames must be
# present.

register_case "bt" '^ *#[0-9]+ ' 2 check_bt

check_bt()
{
	expect '^PID: [0-9]+' &&
	expect 'COMMAND: '
}
