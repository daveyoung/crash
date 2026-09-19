/* help.c - core analysis suite
 *
 * Copyright (C) 1999, 2000, 2001, 2002 Mission Critical Linux, Inc.
 * Copyright (C) 2002-2020 David Anderson
 * Copyright (C) 2002-2020 Red Hat, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "defs.h"

static void reshuffle_cmdlist(void);
static int sort_command_name(const void *, const void *);
static void display_commands(void);
static void display_copying_info(void);
static void display_warranty_info(void);
static void display_output_info(void);
static void display_input_info(void);
static void display_README(void);
static char *gnu_public_license[];
static char *gnu_public_license_v3[];
static char *version_info[];
static char *output_info[];
static char *input_info[];
static char *README[];
static void dump_registers(void);

#define GPLv2 2
#define GPLv3 3

#if defined(GDB_5_3) || defined(GDB_6_0) || defined(GDB_6_1)
static int GPL_version = GPLv2;
#else
static int GPL_version = GPLv3;
#endif

static 
char *program_usage_info[] = {
    "",
    "USAGE:",
    "",
    "  crash [OPTION]... NAMELIST MEMORY-IMAGE[@ADDRESS]	(dumpfile form)",
    "  crash [OPTION]... [NAMELIST]             		(live system form)",
    "",
    "OPTIONS:",
    "",
    "  NAMELIST",
    "    This is a pathname to an uncompressed kernel image (a vmlinux",
    "    file), or a Xen hypervisor image (a xen-syms file) which has",
    "    been compiled with the \"-g\" option.  If using the dumpfile form,",
    "    a vmlinux file may be compressed in either gzip or bzip2 formats.",
    "",
    "  MEMORY-IMAGE",
    "    A kernel core dump file created by the netdump, diskdump, LKCD",
    "    kdump, xendump or kvmdump facilities.",
    "",
    "    If a MEMORY-IMAGE argument is not entered, the session will be",
    "    invoked on the live system, which typically requires root privileges",
    "    because of the device file used to access system RAM.  By default, ",
    "    /dev/crash will be used if it exists.  If it does not exist, then ",
    "    /dev/mem will be used; but if the kernel has been configured with ",
    "    CONFIG_STRICT_DEVMEM, then /proc/kcore will be used.  It is permissible",
    "    to explicitly enter /dev/crash, /dev/mem or /proc/kcore.",
    "",
    "    An @ADDRESS value must be appended to the MEMORY-IMAGE if the dumpfile",
    "    is a raw RAM dumpfile that has no header information describing the file",
    "    contents.  Multiple MEMORY-IMAGE@ADDRESS ordered pairs may be entered,",
    "    with each dumpfile containing a contiguous block of RAM, where the ADDRESS",
    "    value is the physical start address of the block expressed in hexadecimal.",
    "    The physical address value(s) will be used to create a temporary ELF header",
    "    in /var/tmp, which will only exist during the crash session.  If a raw RAM",
    "    dumpfile represents a live memory source, such as that specified by the QEMU",
    "    mem-path argument of a memory-backend-file object, then \"live:\" must be",
    "    prepended to the MEMORY-IMAGE name.",
    "",
    "  mapfile",
    "    If the NAMELIST file is not the same kernel that is running",
    "    (live system form), or the kernel that was running when the system",
    "    crashed (dumpfile form), then the System.map file of the original ",
    "    kernel should be entered on the command line.",
    "",
    "  -h [option]",
    "  --help [option]",
    "    Without an option argument, display a crash usage help message.",
    "    If the option argument is a crash command name, the help page",
    "    for that command is displayed.  If it is the string \"input\", a",
    "    page describing the various crash command line input options is",
    "    displayed.  If it is the string \"output\", a page describing command",
    "    line output options is displayed.  If it is the string \"all\", then",
    "    all of the possible help messages are displayed.  After the help",
    "    message is displayed, crash exits.",
    "",
    "  -s     ",
    "    Silently proceed directly to the \"crash>\" prompt without displaying",
    "    any version, GPL, or crash initialization data during startup, and by",
    "    default, runtime command output is not passed to any scrolling command.",
    "",
    "  -i file",
    "    Execute the command(s) contained in \"file\" prior to displaying ",
    "    the \"crash>\" prompt for interactive user input.",
    "",
    "  -d num ",
    "    Set the internal debug level.  The higher the number, the more",
    "    debugging data will be printed when crash initializes and runs.",
    "",
    "  -S     ",
    "    Use /boot/System.map as the mapfile.",
    "",
    "  -e  vi | emacs",
    "    Set the readline(3) command  line editing mode to \"vi\" or \"emacs\".  ",
    "    The default editing mode is \"vi\".",
    "",
    "  -f     ",
    "    Force the usage of a compressed vmlinux file if its original",
    "    name does not start with \"vmlinux\".",
    "",
    "  -k     ",
    "    Indicate that the NAMELIST file is an LKCD \"Kerntypes\" debuginfo file.",
    "",
    "  -g [namelist]",
    "    Determine if a vmlinux or xen-syms namelist file contains debugging data.",
    "",
    "  -t     ",
    "    Display the system-crash timestamp and exit.",
    "",
    "  -L     ",
    "    Attempt to lock all of its virtual address space into memory by",
    "    calling mlockall(MCL_CURRENT|MCL_FUTURE) during initialization.",
    "    If the system call fails, an error message will be displayed,",
    "    but the session continues.",
    "",
    "  -c tty-device",
    "    Open the tty-device as the console used for debug messages.",
    "",
    "  -p page-size",
    "    If a processor's page size cannot be determined by the dumpfile, ",
    "    and the processor default cannot be used, use page-size.",
    "",
    "  -o filename",
    "    Only used with the MEMORY-IMAGE@ADDRESS format for raw RAM dumpfiles,",
    "    specifies a filename of a new ELF vmcore that will be created and used", 
    "    as the dumpfile.  It will be saved to allow future use as a standalone", 
    "    vmcore, replacing the original raw RAM dumpfile.",
    "",
    "  -m option=value",
    "  --machdep option=value",
    "    Pass an option and value pair to machine-dependent code.  These",
    "    architecture-specific option/pairs should only be required in",
    "    very rare circumstances:",
    "",
    "    X86_64:",
    "      phys_base=<physical-address>",
    "      irq_eframe_link=<value>",
    "      irq_stack_gap=<value>",
    "      max_physmem_bits=<value>",
    "      kernel_image_size=<value>",
    "      vm=orig       (pre-2.6.11 virtual memory address ranges)",
    "      vm=2.6.11     (2.6.11 and later virtual memory address ranges)",
    "      vm=xen        (Xen kernel virtual memory address ranges)",
    "      vm=xen-rhel4  (RHEL4 Xen kernel virtual address ranges)",
    "      vm=5level     (5-level page tables)",
    "      page_offset=<PAGE_OFFSET-value>",
    "    PPC64:",
    "      vm=orig",
    "      vm=2.6.14     (4-level page tables)",
    "    IA64:",
    "      phys_start=<physical-address>",
    "      init_stack_size=<size>",
    "      vm=4l         (4-level page tables)",
    "    ARM:",
    "      phys_base=<physical-address>",
    "    ARM64:",
    "      phys_offset=<physical-address>",
    "      kimage_voffset=<kimage_voffset-value>",
    "      max_physmem_bits=<value>",
    "      vabits_actual=<value>",
    "    X86:",
    "      page_offset=<CONFIG_PAGE_OFFSET-value>",
    "",
    "  -x     ",
    "    Automatically load extension modules from a particular directory.",
    "    The directory is determined by the following order of precedence:",
    "",
    "    (1) the directory specified in the CRASH_EXTENSIONS shell ",
    "        environment variable",
    "    (2) /usr/lib64/crash/extensions  (64-bit  architectures)",
    "    (3) /usr/lib/crash/extensions  (32-bit architectures)",
    "    (4) the ./extensions subdirectory of the current directory",
    "",
    "  --active",
    "    Track only the active task on each cpu.",
    "",
    "  --buildinfo",
    "    Display the crash binary's build date, the user ID of the builder,",
    "    the hostname of the machine where the build was done, the target", 
    "    architecture, the version number, and the compiler version.",
    "",
    "  --memory_module modname",
    "    Use the modname as an alternative kernel module to the crash.ko",
    "    module that creates the /dev/crash device.",
    "",
    "  --memory_device device",
    "    Use device as an alternative device to the /dev/crash, /dev/mem",
    "    or /proc/kcore devices.",
    "",
    "  --log dumpfile",
    "    Dump the contents of the kernel log buffer.  A kernel namelist",
    "    argument is not necessary, but the dumpfile must contain the",
    "    VMCOREINFO data taken from the original /proc/vmcore ELF header.",
    "    Note: this option is deprecated and will no longer work for",
    "    kernel(>=v5.10).",
    "",
    "  --no_kallsyms",
    "    Do not use kallsyms-generated symbol information contained within",
    "    kernel module object files.",
    "",
    "  --no_modules",
    "    Do not access or display any kernel module related information.",
    "",
    "  --no_ikconfig",
    "    Do not attempt to read configuration data that was built into",
    "    kernels configured with CONFIG_IKCONFIG.",
    "",
    "  --no_data_debug",
    "    Do not verify the validity of all structure member offsets and",
    "    structure sizes that it uses.",
    "",
    "  --no_kmem_cache",
    "    Do not initialize the kernel's slab cache infrastructure, and",
    "    commands that use kmem_cache-related data will not work.",
    "",
    "  --no_elf_notes",
    "    Do not use the registers from the ELF NT_PRSTATUS notes saved",
    "    in a compressed kdump header for backtraces.",
    "",
    "  --kmem_cache_delay",
    "    Delay the initialization of the kernel's slab cache infrastructure",
    "    until it is required by a run-time command.",
    "",
    "  --readnow",
    "    Pass this flag to the embedded gdb module, which will override",
    "    the two-stage strategy that it uses for reading symbol tables",
    "    from the NAMELIST.  If module symbol tables are loaded during",
    "    runtime with the \"mod\" command, the same override will occur.",
    "",
    "  --smp  ",
    "    Specify that the system being analyzed is an SMP kernel.",
    "",
    "  -v",
    "  --version",
    "    Display the version of the crash utility, the version of the",
    "    embedded gdb module, GPL information, and copyright notices.",
    "",
    "  --cpus number",
    "    Specify the number of cpus in the SMP system being analyzed.",
    "",
    "  --osrelease dumpfile",
    "    Display the OSRELEASE vmcoreinfo string from a kdump dumpfile",
    "    header.",
    "",
    "  --build-id dumpfile",
    "    Display the BUILD-ID vmcoreinfo string from a kdump dumpfile",
    "    header.",
    "    Note: this option only works for kernel(>=v5.9); otherwise it",
    "    prints \"unknown\" and exits with non-zero status",
    "",
    "  --hyper",
    "    Force the session to be that of a Xen hypervisor.",
    "",
    "  --p2m_mfn pfn",
    "    When a Xen Hypervisor or its dom0 kernel crashes, the dumpfile",
    "    is typically analyzed with either the Xen hypervisor or the dom0",
    "    kernel.  It is also possible to analyze any of the guest domU",
    "    kernels if the pfn_to_mfn_list_list pfn value of the guest kernel",
    "    is passed on the command line along with its NAMELIST and the ",
    "    dumpfile.",
    "",
    "  --xen_phys_start physical-address",
    "    Supply the base physical address of the Xen hypervisor's text",
    "    and static data for older xendump dumpfiles that did not pass",
    "    that information in the dumpfile header.",
    "",
    "  --zero_excluded",
    "    If the makedumpfile(8) facility has filtered a compressed kdump",
    "    dumpfile to exclude various types of non-essential pages, or has",
    "    marked a compressed or ELF kdump dumpfile as incomplete due to",
    "    an ENOSPC or other error during its creation, any attempt to",
    "    read missing pages will fail.  With this flag, reads from any",
    "    of those pages will return zero-filled memory.",
    "",
    "  --no_panic",
    "    Do not attempt to find the task that was running when the kernel",
    "    crashed.  Set the initial context to that of the \"swapper\"  task",
    "    on cpu 0.",
    "",
    "  --more ",
    "    Use /bin/more as the command output scroller, overriding the",
    "    default of /usr/bin/less and any settings in either ./.crashrc",
    "    or $HOME/.crashrc.",
    "",
    "  --less ",
    "    Use /usr/bin/less as the command output scroller, overriding any",
    "    settings in either ./.crashrc or $HOME/.crashrc.",
    "",
    "  --CRASHPAGER",
    "    Use the output paging command defined in the CRASHPAGER shell",
    "    environment variable, overriding any settings in either ./.crashrc ",
    "    or $HOME/.crashrc.",
    "",
    "  --no_scroll",
    "    Do not pass run-time command output to any scrolling command.",
    "",
    "  --no_strip",
    "    Do not strip cloned kernel text symbol names.",
    "",
    "  --no_crashrc",
    "    Do not execute the commands in either $HOME/.crashrc or ./.crashrc.",
    "",
    "  --mod directory",
    "    When loading the debuginfo data of kernel modules with the \"mod -S\"",
    "    command, search for their object files in directory instead of in ",
    "    the standard location.",
    "",
    "  --src directory",
    "    Search for the kernel source code in directory instead of in the",
    "    standard location that is compiled into the debuginfo data.",
    "",
    "  --reloc size",
    "    When analyzing live x86 kernels configured with a CONFIG_PHYSICAL_START ",
    "    value that is larger than its CONFIG_PHYSICAL_ALIGN value, then it will",
    "    be necessary to enter a relocation size equal to the difference between",
    "    the two values.",
    "",
    "  --hash count",
    "    Set the number of internal hash queue heads used for list gathering",
    "    and verification.  The default count is 32768.",
    "",
    "  --kaslr offset | auto",
    "    If x86, x86_64, s390x or loongarch64 kernel was configured with",
    "    CONFIG_RANDOMIZE_BASE, the offset value is equal to the difference",
    "    between the symbol values compiled into the vmlinux file and their",
    "    relocated KASLR value.  If set to auto, the KASLR offset value will",
    "    be automatically calculated.",
    "",
    "  --max-malloc-bufs <size>",
    "    Set the value of MAX_MALLOC_BUFS to size.",
    "    The minimum allowed value is 3072.",
    "",
    "  --minimal",
    "    Bring up a session that is restricted to the log, dis, rd, sym,",
    "    eval, set and exit commands.  This option may provide a way to",
    "    extract some minimal/quick information from a corrupted or truncated",
    "    dumpfile, or in situations where one of the several kernel subsystem ",
    "    initialization routines would abort the crash session.",
    "",
    "  --kvmhost [32|64]",
    "    When examining an x86 KVM guest dumpfile, this option specifies",
    "    that the KVM host that created the dumpfile was an x86 (32-bit)",
    "    or an x86_64 (64-bit) machine, overriding the automatically",
    "    determined value.",
    "",
    "  --kvmio <size>",
    "    override the automatically-calculated KVM guest I/O hole size.",
    "",
    "  --offline [show|hide]",
    "    Show or hide command output that is associated with offline cpus,",
    "    overriding any settings in either ./.crashrc or $HOME/.crashrc.",
    "",
    "FILES:",
    "",
    "  .crashrc",
    "    Initialization commands.  The file can be located in the user's",
    "    HOME directory and/or the current directory.  Commands found in",
    "    the .crashrc file in the HOME directory are executed before",
    "    those in the current directory's .crashrc file.",
    "",
    "ENVIRONMENT VARIABLES:",
    "",
    "  EDITOR ",
    "    Command input is read using readline(3).  If EDITOR is set to",
    "    emacs or vi then suitable keybindings are used.  If EDITOR is",
    "    not set, then vi is used.  This can be overridden by \"set vi\" or",
    "    \"set emacs\" commands located in a .crashrc file, or by entering",
    "    \"-e emacs\" on the crash command line.",
    "",
    "  CRASHPAGER",
    "    If CRASHPAGER is set, its value is used as the name of the program",
    "    to which command output will be sent.  If not, then command output",
    "    output is sent to \"/usr/bin/less -E -X\" by default.",
    "",
    "  CRASH_MODULE_PATH",
    "    Specifies an alternative directory tree to search for kernel",
    "    module object files.",
    "",
    "  CRASH_EXTENSIONS",
    "    Specifies a directory containing extension modules that will be",
    "    loaded automatically if the -x command line option is used.",
    "",
    NULL
};

void
program_usage(int form)
{
	if (form == SHORT_FORM) {
		fprintf(fp, "\nUsage:\n\n");
		fprintf(fp, "%s\n%s\n", program_usage_info[3], 
			program_usage_info[4]);
		fprintf(fp, "\nEnter \"%s -h\" for details.\n",
			pc->program_name);
		clean_exit(1);
	} else {
		FILE *scroll;
		char *scroll_command;
		char **p;

		if ((scroll_command = setup_scroll_command()) &&
		    (scroll = popen(scroll_command, "w")))
			fp = scroll;
		else
			scroll = NULL;

		for (p = program_usage_info; *p; p++) {
			fprintf(fp, *p, pc->program_name);
			fprintf(fp, "\n");
		}
		fflush(fp);

		if (scroll)
			pclose(scroll);

		clean_exit(0);
	}
}


/*
 *  Get an updated count of commands for subsequent help menu display,
 *  reshuffling the deck if this is the first time or if something's changed.
 */
