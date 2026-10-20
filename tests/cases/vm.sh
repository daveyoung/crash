#!/bin/sh
#
# vm: the task header and the VMA table (vma, start, end, flags) must
# be present.

register_case "vm" '^[0-9a-f]+ +[0-9a-f]+ +[0-9a-f]+ +[0-9a-f]+' 2 check_vm

check_vm()
{
	expect '^PID: [0-9]+' &&
	expect '^ *MM +PGD +RSS +TOTAL_VM'
}
