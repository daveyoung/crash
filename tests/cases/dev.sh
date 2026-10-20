#!/bin/sh
#
# dev: both the character and the block device tables must be present.

register_case "dev" '^ *[0-9]+ +[^ ]+ +' 5 check_dev

check_dev()
{
	expect '^CHRDEV ' &&
	expect '^BLKDEV '
}