void
help_init(void)
{
        struct command_table_entry *cp;
	struct extension_table *ext;

	for (pc->ncmds = 0, cp = pc->cmd_table; cp->name; cp++) {
		if (!(cp->flags & HIDDEN_COMMAND))
                	pc->ncmds++;
	}

        for (ext = extension_table; ext; ext = ext->next) {
		for (cp = ext->command_table; cp->name; cp++) {
			if (!(cp->flags & (CLEANUP|HIDDEN_COMMAND)))
				pc->ncmds++;
		}
	}

        if (!pc->cmdlist) {
		pc->cmdlistsz = pc->ncmds;        
        	if ((pc->cmdlist = (char **)
                	malloc(sizeof(char *) * pc->cmdlistsz)) == NULL)
                        	error(FATAL,
                                    	"cannot malloc command list space\n");
	} else if (pc->ncmds > pc->cmdlistsz) {
		pc->cmdlistsz = pc->ncmds;
		if ((pc->cmdlist = (char **)realloc(pc->cmdlist,
                	sizeof(char *) * pc->cmdlistsz)) == NULL)
				error(FATAL, 
					"cannot realloc command list space\n");
	}

	reshuffle_cmdlist();
}

/*
 *  If the command list is modified during runtime, re-shuffle the list
 *  for proper help menu display.
 */
static void
reshuffle_cmdlist(void)
{
	int i, cnt;
        struct command_table_entry *cp;
	struct extension_table *ext;

	for (i = 0; i < pc->cmdlistsz; i++) 
		pc->cmdlist[i] = NULL;

        for (cnt = 0, cp = pc->cmd_table; cp->name; cp++) {
		if (!(cp->flags & HIDDEN_COMMAND))
                	pc->cmdlist[cnt++] = cp->name;
	}

        for (ext = extension_table; ext; ext = ext->next) {
                for (cp = ext->command_table; cp->name; cp++) {
			if (!(cp->flags & (CLEANUP|HIDDEN_COMMAND)))
				pc->cmdlist[cnt++] = cp->name;
		}
        }

	if (cnt > pc->cmdlistsz)
		error(FATAL, "help table malfunction!\n");

        qsort((void *)pc->cmdlist, (size_t)cnt,
                sizeof(char *), sort_command_name);
}


/*
 *  The help list is in alphabetical order, with exception of the "q" command,
 *  which has historically always been the last command in the list.
 */

static int
sort_command_name(const void *name1, const void *name2)
{
	char **s1, **s2;

	s1 = (char **)name1;
	s2 = (char **)name2;

	if (STREQ(*s1, "q"))  
		return 1;

	return strcmp(*s1, *s2);
}


/*
 *  Get help for a command, to dump an internal table, or the GNU public
 *  license copying/warranty information.
 */
void
cmd_help(void)
{
	int c;
	int oflag;

	oflag = 0;

        while ((c = getopt(argcnt, args, 
	        "efNDdmM:ngcaBbHhkKsvVoptTzLOr")) != EOF) {
                switch(c)
                {
		case 'e':
			dump_extension_table(VERBOSE);
			return;

		case 'f':
			dump_filesys_table(VERBOSE);
			return;

		case 'n':
		case 'D':
			dumpfile_memory(DUMPFILE_MEM_DUMP);
			return;

		case 'd':
			dump_dev_table();
			return;

		case 'M':
			dump_machdep_table(stol(optarg, FAULT_ON_ERROR, NULL));
			return;
		case 'm':
			dump_machdep_table(0);
			return;

		case 'g':
			dump_gdb_data();
			return;

		case 'N':
			dump_net_table();
			return;

		case 'a':
			dump_alias_data();
			return;

		case 'b':
			dump_shared_bufs();
			return;

		case 'B':
			dump_build_data();
			return;

		case 'c':
			dump_numargs_cache();
			return;

		case 'H':
			dump_hash_table(VERBOSE);
			return;

		case 'h':
			dump_hash_table(!VERBOSE);
 			return;

		case 'k':
			dump_kernel_table(!VERBOSE);
			return;

		case 'K':
			dump_kernel_table(VERBOSE);
			return;

		case 's':
			dump_symbol_table();
			return;

		case 'V':
			dump_vm_table(VERBOSE);
			return;

		case 'v':
			dump_vm_table(!VERBOSE);
			return;

		case 'O':
			dump_offset_table(NULL, TRUE);
			return;

		case 'o':
			oflag = TRUE;
			break;

		case 'T':
			dump_task_table(VERBOSE);
			return;

		case 't':
			dump_task_table(!VERBOSE);
			return;

		case 'p':
			dump_program_context();
			return;

		case 'z':
			fprintf(fp, "help options:\n");
			fprintf(fp, " -a - alias data\n");
			fprintf(fp, " -b - shared buffer data\n");
			fprintf(fp, " -B - build data\n");
			fprintf(fp, " -c - numargs cache\n");
			fprintf(fp, " -d - device table\n");
			fprintf(fp, " -D - dumpfile contents/statistics\n");
			fprintf(fp, " -e - extension table data\n");
			fprintf(fp, " -f - filesys table\n");
			fprintf(fp, " -g - gdb data\n");
			fprintf(fp, " -h - hash_table data\n");
			fprintf(fp, " -H - hash_table data (verbose)\n");
			fprintf(fp, " -k - kernel_table\n");
			fprintf(fp, " -K - kernel_table (verbose)\n");
			fprintf(fp, " -L - LKCD page cache environment\n");
			fprintf(fp, " -M <num> machine specific\n");
			fprintf(fp, " -m - machdep_table\n");
			fprintf(fp, " -N - net_table\n");
			fprintf(fp, " -n - dumpfile contents/statistics\n");
			fprintf(fp, " -o - offset_table and size_table\n");
			fprintf(fp, " -p - program_context\n");
			fprintf(fp, " -r - dump registers from dumpfile header\n");
			fprintf(fp, " -s - symbol table data\n");
			fprintf(fp, " -t - task_table\n");
			fprintf(fp, " -T - task_table plus context_array\n");
			fprintf(fp, " -v - vm_table\n");
			fprintf(fp, " -V - vm_table (verbose)\n");
			fprintf(fp, " -z - help options\n");
			return;

		case 'L':
			dumpfile_memory(DUMPFILE_ENVIRONMENT);
			return;

		case 'r':
			dump_registers();
			return;

                default:  
			argerrs++;
                        break;
                }
        }

        if (argerrs)
                cmd_usage(pc->curcmd, COMPLETE_HELP);

	if (!args[optind]) {
		if (oflag) 
			dump_offset_table(NULL, FALSE);
		else 
			display_help_screen("");
		return;
	}

        do {
		if (oflag) 
			dump_offset_table(args[optind], FALSE);
		else	
        		cmd_usage(args[optind], COMPLETE_HELP|MUST_HELP);
		optind++;
        } while (args[optind]);
}

