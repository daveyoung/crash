#!/bin/sh
#
# kmem -n: the NODE and ZONE tables must be present with numeric rows.
#
# Zone names are not all upper case: DMA, DMA32, Normal, Movable, ...

register_case "kmem -n" '^ *[0-9]+ +[0-9a-f]+ +[0-9a-f]+' 2 check_kmem

check_kmem()
{
	expect '^NODE ' &&
	expect '^ *ZONE +NAME' &&
	expect '^ *[0-9]+ +[^ ]+ +[0-9]+'
}
