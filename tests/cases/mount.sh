#!/bin/sh
#
# mount: each row ends with a mount point starting with "/".

register_case "mount" '^ *[0-9a-f]+ +[0-9a-f]+ +[^ ]+ +[^ ]+ +/' 2 check_mount

check_mount()
{
	expect '^ *MOUNT +SUPERBLK'
}