static void
dump_registers(void)
{
	if (pc->flags2 & QEMU_MEM_DUMP_ELF) {
		dump_registers_for_qemu_mem_dump();
		return;
	} else if (DISKDUMP_DUMPFILE()) {
		dump_registers_for_compressed_kdump();
		return;
	} else if (NETDUMP_DUMPFILE() || KDUMP_DUMPFILE()) {
		dump_registers_for_elf_dumpfiles();
		return;
	} else if (VMSS_DUMPFILE()) {
		dump_registers_for_vmss_dump();
		return;
	}

	error(FATAL, "-r option not supported on %s\n",
		ACTIVE() ? "a live system" : "this dumpfile type");
}

/*
 *  Format and display the help menu.
 */

void
display_help_screen(char *indent)
{
        int i, j, rows;
	char **namep;

	help_init();

	fprintf(fp, "\n%s", indent);

        rows = (pc->ncmds + (HELP_COLUMNS-1)) / HELP_COLUMNS;

        for (i = 0; i < rows; i++) {
                namep = &pc->cmdlist[i];
                for (j = 0; j < HELP_COLUMNS; j++) {
                        fprintf(fp,"%-15s", *namep);
                        namep += rows;
                        if ((namep - pc->cmdlist) >= pc->ncmds)
                                break;
                }
                fprintf(fp,"\n%s", indent);
        }

        fprintf(fp, "\n%s%s version: %-6s   gdb version: %s\n", indent,
		pc->program_name, pc->program_version, pc->gdb_version); 
	fprintf(fp,
		"%sFor help on any command above, enter \"help <command>\".\n",
		indent);
	fprintf(fp, "%sFor help on input options, enter \"help input\".\n",
		indent);
	fprintf(fp, "%sFor help on output options, enter \"help output\".\n",
		indent);
#ifdef NO_LONGER_TRUE
	fprintf(fp, "%sFor the most recent version: "
		    "http://www.missioncriticallinux.com/download\n\n", indent);
#else
	fprintf(fp, "\n");
#endif
}

/*
 *  Used for generating HTML pages, dump the commands in the order
 *  they would be seen on the help menu, i.e., from left-to-right, row-by-row.
 *  Line ends are signaled with a "BREAK" string.
 */
static void
display_commands(void)
{
        int i, j, rows;
	char **namep;

	help_init();
        rows = (pc->ncmds + (HELP_COLUMNS-1)) / HELP_COLUMNS;

        for (i = 0; i < rows; i++) {
                namep = &pc->cmdlist[i];
                for (j = 0; j < HELP_COLUMNS; j++) {
                        fprintf(fp,"%s\n", *namep);
                        namep += rows;
                        if ((namep - pc->cmdlist) >= pc->ncmds) {
                                fprintf(fp, "BREAK\n");
                                break;
                        }
                }
        }
}


/*
 *  Help data for a command must be formatted using the following template:
 
"command-name",
"command description line", 
"argument-usage line",
"description...",
"description...",
"description...",
NULL,
 
 *  The first line is concatenated with the second line, and will follow the
 *  help command's "NAME" header.
 *  The first and third lines will also be concatenated, and will follow the
 *  help command's "SYNOPSIS" header.  If the command has no arguments, enter
 *  a string consisting of a space, i.e., " ".
 *  The fourth and subsequent lines will follow the help command's "DESCRIPTION"
 *  header.
 *
 *  The program name can be referenced by using the %%s format.  The final
 *  entry in each command's help data string list must be a NULL.
 */

















 





























































/*
 *  Find out what the help request is looking for, accepting aliases as well,
 *  and print the relevant strings from the appropriate help table.
 */
void
cmd_usage(char *cmd, int helpflag)
{
	char **p, *scroll_command;
	struct command_table_entry *cp;
	char buf[BUFSIZE];
	FILE *scroll;
	int i;

	if (helpflag & PIPE_TO_SCROLL) {
		if ((scroll_command = setup_scroll_command()) &&
                    (scroll = popen(scroll_command, "w")))
			fp = scroll;
                else
                        scroll = NULL;
	} else {
		scroll_command = NULL;
		scroll = NULL;
	}

	if (STREQ(cmd, "copying")) {
		display_copying_info();
		goto done_usage;
	}

	if (STREQ(cmd, "warranty")) {
		display_warranty_info();
		goto done_usage;
	}

	if (STREQ(cmd, "input")) {
		display_input_info();
		goto done_usage;
	}

        if (STREQ(cmd, "output")) {
                display_output_info();
                goto done_usage;
        }

	if (STREQ(cmd, "all")) {
		display_input_info();
                display_output_info();
		help_init();
		for (i = 0; i < pc->ncmds; i++)
			cmd_usage(pc->cmdlist[i], COMPLETE_HELP);
		display_warranty_info();
		display_copying_info();
		goto done_usage;
	}

	if (STREQ(cmd, "commands")) {
		display_commands();
		goto done_usage;
	}

	if (STREQ(cmd, "README")) {
		display_README();
		goto done_usage;
	}

	/* look up command, possibly through an alias */
	for (;;) {
		struct alias_data *ad;

		cp = get_command_table_entry(cmd);
		if (cp != NULL)
			break;	/* found command */

		/* try for an alias */
		ad = is_alias(cmd);
		if (ad == NULL)
			break;	/* neither command nor alias */

		cmd = ad->args[0];
		cp = get_command_table_entry(cmd);
	}

	/*
	 *  Help generated from the man pages carries a sentinel in [0], the
	 *  argument synopsis in [1] and the rendered man page text from [2].
	 */
	if (cp != NULL && (p = cp->help_data) != NULL &&
	    STREQ(p[0], MANPAGE_HELP)) {
		if (helpflag & SYNOPSIS) {
			fprintf(fp, "Usage:\n  %s %s\n", cmd, p[1]);
			fprintf(fp, "Enter \"help %s\" for details.\n", cmd);
			RESTART();
		}
		for (p += 2; *p; p++) {
			fputs(*p, fp);
			fputc('\n', fp);
		}
		goto done_usage;
	}

	if (cp == NULL || (p = cp->help_data) == NULL) {
		if (helpflag & SYNOPSIS) { 
			fprintf(fp,
				"No usage data for the \"%s\" command"
				" is available.\n",
				cmd);
			RESTART();
		}

		if (helpflag & MUST_HELP) {
			if (cp || !(pc->flags & GDB_INIT))
				fprintf(fp,
				    "No help data for the \"%s\" command"
				    " is available.\n",
					cmd);
			else if (!gdb_pass_through(concat_args(buf, 0, FALSE), 
				NULL, GNU_RETURN_ON_ERROR))
				fprintf(fp, 
					"No help data for \"%s\" is available.\n",
					cmd);
		}
		goto done_usage;
        }

	p++;

        if (helpflag & SYNOPSIS) {
                p++;
                fprintf(fp, "Usage:\n  %s ", cmd);
		fprintf(fp, *p, pc->program_name, pc->program_name);
		fprintf(fp, "\nEnter \"help %s\" for details.\n", cmd);
                RESTART();
        }

        fprintf(fp, "\nNAME\n  %s - ", cmd);
        fprintf(fp, *p, pc->program_name);

        fprintf(fp, "\n\nSYNOPSIS\n");
        p++;
        fprintf(fp, "  %s ", cmd);
        fprintf(fp, *p, pc->program_name);

        fprintf(fp,"\n\nDESCRIPTION\n");
        p++;
        do {
		if (strstr(*p, "%") && !strstr(*p, "%s")) 
			print_verbatim(fp, *p);
		else
                	fprintf(fp, *p, pc->program_name, pc->program_name);
		fprintf(fp, "\n");
                p++;
        } while (*p);

        fprintf(fp, "\n");

done_usage:

	if (scroll) {
		fflush(scroll);
		pclose(scroll);
	}
	if (scroll_command)
		FREEBUF(scroll_command);
}


static
char *input_info[] = {

"Interactive %s commands are gathered using the GNU readline library,",
"which implements a command line history mechanism, and a command line editing",
"mode.\n", 

"The command line history consists of a numbered list of previously run ",
"commands, which can be viewed by entering \"h\" at any time.  A previously", 
"run command can be re-executed in a number of manners:",
" ",       
"  1. To re-execute the last command, enter \"r\" alone, or \"!!\".",
"  2. Enter \"r\" followed by the number identifying the desired command.",
"  3. Enter \"r\" followed by a uniquely-identifying set of characters from",
"     the beginning of the desired command string.",
"  4. Enter \"!\" followed by the number identifying the desired command,", 
"     providing that the number is not a command name in the user's PATH.",
"  5. Recycle back through the command history list by hitting the up-arrow",
"     key until the desired command is re-displayed, and then hit <ENTER>.",
"     If you go too far back, hit the down-arrow key.",
" ",
"The command line editing mode can be set to emulate either vi or emacs.",
"The mode can be set in the following manners, listed in increasing order of",
"precedence:",
" ",
"  1. The setting of the EDITOR environment variable.",
"  2. An entry in either in a .%src file, which can be located either",
"     in the current directory or in your home directory.  The entry must",
"     be of the form \"set vi\" or \"set emacs\".",
"  3. By use of the %s \"-e\" command line option, as in \"-e vi\" or \"-e emacs\".",
" ",
"To edit a previously entered command:",
" ",
"  1. Recycle back through the command history list until the desired command",
"     is re-displayed.",
"  2. Edit the line using vi or emacs editing commands, as appropriate. ",
"  3. Hit <ENTER>.",
" ",
"It should be noted that command line re-cycling may be accomplished by using",
"the CTRL-p and CTRL-n keys instead of the up- and down-arrow keys; in vi mode",
"you can enter <ESC>, then \"k\" to cycle back, or \"j\" to cycle forward.",
" ",
"A set of %s commands may be entered into a regular file that can be used as",
"input, using standard command line syntax:\n",
"  %s> < inputfile\n",
"An input file may be also be run from the %s command line using the -i ",
"option:\n",
"  $ %s -i inputfile",
"",
"Alternatively, an input file containing command arguments may be created.",
"The arguments in the input file will be passed to the command specified,",
"which will be executed repetitively for each line of arguments in the file:",
"",
"  %s> ps -p < inputfile",
"",
"Lastly, if a command is entered that is not recognized, it is checked against",
"the kernel's list of variables, structure, union or typedef names, and if ",
"found, the command is passed to p, struct, union or whatis.  That being the ",
"case, as long as a kernel variable/structure/union name is different than any",
"of the current commands, the appropriate command above will be executed.  If",
"not, the command will be passed on to the built-in gdb command for execution.",
"If an input line starts with \"#\" or \"//\", then the line will be saved",
"as a comment that is visible when re-cycling through the history list.",
"",
"To execute an external shell command, precede the command with an \"!\".",
"To escape to a shell, enter \"!\" alone.",
NULL
};


/*
 *  Display information concerning command input options: history,
 *  command recall, command-line editing, and input files.
 */
static void
display_input_info(void)
{
	int i;

        for (i = 0; input_info[i]; i++) {
                fprintf(fp, input_info[i],  
                        pc->program_name, pc->program_name);
                fprintf(fp, "\n");
	}
}

