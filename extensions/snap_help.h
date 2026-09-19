/* Generated from man/*.8 by man/gen-help-data.py.
 * Do not edit; run "make man" after changing a man page.
 */
static char *help_snap[] = {
"@MANPAGE@",
"[-n] dumpfile",
"NAME",
"       crash-snap - take a memory snapshot",
"",
"SYNOPSIS",
"       snap [-n] dumpfile",
"",
"DESCRIPTION",
"       This command takes a snapshot of physical memory and creates an ELF",
"       vmcore.  The default vmcore is a kdump-style dumpfile.  Supported on",
"       x86, x86_64, ia64 and ppc64 architectures only.",
"",
"       -n     create a netdump-style vmcore (n/a on x86_64).",
"",
"SEE ALSO",
"       crash(8)",
NULL
};