static
char *output_info[] = {

"\nBy default, %s command output is piped to \"/usr/bin/less -E -X\" along",
"with a prompt line.  This behavior can be turned off in two ways:\n",
"  1. During runtime, enter \"set scroll off\" or the alias \"sf\".",
"  2. Enter \"set scroll off\" in a .%src file, which can be located either",
"     in the current directory or in your home directory.\n",
"To restore the scrolling behavior during runtime, enter \"set scroll on\"",
"or the alias: \"sn\"\n",
"Command output may be piped to an external command using standard command",
"line pipe syntax.  For example:\n", 
"  %s> log | grep eth0\n",
"Command output may be redirected to a file using standard command line syntax.",
"For example:\n",
"  %s> foreach bt > bt.all\n",
"Use double brackets to append the output to a pre-existing file:\n",
"  %s> ps >> crash.data\n",
"The default output radix for gdb output and certain %s commands is",
"hexadecimal.  This can be changed to decimal by entering \"set radix 10\"",
"or the alias \"dec\".  It can be reverted back to hexadecimal by entering",
"\"set radix 16\" or the alias \"hex\".\n",
"To execute an external shell command, precede the command with an \"!\".",
"To escape to a shell, enter \"!\" alone.",
" ",
NULL
};

/*
 *  Display information concerning command output options.
 */
static void
display_output_info(void)
{
        int i;

        for (i = 0; output_info[i]; i++) {
                fprintf(fp, output_info[i], 
			pc->program_name, pc->program_name);
                fprintf(fp, "\n");
	}
}


/*
 *  Display the program name, version, and the GPL-required stuff for
 *  interactive programs.
 */
void
display_version(void)
{
	int i;

	if (pc->flags & SILENT)
		return;

        fprintf(fp, "\n%s %s\n", pc->program_name, pc->program_version);

        for (i = 0; version_info[i]; i++) 
                fprintf(fp, "%s\n", version_info[i]);
}

static 
char *version_info[] = {

"Copyright (C) 2002-2026  Red Hat, Inc.",
"Copyright (C) 2004, 2005, 2006, 2010  IBM Corporation", 
"Copyright (C) 1999-2006  Hewlett-Packard Co",
"Copyright (C) 2005, 2006, 2011, 2012  Fujitsu Limited",
"Copyright (C) 2006, 2007  VA Linux Systems Japan K.K.",
"Copyright (C) 2005, 2011, 2020-2024  NEC Corporation",
"Copyright (C) 1999, 2002, 2007  Silicon Graphics, Inc.",
"Copyright (C) 1999, 2000, 2001, 2002  Mission Critical Linux, Inc.",
"Copyright (C) 2015, 2021  VMware, Inc.",
"This program is free software, covered by the GNU General Public License,",
"and you are welcome to change it and/or distribute copies of it under",
"certain conditions.  Enter \"help copying\" to see the conditions.",
"This program has absolutely no warranty.  Enter \"help warranty\" for details.",
" ",
NULL
};

/*
 *  "help copying" output
 */
static void
display_copying_info(void)
{
	int i;

	switch (GPL_version)
	{
	case GPLv2:
		for (i = 0; !strstr(gnu_public_license[i], "NO WARRANTY"); i++)
			fprintf(fp, "%s\n", gnu_public_license[i]);
		break;

	case GPLv3:
		for (i = 0; !strstr(gnu_public_license_v3[i], 
		    "15. Disclaimer of Warranty."); i++)
			fprintf(fp, "%s\n", gnu_public_license_v3[i]);
		break;
	}
}

/*
 *  "help warranty" output
 */
static void
display_warranty_info(void)
{
	int i;

	switch (GPL_version)
	{
	case GPLv2:
		for (i = 0; !strstr(gnu_public_license[i], "NO WARRANTY"); i++)
			;

		do {
			fprintf(fp, "%s\n", gnu_public_license[i]);
		} while (!strstr(gnu_public_license[i++], "END OF TERMS"));
		break;

	case GPLv3:
		for (i = 0; !strstr(gnu_public_license_v3[i], 
		    "15. Disclaimer of Warranty."); i++)
			;

		while (!strstr(gnu_public_license_v3[i], "END OF TERMS"))
			fprintf(fp, "%s\n", gnu_public_license_v3[i++]);
		break;
	}
}

static
char *gnu_public_license[] = {
"",
"		    GNU GENERAL PUBLIC LICENSE",
"		       Version 2, June 1991",
"",
" Copyright (C) 1989, 1991 Free Software Foundation, Inc.",
"                       59 Temple Place, Suite 330, Boston, MA  02111-1307  USA",
" Everyone is permitted to copy and distribute verbatim copies",
" of this license document, but changing it is not allowed.",
"",
"			    Preamble",
"",
"  The licenses for most software are designed to take away your",
"freedom to share and change it.  By contrast, the GNU General Public",
"License is intended to guarantee your freedom to share and change free",
"software--to make sure the software is free for all its users.  This",
"General Public License applies to most of the Free Software",
"Foundation's software and to any other program whose authors commit to",
"using it.  (Some other Free Software Foundation software is covered by",
"the GNU Library General Public License instead.)  You can apply it to",
"your programs, too.",
"",
"  When we speak of free software, we are referring to freedom, not",
"price.  Our General Public Licenses are designed to make sure that you",
"have the freedom to distribute copies of free software (and charge for",
"this service if you wish), that you receive source code or can get it",
"if you want it, that you can change the software or use pieces of it",
"in new free programs; and that you know you can do these things.",
"",
"  To protect your rights, we need to make restrictions that forbid",
"anyone to deny you these rights or to ask you to surrender the rights.",
"These restrictions translate to certain responsibilities for you if you",
"distribute copies of the software, or if you modify it.",
"",
"  For example, if you distribute copies of such a program, whether",
"gratis or for a fee, you must give the recipients all the rights that",
"you have.  You must make sure that they, too, receive or can get the",
"source code.  And you must show them these terms so they know their",
"rights.",
"",
"  We protect your rights with two steps: (1) copyright the software, and",
"(2) offer you this license which gives you legal permission to copy,",
"distribute and/or modify the software.",
"",
"  Also, for each author's protection and ours, we want to make certain",
"that everyone understands that there is no warranty for this free",
"software.  If the software is modified by someone else and passed on, we",
"want its recipients to know that what they have is not the original, so",
"that any problems introduced by others will not reflect on the original",
"authors' reputations.",
"",
"  Finally, any free program is threatened constantly by software",
"patents.  We wish to avoid the danger that redistributors of a free",
"program will individually obtain patent licenses, in effect making the",
"program proprietary.  To prevent this, we have made it clear that any",
"patent must be licensed for everyone's free use or not licensed at all.",
"",
"  The precise terms and conditions for copying, distribution and",
"modification follow.",
"",
"		    GNU GENERAL PUBLIC LICENSE",
"   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION",
"",
"  0. This License applies to any program or other work which contains",
"a notice placed by the copyright holder saying it may be distributed",
"under the terms of this General Public License.  The \"Program\", below,",
"refers to any such program or work, and a \"work based on the Program\"",
"means either the Program or any derivative work under copyright law:",
"that is to say, a work containing the Program or a portion of it,",
"either verbatim or with modifications and/or translated into another",
"language.  (Hereinafter, translation is included without limitation in",
"the term \"modification\".)  Each licensee is addressed as \"you\".",
"",
"Activities other than copying, distribution and modification are not",
"covered by this License; they are outside its scope.  The act of",
"running the Program is not restricted, and the output from the Program",
"is covered only if its contents constitute a work based on the",
"Program (independent of having been made by running the Program).",
"Whether that is true depends on what the Program does.",
"",
"  1. You may copy and distribute verbatim copies of the Program's",
"source code as you receive it, in any medium, provided that you",
"conspicuously and appropriately publish on each copy an appropriate",
"copyright notice and disclaimer of warranty; keep intact all the",
"notices that refer to this License and to the absence of any warranty;",
"and give any other recipients of the Program a copy of this License",
"along with the Program.",
"",
"You may charge a fee for the physical act of transferring a copy, and",
"you may at your option offer warranty protection in exchange for a fee.",
"",
"  2. You may modify your copy or copies of the Program or any portion",
"of it, thus forming a work based on the Program, and copy and",
"distribute such modifications or work under the terms of Section 1",
"above, provided that you also meet all of these conditions:",
"",
"    a) You must cause the modified files to carry prominent notices",
"    stating that you changed the files and the date of any change.",
"",
"    b) You must cause any work that you distribute or publish, that in",
"    whole or in part contains or is derived from the Program or any",
"    part thereof, to be licensed as a whole at no charge to all third",
"    parties under the terms of this License.",
"",
"    c) If the modified program normally reads commands interactively",
"    when run, you must cause it, when started running for such",
"    interactive use in the most ordinary way, to print or display an",
"    announcement including an appropriate copyright notice and a",
"    notice that there is no warranty (or else, saying that you provide",
"    a warranty) and that users may redistribute the program under",
"    these conditions, and telling the user how to view a copy of this",
"    License.  (Exception: if the Program itself is interactive but",
"    does not normally print such an announcement, your work based on",
"    the Program is not required to print an announcement.)",
"",
"These requirements apply to the modified work as a whole.  If",
"identifiable sections of that work are not derived from the Program,",
"and can be reasonably considered independent and separate works in",
"themselves, then this License, and its terms, do not apply to those",
"sections when you distribute them as separate works.  But when you",
"distribute the same sections as part of a whole which is a work based",
"on the Program, the distribution of the whole must be on the terms of",
"this License, whose permissions for other licensees extend to the",
"entire whole, and thus to each and every part regardless of who wrote it.",
"",
"Thus, it is not the intent of this section to claim rights or contest",
"your rights to work written entirely by you; rather, the intent is to",
"exercise the right to control the distribution of derivative or",
"collective works based on the Program.",
"",
"In addition, mere aggregation of another work not based on the Program",
"with the Program (or with a work based on the Program) on a volume of",
"a storage or distribution medium does not bring the other work under",
"the scope of this License.",
"",
"  3. You may copy and distribute the Program (or a work based on it,",
"under Section 2) in object code or executable form under the terms of",
"Sections 1 and 2 above provided that you also do one of the following:",
"",
"    a) Accompany it with the complete corresponding machine-readable",
"    source code, which must be distributed under the terms of Sections",
"    1 and 2 above on a medium customarily used for software interchange; or,",
"",
"    b) Accompany it with a written offer, valid for at least three",
"    years, to give any third party, for a charge no more than your",
"    cost of physically performing source distribution, a complete",
"    machine-readable copy of the corresponding source code, to be",
"    distributed under the terms of Sections 1 and 2 above on a medium",
"    customarily used for software interchange; or,",
"",
"    c) Accompany it with the information you received as to the offer",
"    to distribute corresponding source code.  (This alternative is",
"    allowed only for noncommercial distribution and only if you",
"    received the program in object code or executable form with such",
"    an offer, in accord with Subsection b above.)",
"",
"The source code for a work means the preferred form of the work for",
"making modifications to it.  For an executable work, complete source",
"code means all the source code for all modules it contains, plus any",
"associated interface definition files, plus the scripts used to",
"control compilation and installation of the executable.  However, as a",
"special exception, the source code distributed need not include",
"anything that is normally distributed (in either source or binary",
"form) with the major components (compiler, kernel, and so on) of the",
"operating system on which the executable runs, unless that component",
"itself accompanies the executable.",
"",
"If distribution of executable or object code is made by offering",
"access to copy from a designated place, then offering equivalent",
"access to copy the source code from the same place counts as",
"distribution of the source code, even though third parties are not",
"compelled to copy the source along with the object code.",
"",
"  4. You may not copy, modify, sublicense, or distribute the Program",
"except as expressly provided under this License.  Any attempt",
"otherwise to copy, modify, sublicense or distribute the Program is",
"void, and will automatically terminate your rights under this License.",
"However, parties who have received copies, or rights, from you under",
"this License will not have their licenses terminated so long as such",
"parties remain in full compliance.",
"",
"  5. You are not required to accept this License, since you have not",
"signed it.  However, nothing else grants you permission to modify or",
"distribute the Program or its derivative works.  These actions are",
"prohibited by law if you do not accept this License.  Therefore, by",
"modifying or distributing the Program (or any work based on the",
"Program), you indicate your acceptance of this License to do so, and",
"all its terms and conditions for copying, distributing or modifying",
"the Program or works based on it.",
"",
"  6. Each time you redistribute the Program (or any work based on the",
"Program), the recipient automatically receives a license from the",
"original licensor to copy, distribute or modify the Program subject to",
"these terms and conditions.  You may not impose any further",
"restrictions on the recipients' exercise of the rights granted herein.",
"You are not responsible for enforcing compliance by third parties to",
"this License.",
"",
"  7. If, as a consequence of a court judgment or allegation of patent",
"infringement or for any other reason (not limited to patent issues),",
"conditions are imposed on you (whether by court order, agreement or",
"otherwise) that contradict the conditions of this License, they do not",
"excuse you from the conditions of this License.  If you cannot",
"distribute so as to satisfy simultaneously your obligations under this",
"License and any other pertinent obligations, then as a consequence you",
"may not distribute the Program at all.  For example, if a patent",
"license would not permit royalty-free redistribution of the Program by",
"all those who receive copies directly or indirectly through you, then",
"the only way you could satisfy both it and this License would be to",
"refrain entirely from distribution of the Program.",
"",
"If any portion of this section is held invalid or unenforceable under",
"any particular circumstance, the balance of the section is intended to",
"apply and the section as a whole is intended to apply in other",
"circumstances.",
"",
"It is not the purpose of this section to induce you to infringe any",
"patents or other property right claims or to contest validity of any",
"such claims; this section has the sole purpose of protecting the",
"integrity of the free software distribution system, which is",
"implemented by public license practices.  Many people have made",
"generous contributions to the wide range of software distributed",
"through that system in reliance on consistent application of that",
"system; it is up to the author/donor to decide if he or she is willing",
"to distribute software through any other system and a licensee cannot",
"impose that choice.",
"",
"This section is intended to make thoroughly clear what is believed to",
"be a consequence of the rest of this License.",
"",
"  8. If the distribution and/or use of the Program is restricted in",
"certain countries either by patents or by copyrighted interfaces, the",
"original copyright holder who places the Program under this License",
"may add an explicit geographical distribution limitation excluding",
"those countries, so that distribution is permitted only in or among",
"countries not thus excluded.  In such case, this License incorporates",
"the limitation as if written in the body of this License.",
"",
"  9. The Free Software Foundation may publish revised and/or new versions",
"of the General Public License from time to time.  Such new versions will",
"be similar in spirit to the present version, but may differ in detail to",
"address new problems or concerns.",
"",
"Each version is given a distinguishing version number.  If the Program",
"specifies a version number of this License which applies to it and \"any",
"later version\", you have the option of following the terms and conditions",
"either of that version or of any later version published by the Free",
"Software Foundation.  If the Program does not specify a version number of",
"this License, you may choose any version ever published by the Free Software",
"Foundation.",
"",
"  10. If you wish to incorporate parts of the Program into other free",
"programs whose distribution conditions are different, write to the author",
"to ask for permission.  For software which is copyrighted by the Free",
"Software Foundation, write to the Free Software Foundation; we sometimes",
"make exceptions for this.  Our decision will be guided by the two goals",
"of preserving the free status of all derivatives of our free software and",
"of promoting the sharing and reuse of software generally.",
"",
"\n			    NO WARRANTY",
"",
"  11. BECAUSE THE PROGRAM IS LICENSED FREE OF CHARGE, THERE IS NO WARRANTY",
"FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW.  EXCEPT WHEN",
"OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES",
"PROVIDE THE PROGRAM \"AS IS\" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED",
"OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF",
"MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE ENTIRE RISK AS",
"TO THE QUALITY AND PERFORMANCE OF THE PROGRAM IS WITH YOU.  SHOULD THE",
"PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING,",
"REPAIR OR CORRECTION.",
"",
"  12. IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING",
"WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MAY MODIFY AND/OR",
"REDISTRIBUTE THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR DAMAGES,",
"INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING",
"OUT OF THE USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED",
"TO LOSS OF DATA OR DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY",
"YOU OR THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER",
"PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE",
"POSSIBILITY OF SUCH DAMAGES.",
"",
"		     END OF TERMS AND CONDITIONS\n",
};

static
char *gnu_public_license_v3[] = {
"                    GNU GENERAL PUBLIC LICENSE",
"                       Version 3, 29 June 2007",
"",
" Copyright (C) 2007 Free Software Foundation, Inc. <http://fsf.org/>",
" Everyone is permitted to copy and distribute verbatim copies",
" of this license document, but changing it is not allowed.",
"",
"                            Preamble",
"",
"  The GNU General Public License is a free, copyleft license for",
"software and other kinds of works.",
"",
"  The licenses for most software and other practical works are designed",
"to take away your freedom to share and change the works.  By contrast,",
"the GNU General Public License is intended to guarantee your freedom to",
"share and change all versions of a program--to make sure it remains free",
"software for all its users.  We, the Free Software Foundation, use the",
"GNU General Public License for most of our software; it applies also to",
"any other work released this way by its authors.  You can apply it to",
"your programs, too.",
"",
"  When we speak of free software, we are referring to freedom, not",
"price.  Our General Public Licenses are designed to make sure that you",
"have the freedom to distribute copies of free software (and charge for",
"them if you wish), that you receive source code or can get it if you",
"want it, that you can change the software or use pieces of it in new",
"free programs, and that you know you can do these things.",
"",
"  To protect your rights, we need to prevent others from denying you",
"these rights or asking you to surrender the rights.  Therefore, you have",
"certain responsibilities if you distribute copies of the software, or if",
"you modify it: responsibilities to respect the freedom of others.",
"",
"  For example, if you distribute copies of such a program, whether",
"gratis or for a fee, you must pass on to the recipients the same",
"freedoms that you received.  You must make sure that they, too, receive",
"or can get the source code.  And you must show them these terms so they",
"know their rights.",
"",
"  Developers that use the GNU GPL protect your rights with two steps:",
"(1) assert copyright on the software, and (2) offer you this License",
"giving you legal permission to copy, distribute and/or modify it.",
"",
"  For the developers' and authors' protection, the GPL clearly explains",
"that there is no warranty for this free software.  For both users' and",
"authors' sake, the GPL requires that modified versions be marked as",
"changed, so that their problems will not be attributed erroneously to",
"authors of previous versions.",
"",
"  Some devices are designed to deny users access to install or run",
"modified versions of the software inside them, although the manufacturer",
"can do so.  This is fundamentally incompatible with the aim of",
"protecting users' freedom to change the software.  The systematic",
"pattern of such abuse occurs in the area of products for individuals to",
"use, which is precisely where it is most unacceptable.  Therefore, we",
"have designed this version of the GPL to prohibit the practice for those",
"products.  If such problems arise substantially in other domains, we",
"stand ready to extend this provision to those domains in future versions",
"of the GPL, as needed to protect the freedom of users.",
"",
"  Finally, every program is threatened constantly by software patents.",
"States should not allow patents to restrict development and use of",
"software on general-purpose computers, but in those that do, we wish to",
"avoid the special danger that patents applied to a free program could",
"make it effectively proprietary.  To prevent this, the GPL assures that",
"patents cannot be used to render the program non-free.",
"",
"  The precise terms and conditions for copying, distribution and",
"modification follow.",
"",
"                       TERMS AND CONDITIONS",
"",
"  0. Definitions.",
"",
"  \"This License\" refers to version 3 of the GNU General Public License.",
"",
"  \"Copyright\" also means copyright-like laws that apply to other kinds of",
"works, such as semiconductor masks.",
"",
"  \"The Program\" refers to any copyrightable work licensed under this",
"License.  Each licensee is addressed as \"you\".  \"Licensees\" and",
"\"recipients\" may be individuals or organizations.",
"",
"  To \"modify\" a work means to copy from or adapt all or part of the work",
"in a fashion requiring copyright permission, other than the making of an",
"exact copy.  The resulting work is called a \"modified version\" of the",
"earlier work or a work \"based on\" the earlier work.",
"",
"  A \"covered work\" means either the unmodified Program or a work based",
"on the Program.",
"",
"  To \"propagate\" a work means to do anything with it that, without",
"permission, would make you directly or secondarily liable for",
"infringement under applicable copyright law, except executing it on a",
"computer or modifying a private copy.  Propagation includes copying,",
"distribution (with or without modification), making available to the",
"public, and in some countries other activities as well.",
"",
"  To \"convey\" a work means any kind of propagation that enables other",
"parties to make or receive copies.  Mere interaction with a user through",
"a computer network, with no transfer of a copy, is not conveying.",
"",
"  An interactive user interface displays \"Appropriate Legal Notices\"",
"to the extent that it includes a convenient and prominently visible",
"feature that (1) displays an appropriate copyright notice, and (2)",
"tells the user that there is no warranty for the work (except to the",
"extent that warranties are provided), that licensees may convey the",
"work under this License, and how to view a copy of this License.  If",
"the interface presents a list of user commands or options, such as a",
"menu, a prominent item in the list meets this criterion.",
"",
"  1. Source Code.",
"",
"  The \"source code\" for a work means the preferred form of the work",
"for making modifications to it.  \"Object code\" means any non-source",
"form of a work.",
"",
"  A \"Standard Interface\" means an interface that either is an official",
"standard defined by a recognized standards body, or, in the case of",
"interfaces specified for a particular programming language, one that",
"is widely used among developers working in that language.",
"",
"  The \"System Libraries\" of an executable work include anything, other",
"than the work as a whole, that (a) is included in the normal form of",
"packaging a Major Component, but which is not part of that Major",
"Component, and (b) serves only to enable use of the work with that",
"Major Component, or to implement a Standard Interface for which an",
"implementation is available to the public in source code form.  A",
"\"Major Component\", in this context, means a major essential component",
"(kernel, window system, and so on) of the specific operating system",
"(if any) on which the executable work runs, or a compiler used to",
"produce the work, or an object code interpreter used to run it.",
"",
"  The \"Corresponding Source\" for a work in object code form means all",
"the source code needed to generate, install, and (for an executable",
"work) run the object code and to modify the work, including scripts to",
"control those activities.  However, it does not include the work's",
"System Libraries, or general-purpose tools or generally available free",
"programs which are used unmodified in performing those activities but",
"which are not part of the work.  For example, Corresponding Source",
"includes interface definition files associated with source files for",
"the work, and the source code for shared libraries and dynamically",
"linked subprograms that the work is specifically designed to require,",
"such as by intimate data communication or control flow between those",
"subprograms and other parts of the work.",
"",
"  The Corresponding Source need not include anything that users",
"can regenerate automatically from other parts of the Corresponding",
"Source.",
"",
"  The Corresponding Source for a work in source code form is that",
"same work.",
"",
"  2. Basic Permissions.",
"",
"  All rights granted under this License are granted for the term of",
"copyright on the Program, and are irrevocable provided the stated",
"conditions are met.  This License explicitly affirms your unlimited",
"permission to run the unmodified Program.  The output from running a",
"covered work is covered by this License only if the output, given its",
"content, constitutes a covered work.  This License acknowledges your",
"rights of fair use or other equivalent, as provided by copyright law.",
"",
"  You may make, run and propagate covered works that you do not",
"convey, without conditions so long as your license otherwise remains",
"in force.  You may convey covered works to others for the sole purpose",
"of having them make modifications exclusively for you, or provide you",
"with facilities for running those works, provided that you comply with",
"the terms of this License in conveying all material for which you do",
"not control copyright.  Those thus making or running the covered works",
"for you must do so exclusively on your behalf, under your direction",
"and control, on terms that prohibit them from making any copies of",
"your copyrighted material outside their relationship with you.",
"",
"  Conveying under any other circumstances is permitted solely under",
"the conditions stated below.  Sublicensing is not allowed; section 10",
"makes it unnecessary.",
"",
"  3. Protecting Users' Legal Rights From Anti-Circumvention Law.",
"",
"  No covered work shall be deemed part of an effective technological",
"measure under any applicable law fulfilling obligations under article",
"11 of the WIPO copyright treaty adopted on 20 December 1996, or",
"similar laws prohibiting or restricting circumvention of such",
"measures.",
"",
"  When you convey a covered work, you waive any legal power to forbid",
"circumvention of technological measures to the extent such circumvention",
"is effected by exercising rights under this License with respect to",
"the covered work, and you disclaim any intention to limit operation or",
"modification of the work as a means of enforcing, against the work's",
"users, your or third parties' legal rights to forbid circumvention of",
"technological measures.",
"",
"  4. Conveying Verbatim Copies.",
"",
"  You may convey verbatim copies of the Program's source code as you",
"receive it, in any medium, provided that you conspicuously and",
"appropriately publish on each copy an appropriate copyright notice;",
"keep intact all notices stating that this License and any",
"non-permissive terms added in accord with section 7 apply to the code;",
"keep intact all notices of the absence of any warranty; and give all",
"recipients a copy of this License along with the Program.",
"",
"  You may charge any price or no price for each copy that you convey,",
"and you may offer support or warranty protection for a fee.",
"",
"  5. Conveying Modified Source Versions.",
"",
"  You may convey a work based on the Program, or the modifications to",
"produce it from the Program, in the form of source code under the",
"terms of section 4, provided that you also meet all of these conditions:",
"",
"    a) The work must carry prominent notices stating that you modified",
"    it, and giving a relevant date.",
"",
"    b) The work must carry prominent notices stating that it is",
"    released under this License and any conditions added under section",
"    7.  This requirement modifies the requirement in section 4 to",
"    \"keep intact all notices\".",
"",
"    c) You must license the entire work, as a whole, under this",
"    License to anyone who comes into possession of a copy.  This",
"    License will therefore apply, along with any applicable section 7",
"    additional terms, to the whole of the work, and all its parts,",
"    regardless of how they are packaged.  This License gives no",
"    permission to license the work in any other way, but it does not",
"    invalidate such permission if you have separately received it.",
"",
"    d) If the work has interactive user interfaces, each must display",
"    Appropriate Legal Notices; however, if the Program has interactive",
"    interfaces that do not display Appropriate Legal Notices, your",
"    work need not make them do so.",
"",
"  A compilation of a covered work with other separate and independent",
"works, which are not by their nature extensions of the covered work,",
"and which are not combined with it such as to form a larger program,",
"in or on a volume of a storage or distribution medium, is called an",
"\"aggregate\" if the compilation and its resulting copyright are not",
"used to limit the access or legal rights of the compilation's users",
"beyond what the individual works permit.  Inclusion of a covered work",
"in an aggregate does not cause this License to apply to the other",
"parts of the aggregate.",
"",
"  6. Conveying Non-Source Forms.",
"",
"  You may convey a covered work in object code form under the terms",
"of sections 4 and 5, provided that you also convey the",
"machine-readable Corresponding Source under the terms of this License,",
"in one of these ways:",
"",
"    a) Convey the object code in, or embodied in, a physical product",
"    (including a physical distribution medium), accompanied by the",
"    Corresponding Source fixed on a durable physical medium",
"    customarily used for software interchange.",
"",
"    b) Convey the object code in, or embodied in, a physical product",
"    (including a physical distribution medium), accompanied by a",
"    written offer, valid for at least three years and valid for as",
"    long as you offer spare parts or customer support for that product",
"    model, to give anyone who possesses the object code either (1) a",
"    copy of the Corresponding Source for all the software in the",
"    product that is covered by this License, on a durable physical",
"    medium customarily used for software interchange, for a price no",
"    more than your reasonable cost of physically performing this",
"    conveying of source, or (2) access to copy the",
"    Corresponding Source from a network server at no charge.",
"",
"    c) Convey individual copies of the object code with a copy of the",
"    written offer to provide the Corresponding Source.  This",
"    alternative is allowed only occasionally and noncommercially, and",
"    only if you received the object code with such an offer, in accord",
"    with subsection 6b.",
"",
"    d) Convey the object code by offering access from a designated",
"    place (gratis or for a charge), and offer equivalent access to the",
"    Corresponding Source in the same way through the same place at no",
"    further charge.  You need not require recipients to copy the",
"    Corresponding Source along with the object code.  If the place to",
"    copy the object code is a network server, the Corresponding Source",
"    may be on a different server (operated by you or a third party)",
"    that supports equivalent copying facilities, provided you maintain",
"    clear directions next to the object code saying where to find the",
"    Corresponding Source.  Regardless of what server hosts the",
"    Corresponding Source, you remain obligated to ensure that it is",
"    available for as long as needed to satisfy these requirements.",
"",
"    e) Convey the object code using peer-to-peer transmission, provided",
"    you inform other peers where the object code and Corresponding",
"    Source of the work are being offered to the general public at no",
"    charge under subsection 6d.",
"",
"  A separable portion of the object code, whose source code is excluded",
"from the Corresponding Source as a System Library, need not be",
"included in conveying the object code work.",
"",
"  A \"User Product\" is either (1) a \"consumer product\", which means any",
"tangible personal property which is normally used for personal, family,",
"or household purposes, or (2) anything designed or sold for incorporation",
"into a dwelling.  In determining whether a product is a consumer product,",
"doubtful cases shall be resolved in favor of coverage.  For a particular",
"product received by a particular user, \"normally used\" refers to a",
"typical or common use of that class of product, regardless of the status",
"of the particular user or of the way in which the particular user",
"actually uses, or expects or is expected to use, the product.  A product",
"is a consumer product regardless of whether the product has substantial",
"commercial, industrial or non-consumer uses, unless such uses represent",
"the only significant mode of use of the product.",
"",
"  \"Installation Information\" for a User Product means any methods,",
"procedures, authorization keys, or other information required to install",
"and execute modified versions of a covered work in that User Product from",
"a modified version of its Corresponding Source.  The information must",
"suffice to ensure that the continued functioning of the modified object",
"code is in no case prevented or interfered with solely because",
"modification has been made.",
"",
"  If you convey an object code work under this section in, or with, or",
"specifically for use in, a User Product, and the conveying occurs as",
"part of a transaction in which the right of possession and use of the",
"User Product is transferred to the recipient in perpetuity or for a",
"fixed term (regardless of how the transaction is characterized), the",
"Corresponding Source conveyed under this section must be accompanied",
"by the Installation Information.  But this requirement does not apply",
"if neither you nor any third party retains the ability to install",
"modified object code on the User Product (for example, the work has",
"been installed in ROM).",
"",
"  The requirement to provide Installation Information does not include a",
"requirement to continue to provide support service, warranty, or updates",
"for a work that has been modified or installed by the recipient, or for",
"the User Product in which it has been modified or installed.  Access to a",
"network may be denied when the modification itself materially and",
"adversely affects the operation of the network or violates the rules and",
"protocols for communication across the network.",
"",
"  Corresponding Source conveyed, and Installation Information provided,",
"in accord with this section must be in a format that is publicly",
"documented (and with an implementation available to the public in",
"source code form), and must require no special password or key for",
"unpacking, reading or copying.",
"",
"  7. Additional Terms.",
"",
"  \"Additional permissions\" are terms that supplement the terms of this",
"License by making exceptions from one or more of its conditions.",
"Additional permissions that are applicable to the entire Program shall",
"be treated as though they were included in this License, to the extent",
"that they are valid under applicable law.  If additional permissions",
"apply only to part of the Program, that part may be used separately",
"under those permissions, but the entire Program remains governed by",
"this License without regard to the additional permissions.",
"",
"  When you convey a copy of a covered work, you may at your option",
"remove any additional permissions from that copy, or from any part of",
"it.  (Additional permissions may be written to require their own",
"removal in certain cases when you modify the work.)  You may place",
"additional permissions on material, added by you to a covered work,",
"for which you have or can give appropriate copyright permission.",
"",
"  Notwithstanding any other provision of this License, for material you",
"add to a covered work, you may (if authorized by the copyright holders of",
"that material) supplement the terms of this License with terms:",
"",
"    a) Disclaiming warranty or limiting liability differently from the",
"    terms of sections 15 and 16 of this License; or",
"",
"    b) Requiring preservation of specified reasonable legal notices or",
"    author attributions in that material or in the Appropriate Legal",
"    Notices displayed by works containing it; or",
"",
"    c) Prohibiting misrepresentation of the origin of that material, or",
"    requiring that modified versions of such material be marked in",
"    reasonable ways as different from the original version; or",
"",
"    d) Limiting the use for publicity purposes of names of licensors or",
"    authors of the material; or",
"",
"    e) Declining to grant rights under trademark law for use of some",
"    trade names, trademarks, or service marks; or",
"",
"    f) Requiring indemnification of licensors and authors of that",
"    material by anyone who conveys the material (or modified versions of",
"    it) with contractual assumptions of liability to the recipient, for",
"    any liability that these contractual assumptions directly impose on",
"    those licensors and authors.",
"",
"  All other non-permissive additional terms are considered \"further",
"restrictions\" within the meaning of section 10.  If the Program as you",
"received it, or any part of it, contains a notice stating that it is",
"governed by this License along with a term that is a further",
"restriction, you may remove that term.  If a license document contains",
"a further restriction but permits relicensing or conveying under this",
"License, you may add to a covered work material governed by the terms",
"of that license document, provided that the further restriction does",
"not survive such relicensing or conveying.",
"",
"  If you add terms to a covered work in accord with this section, you",
"must place, in the relevant source files, a statement of the",
"additional terms that apply to those files, or a notice indicating",
"where to find the applicable terms.",
"",
"  Additional terms, permissive or non-permissive, may be stated in the",
"form of a separately written license, or stated as exceptions;",
"the above requirements apply either way.",
"",
"  8. Termination.",
"",
"  You may not propagate or modify a covered work except as expressly",
"provided under this License.  Any attempt otherwise to propagate or",
"modify it is void, and will automatically terminate your rights under",
"this License (including any patent licenses granted under the third",
"paragraph of section 11).",
"",
"  However, if you cease all violation of this License, then your",
"license from a particular copyright holder is reinstated (a)",
"provisionally, unless and until the copyright holder explicitly and",
"finally terminates your license, and (b) permanently, if the copyright",
"holder fails to notify you of the violation by some reasonable means",
"prior to 60 days after the cessation.",
"",
"  Moreover, your license from a particular copyright holder is",
"reinstated permanently if the copyright holder notifies you of the",
"violation by some reasonable means, this is the first time you have",
"received notice of violation of this License (for any work) from that",
"copyright holder, and you cure the violation prior to 30 days after",
"your receipt of the notice.",
"",
"  Termination of your rights under this section does not terminate the",
"licenses of parties who have received copies or rights from you under",
"this License.  If your rights have been terminated and not permanently",
"reinstated, you do not qualify to receive new licenses for the same",
"material under section 10.",
"",
"  9. Acceptance Not Required for Having Copies.",
"",
"  You are not required to accept this License in order to receive or",
"run a copy of the Program.  Ancillary propagation of a covered work",
"occurring solely as a consequence of using peer-to-peer transmission",
"to receive a copy likewise does not require acceptance.  However,",
"nothing other than this License grants you permission to propagate or",
"modify any covered work.  These actions infringe copyright if you do",
"not accept this License.  Therefore, by modifying or propagating a",
"covered work, you indicate your acceptance of this License to do so.",
"",
"  10. Automatic Licensing of Downstream Recipients.",
"",
"  Each time you convey a covered work, the recipient automatically",
"receives a license from the original licensors, to run, modify and",
"propagate that work, subject to this License.  You are not responsible",
"for enforcing compliance by third parties with this License.",
"",
"  An \"entity transaction\" is a transaction transferring control of an",
"organization, or substantially all assets of one, or subdividing an",
"organization, or merging organizations.  If propagation of a covered",
"work results from an entity transaction, each party to that",
"transaction who receives a copy of the work also receives whatever",
"licenses to the work the party's predecessor in interest had or could",
"give under the previous paragraph, plus a right to possession of the",
"Corresponding Source of the work from the predecessor in interest, if",
"the predecessor has it or can get it with reasonable efforts.",
"",
"  You may not impose any further restrictions on the exercise of the",
"rights granted or affirmed under this License.  For example, you may",
"not impose a license fee, royalty, or other charge for exercise of",
"rights granted under this License, and you may not initiate litigation",
"(including a cross-claim or counterclaim in a lawsuit) alleging that",
"any patent claim is infringed by making, using, selling, offering for",
"sale, or importing the Program or any portion of it.",
"",
"  11. Patents.",
"",
"  A \"contributor\" is a copyright holder who authorizes use under this",
"License of the Program or a work on which the Program is based.  The",
"work thus licensed is called the contributor's \"contributor version\".",
"",
"  A contributor's \"essential patent claims\" are all patent claims",
"owned or controlled by the contributor, whether already acquired or",
"hereafter acquired, that would be infringed by some manner, permitted",
"by this License, of making, using, or selling its contributor version,",
"but do not include claims that would be infringed only as a",
"consequence of further modification of the contributor version.  For",
"purposes of this definition, \"control\" includes the right to grant",
"patent sublicenses in a manner consistent with the requirements of",
"this License.",
"",
"  Each contributor grants you a non-exclusive, worldwide, royalty-free",
"patent license under the contributor's essential patent claims, to",
"make, use, sell, offer for sale, import and otherwise run, modify and",
"propagate the contents of its contributor version.",
"",
"  In the following three paragraphs, a \"patent license\" is any express",
"agreement or commitment, however denominated, not to enforce a patent",
"(such as an express permission to practice a patent or covenant not to",
"sue for patent infringement).  To \"grant\" such a patent license to a",
"party means to make such an agreement or commitment not to enforce a",
"patent against the party.",
"",
"  If you convey a covered work, knowingly relying on a patent license,",
"and the Corresponding Source of the work is not available for anyone",
"to copy, free of charge and under the terms of this License, through a",
"publicly available network server or other readily accessible means,",
"then you must either (1) cause the Corresponding Source to be so",
"available, or (2) arrange to deprive yourself of the benefit of the",
"patent license for this particular work, or (3) arrange, in a manner",
"consistent with the requirements of this License, to extend the patent",
"license to downstream recipients.  \"Knowingly relying\" means you have",
"actual knowledge that, but for the patent license, your conveying the",
"covered work in a country, or your recipient's use of the covered work",
"in a country, would infringe one or more identifiable patents in that",
"country that you have reason to believe are valid.",
"",
"  If, pursuant to or in connection with a single transaction or",
"arrangement, you convey, or propagate by procuring conveyance of, a",
"covered work, and grant a patent license to some of the parties",
"receiving the covered work authorizing them to use, propagate, modify",
"or convey a specific copy of the covered work, then the patent license",
"you grant is automatically extended to all recipients of the covered",
"work and works based on it.",
"",
"  A patent license is \"discriminatory\" if it does not include within",
"the scope of its coverage, prohibits the exercise of, or is",
"conditioned on the non-exercise of one or more of the rights that are",
"specifically granted under this License.  You may not convey a covered",
"work if you are a party to an arrangement with a third party that is",
"in the business of distributing software, under which you make payment",
"to the third party based on the extent of your activity of conveying",
"the work, and under which the third party grants, to any of the",
"parties who would receive the covered work from you, a discriminatory",
"patent license (a) in connection with copies of the covered work",
"conveyed by you (or copies made from those copies), or (b) primarily",
"for and in connection with specific products or compilations that",
"contain the covered work, unless you entered into that arrangement,",
"or that patent license was granted, prior to 28 March 2007.",
"",
"  Nothing in this License shall be construed as excluding or limiting",
"any implied license or other defenses to infringement that may",
"otherwise be available to you under applicable patent law.",
"",
"  12. No Surrender of Others' Freedom.",
"",
"  If conditions are imposed on you (whether by court order, agreement or",
"otherwise) that contradict the conditions of this License, they do not",
"excuse you from the conditions of this License.  If you cannot convey a",
"covered work so as to satisfy simultaneously your obligations under this",
"License and any other pertinent obligations, then as a consequence you may",
"not convey it at all.  For example, if you agree to terms that obligate you",
"to collect a royalty for further conveying from those to whom you convey",
"the Program, the only way you could satisfy both those terms and this",
"License would be to refrain entirely from conveying the Program.",
"",
"  13. Use with the GNU Affero General Public License.",
"",
"  Notwithstanding any other provision of this License, you have",
"permission to link or combine any covered work with a work licensed",
"under version 3 of the GNU Affero General Public License into a single",
"combined work, and to convey the resulting work.  The terms of this",
"License will continue to apply to the part which is the covered work,",
"but the special requirements of the GNU Affero General Public License,",
"section 13, concerning interaction through a network will apply to the",
"combination as such.",
"",
"  14. Revised Versions of this License.",
"",
"  The Free Software Foundation may publish revised and/or new versions of",
"the GNU General Public License from time to time.  Such new versions will",
"be similar in spirit to the present version, but may differ in detail to",
"address new problems or concerns.",
"",
"  Each version is given a distinguishing version number.  If the",
"Program specifies that a certain numbered version of the GNU General",
"Public License \"or any later version\" applies to it, you have the",
"option of following the terms and conditions either of that numbered",
"version or of any later version published by the Free Software",
"Foundation.  If the Program does not specify a version number of the",
"GNU General Public License, you may choose any version ever published",
"by the Free Software Foundation.",
"",
"  If the Program specifies that a proxy can decide which future",
"versions of the GNU General Public License can be used, that proxy's",
"public statement of acceptance of a version permanently authorizes you",
"to choose that version for the Program.",
"",
"  Later license versions may give you additional or different",
"permissions.  However, no additional obligations are imposed on any",
"author or copyright holder as a result of your choosing to follow a",
"later version.",
"",
"  15. Disclaimer of Warranty.",
"",
"  THERE IS NO WARRANTY FOR THE PROGRAM, TO THE EXTENT PERMITTED BY",
"APPLICABLE LAW.  EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT",
"HOLDERS AND/OR OTHER PARTIES PROVIDE THE PROGRAM \"AS IS\" WITHOUT WARRANTY",
"OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO,",
"THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR",
"PURPOSE.  THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE PROGRAM",
"IS WITH YOU.  SHOULD THE PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF",
"ALL NECESSARY SERVICING, REPAIR OR CORRECTION.",
"",
"  16. Limitation of Liability.",
"",
"  IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING",
"WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MODIFIES AND/OR CONVEYS",
"THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR DAMAGES, INCLUDING ANY",
"GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE",
"USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED TO LOSS OF",
"DATA OR DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY YOU OR THIRD",
"PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER PROGRAMS),",
"EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE POSSIBILITY OF",
"SUCH DAMAGES.",
"",
"  17. Interpretation of Sections 15 and 16.",
"",
"  If the disclaimer of warranty and limitation of liability provided",
"above cannot be given local legal effect according to their terms,",
"reviewing courts shall apply local law that most closely approximates",
"an absolute waiver of all civil liability in connection with the",
"Program, unless a warranty or assumption of liability accompanies a",
"copy of the Program in return for a fee.",
"",
"                     END OF TERMS AND CONDITIONS",
};

#define README_CRASH_VERSION      "README_CRASH_VERSION"
#define README_GNU_GDB_VERSION    "README_GNU_GDB_VERSION"
#define README_DATE               "README_DATE"
#define README_GPL_INFO           "README_GPL_INFO"
#define README_HELP_MENU          "README_HELP_MENU"
#define README_ENTER_DIRECTORY    "README_ENTER_DIRECTORY"
#define README_MEMORY_DRIVER      "README_MEMORY_DRIVER"

static void
display_README(void)
{
        int i, j;
    	time_t time_now;           

        for (i = 0; README[i]; i++) {
		if (STREQ(README[i], README_CRASH_VERSION)) {
			fprintf(fp, "    crash %s\n", pc->program_version);
		} else if (STREQ(README[i], README_GNU_GDB_VERSION)) {
			fprintf(fp, "    GNU gdb %s\n", pc->gdb_version);
		} else if (STREQ(README[i], README_DATE)) {
    			time(&time_now);               
			fprintf(fp, "            DATE: %s\n", ctime_tz(&time_now));
		} else if (STREQ(README[i], README_HELP_MENU)) {
			display_help_screen("    ");
		} else if (STREQ(README[i], README_GPL_INFO)) {
        		for (j = 0; version_info[j]; j++)
                		fprintf(fp, "    %s\n", version_info[j]);
		} else if (STREQ(README[i], README_ENTER_DIRECTORY)) {
			fprintf(fp, 
			    "    $ tar -xf crash-%s.tar.gz\n", 
					pc->program_version);
			fprintf(fp,
			    "    $ cd crash-%s\n", pc->program_version);
		} else if (STREQ(README[i], README_MEMORY_DRIVER)) {
			fprintf(fp, 
			    "  is included in the crash-%s/memory_driver subdirectory.\n",
				pc->program_version);
		} else 
			fprintf(fp, "%s\n", README[i]);
        }
}

static 
char *README[] = {
"",
"",
"                         CORE ANALYSIS SUITE",
"",
"  The core analysis suite is a self-contained tool that can be used to",
"  investigate either live systems, kernel core dumps created from dump",
"  creation facilities such as kdump, kvmdump, xendump, the netdump and",
"  diskdump packages offered by Red Hat, the LKCD kernel patch, the mcore",
"  kernel patch created by Mission Critical Linux, as well as other formats",
"  created by manufacturer-specific firmware.",
"",
"  o  The tool is loosely based on the SVR4 crash command, but has been",
"     completely integrated with gdb in order to be able to display ",
"     formatted kernel data structures, disassemble source code, etc.",
"     ",
"  o  The current set of available commands consist of common kernel core",
"     analysis tools such as a context-specific stack traces, source code",
"     disassembly, kernel variable displays, memory display, dumps of ",
"     linked-lists, etc.  In addition, any gdb command may be entered,",
"     which in turn will be passed onto the gdb module for execution.  ",
"",
"  o  There are several commands that delve deeper into specific kernel",
"     subsystems, which also serve as templates for kernel developers",
"     to create new commands for analysis of a specific area of interest.",
"     Adding a new command is a simple affair, and a quick recompile",
"     adds it to the command menu.",
"",
"  o  The intent is to make the tool independent of Linux version dependencies,",
"     building in recognition of major kernel code changes so as to adapt to ",
"     new kernel versions, while maintaining backwards compatibility.",
"",
"  A whitepaper with complete documentation concerning the use of this utility",
"  can be found here:",
" ",
"         https://crash-utility.github.io/crash_whitepaper.html",
" ",
"  These are the current prerequisites: ",
"",
"  o  At this point, x86, ia64, x86_64, ppc64, ppc, arm, arm64, alpha, mips,",
"     mips64, loongarch64, riscv64, s390 and s390x-based kernels are supported.",
"     Other architectures may be addressed in the future.",
"",
"  o  One size fits all -- the utility can be run on any Linux kernel version",
"     version dating back to 2.2.5-15.  A primary design goal is to always",
"     maintain backwards-compatibility.",
"",
"  o  In order to contain debugging data, the top-level kernel Makefile's CFLAGS",
"     definition must contain the -g flag.  Typically distributions will contain",
"     a package containing a vmlinux file with full debuginfo data.  If not, the",
"     kernel must be rebuilt:", 
"",
"     For 2.2 kernels that are not built with -g, change the following line:",
"",
"        CFLAGS = -Wall -Wstrict-prototypes -O2 -fomit-frame-pointer",
"",
"     to:",
"",
"        CFLAGS = -g -Wall -Wstrict-prototypes -O2 -fomit-frame-pointer",
"",
"     For 2.4 kernels that are not built with -g, change the following line:",
"",
"        CFLAGS := $(CPPFLAGS) -Wall -Wstrict-prototypes -O2 -fomit-frame-pointer -fno-strict-aliasing",
"",
"     to:",
"",
"        CFLAGS := -g $(CPPFLAGS) -Wall -Wstrict-prototypes -O2 -fomit-frame-pointer -fno-strict-aliasing",
"",
"     For 2.6 and later kernels that are not built with -g, the kernel should",
"     be configured with CONFIG_DEBUG_INFO enabled, which in turn will add",
"     the -g flag to the CFLAGS setting in the kernel Makefile.",
" ",
"     After the kernel is re-compiled, the uncompressed \"vmlinux\" kernel",
"     that is created in the top-level kernel build directory must be saved.",
"",
"  To build the crash utility: ",
"",
README_ENTER_DIRECTORY,
"    $ make",
"",
"  To cross compile the crash utility for aarch64 on x86_64: ",
"    $ make CROSS_COMPILE=aarch64-linux-gnu- -j`nproc`",
"",
"  Supported arches for cross compilation: x86_64, x86, aarch64, s390x, powerpc64, alpha, sparc64, mips, riscv64",
"",
"  The initial build will take several minutes  because the embedded gdb module",
"  must be configured and built.  Alternatively, the crash source RPM file",
"  may be installed and built, and the resultant crash binary RPM file installed.",
"",
"  The crash binary can only be used on systems of the same architecture as",
"  the host build system.  There are a few optional manners of building the",
"  crash binary:",
"",
"  o  On an x86_64 host, a 32-bit x86 binary that can be used to analyze",
"     32-bit x86 dumpfiles may be built by typing \"make target=X86\".",
"  o  On an x86 or x86_64 host, a 32-bit x86 binary that can be used to analyze",
"     32-bit arm dumpfiles may be built by typing \"make target=ARM\".",
"  o  On an x86 or x86_64 host, a 32-bit x86 binary that can be used to analyze",
"     32-bit mips dumpfiles may be built by typing \"make target=MIPS\".",
"  o  On an ppc64 host, a 32-bit ppc binary that can be used to analyze",
"     32-bit ppc dumpfiles may be built by typing \"make target=PPC\".",
"  o  On an x86_64 host, an x86_64 binary that can be used to analyze",
"     arm64 dumpfiles may be built by typing \"make target=ARM64\".",
"  o  On an x86_64 host, an x86_64 binary that can be used to analyze",
"     ppc64le dumpfiles may be built by typing \"make target=PPC64\".",
"  o  On an x86_64 host, an x86_64 binary that can be used to analyze",
"     riscv64 dumpfiles may be built by typing \"make target=RISCV64\".",
"  o  On an x86_64 host, an x86_64 binary that can be used to analyze",
"     loongarch64 dumpfiles may be built by typing \"make target=LOONGARCH64\".",
"",
"  Traditionally when vmcores are compressed via the makedumpfile(8) facility",
"  the libz compression library is used, and by default the crash utility",
"  only supports libz.  Recently makedumpfile has been enhanced to optionally",
"  use the LZO, snappy or zstd compression libraries.  To build crash with any",
"  or all of those libraries, type \"make lzo\", \"make snappy\" or \"make zstd\".",
"",
"  crash supports valgrind Memcheck tool on the crash's custom memory allocator.",
"  To build crash with this feature enabled, type \"make valgrind\" and then run",
"  crash with valgrind as \"valgrind crash vmlinux vmcore\".",
"",
"  All of the alternate build commands above are \"sticky\" in that the",
"  special \"make\" targets only have to be entered one time; all subsequent",
"  builds will follow suit.",
"",
"  If the tool is run against a kernel dumpfile, two arguments are required, the",
"  uncompressed kernel name and the kernel dumpfile name.  ",
"",
"  If run on a live system, only the kernel name is required, because /dev/mem ",
"  will be used as the \"dumpfile\".  On Red Hat or Fedora kernels where the",
"  /dev/mem device is restricted, the /dev/crash memory driver will be used.",
"  If neither /dev/mem or /dev/crash are available, then /proc/kcore will be",
"  be used as the live memory source.  If /proc/kcore is also restricted, then",
"  the Red Hat /dev/crash driver may be compiled and installed; its source",
README_MEMORY_DRIVER,
"",
"  If the kernel file is stored in /boot, /, /boot/efi, or in any /usr/src",
"  or /usr/lib/debug/lib/modules subdirectory, then no command line arguments",
"  are required -- the first kernel found that matches /proc/version will be",
"  used as the namelist.",
"  ",
"  For example, invoking crash on a live system would look like this:",
"",
"    $ crash",
"    ",
README_CRASH_VERSION,
README_GPL_INFO,
README_GNU_GDB_VERSION,
"    Copyright 2013 Free Software Foundation, Inc.",
"    License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>",
"    This is free software: you are free to change and redistribute it.",
"    There is NO WARRANTY, to the extent permitted by law.  Type \"show copying\"",
"    and \"show warranty\" for details.",
"    This GDB was configured as \"i686-pc-linux-gnu\"...",
"     ",
"          KERNEL: /boot/vmlinux",
"        DUMPFILE: /dev/mem",
"            CPUS: 1",
README_DATE,
"          UPTIME: 10 days, 22:55:18",
"    LOAD AVERAGE: 0.08, 0.03, 0.01",
"           TASKS: 42",
"        NODENAME: ha2.mclinux.com",
"         RELEASE: 2.4.0-test10",
"         VERSION: #11 SMP Thu Nov 4 15:09:25 EST 2000",
"         MACHINE: i686  (447 MHz)",
"	  MEMORY: 128 MB",
"             PID: 3621                                  ",
"         COMMAND: \"crash\"",
"            TASK: c463c000  ",
"             CPU: 0",
"           STATE: TASK_RUNNING (ACTIVE)",
"",
"    crash> help",
README_HELP_MENU,
"    crash> ",
" ",
"  When run on a dumpfile, both the kernel namelist and dumpfile must be ",
"  entered on the command line.  For example, when run on a core dump created",
"  by the Red Hat netdump or diskdump facilities:",
"",
"    $ crash vmlinux vmcore",
" ",   
README_CRASH_VERSION,
README_GPL_INFO,
README_GNU_GDB_VERSION,
"    Copyright 2013 Free Software Foundation, Inc.",
"    License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>",
"    This is free software: you are free to change and redistribute it.",
"    There is NO WARRANTY, to the extent permitted by law.  Type \"show copying\"",
"    and \"show warranty\" for details.",
"    This GDB was configured as \"i686-pc-linux-gnu\"...",
"    ",
"          KERNEL: vmlinux",
"        DUMPFILE: vmcore",
"            CPUS: 4",
"            DATE: Tue Mar  2 13:57:09 2004",
"          UPTIME: 00:02:40",
"    LOAD AVERAGE: 2.24, 0.96, 0.37",
"           TASKS: 70",
"        NODENAME: pro1.lab.boston.redhat.com",
"         RELEASE: 2.6.3-2.1.214.11smp",
"         VERSION: #1 SMP Tue Mar 2 10:58:27 EST 2004",
"         MACHINE: i686  (2785 Mhz)",
"          MEMORY: 512 MB",
"           PANIC: \"Oops: 0002 [#1]\" (check log for details)",
"             PID: 0",
"         COMMAND: \"swapper\"",
"            TASK: 22fa200  (1 of 4)  [THREAD_INFO: 2356000]",
"             CPU: 0",
"           STATE: TASK_RUNNING (PANIC)",
"    ",
"    crash> ",
"",
"  The tool's environment is context-specific.  On a live system, the default",
"  context is the command itself; on a dump the default context will be the",
"  task that panicked.  The most commonly-used commands are:",
"",
"    set     - set a new task context by pid, task address, or cpu.",
"    bt      - backtrace of the current context, or as specified with arguments.",
"    p       - print the contents of a kernel variable.",
"    rd      - read memory, which may be either kernel virtual, user virtual, or",
"              physical.",
"    ps      - simple process listing.",
"    log     - dump the kernel log_buf.",
"    struct  - print the contents of a structure at a specified address.",
"    foreach - execute a command on all tasks, or those specified, in the system.",
" ",
"  Detailed help concerning the use of each of the commands in the menu above ",
"  may be displayed by entering \"help command\", where \"command\" is one of those ",
"  listed above.  Rather than getting bogged down in details here, simply",
"  run the help command on each of the commands above.  Note that many commands",
"  have multiple options so as to avoid the proliferation of command names.",
"",
"  Command output may be piped to external commands or redirected to files.",
"  Enter \"help output\" for details.",
"",
"  The command line history mechanism allows for command-line recall and ",
"  command-line editing.  Input files containing a set of crash commands may ",
"  be substituted for command-line input.  Enter \"help input\" for details.",
"",
"  Note that a .crashrc file (or .<your-command-name>rc if the name has been ",
"  changed), may contain any number of \"set\" or \"alias\" commands -- see the",
"  help pages on those two commands for details.",
" ",
"  Lastly, if a command is entered that is not recognized, it is checked",
"  against the kernel's list of variables, structure, union or typedef names, ",
"  and if found, the command is passed to \"p\", \"struct\", \"union\" or \"whatis\".",
"  That being the case, as long as a kernel variable/structure/union name is ",
"  different than any of the current commands.",
"",
"  (1) A kernel variable can be dumped by simply entering its name:",
" ",
"      crash> init_mm",
"      init_mm = $2 = {",
"        mmap = 0xc022d540, ",
"        mmap_avl = 0x0, ",
"        mmap_cache = 0x0, ",
"        pgd = 0xc0101000, ",
"        count = {",
"          counter = 0x6",
"        }, ",
"        map_count = 0x1, ",
"        mmap_sem = {",
"          count = {",
"            counter = 0x1",
"          }, ",
"          waking = 0x0, ",
"          wait = 0x0",
"        }, ",
"        context = 0x0, ",
"        start_code = 0xc0000000, ",
"        end_code = 0xc022b4c8,",
"        end_data = c0250388,",
"        ...",
"      ",
"  (2) A structure or can be dumped simply by entering its name and address:  ",
"",
"      crash> vm_area_struct c5ba3910",
"      struct vm_area_struct {",
"        vm_mm = 0xc3ae3210, ",
"        vm_start = 0x821b000, ",
"        vm_end = 0x8692000, ",
"        vm_next = 0xc5ba3890, ",
"        vm_page_prot = {",
"          pgprot = 0x25",
"        }, ",
"        vm_flags = 0x77, ",
"        vm_avl_height = 0x4, ",
"        vm_avl_left = 0xc0499540, ",
"        vm_avl_right = 0xc0499f40, ",
"        vm_next_share = 0xc04993c0, ",
"        vm_pprev_share = 0xc0499060, ",
"        vm_ops = 0x0, ",
"        vm_offset = 0x0, ",
"        vm_file = 0x0, ",
"        vm_pte = 0x0",
"      }",
"",
"",
"  The crash utility has been designed to facilitate the task of adding new ",
"  commands.  New commands may be permanently compiled into the crash executable,",
"  or dynamically added during runtime using shared object files.",
" ",
"  To permanently add a new command to the crash executable's menu:",
"",
"    1. For a command named \"xxx\", put a reference to cmd_xxx() in defs.h.",
"   ",
"    2. Add cmd_xxx into the base_command_table[] array in global_data.c. ",
"",
"    3. Write cmd_xxx(), putting it in one of the appropriate files.  Look at ",
"       the other commands for guidance on getting symbolic data, reading",
"       memory, displaying data, etc...",
"",
"    4. Recompile and run.",
"",
"  Note that while the initial compile of crash, which configures and compiles",
"  the gdb module, takes several minutes, subsequent re-compiles to do such",
"  things as add new commands or fix bugs just takes a few seconds.",
"",
"  Alternatively, you can create shared object library files consisting of",
"  crash command extensions, that can be dynamically linked into the crash",
"  executable during runtime or during initialization.  This will allow",
"  the same shared object to be used with subsequent crash releases without",
"  having to re-merge the command's code into each new set of crash sources.",
"  The dynamically linked-in commands will automatically show up in the crash",
"  help menu.  For details, enter \"help extend\" during runtime, or enter",
"  \"crash -h extend\" from the shell command line.",
" ",
"",
"",
"",
"",
	0
};
