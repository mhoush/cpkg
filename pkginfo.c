/*
	pkginfo.c
*/

#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pkginfo.h"
#include "pkgutil.h"

int pkginfo_run(int argc, char *argv[]) {

	// generic count and index
	int c, i;

	// package database struct pointer
	struct packagedb *packagedb;

	// strings for passed options
	char *o_arg = NULL;
	char *o_root = NULL;

	char pkgdb[PATH_MAX];

	// flags for selected operating mode
	int o_footprint_mode = 0, o_installed_mode = 0, o_list_mode = 0, o_owner_mode = 0;

	// getopt(3) setup
	int opt = 0, option_index = 1;
	extern int optind, opterr, optopt;
	opterr = 0;
	optind = 1;

	static struct option long_options[] = {
		{ "footprint", required_argument, NULL, 'f' },
		{ "installed", no_argument,       NULL, 'i' },
		{ "list",      required_argument, NULL, 'l' },
		{ "owner",     required_argument, NULL, 'o' },
		{ "root",      required_argument, NULL, 'r' },
		{ 0,           0,                 0,    0   }
	};

	while ((opt = getopt_long(argc, argv, ":f:il:o:r:", long_options, &option_index)) != -1) {
		switch (opt) {
			case 'f':
				// footprint mode
				o_arg = strdup(optarg);
				o_footprint_mode = 1;
				break;
			case 'i':
				// installed mode
				o_installed_mode = 1;
				break;
			case 'l':
				// list mode
				o_list_mode = 1;
				o_arg = strdup(optarg);
				break;
			case 'o':
				// owner mode
				o_arg = strdup(optarg);
				o_owner_mode = 1;
				break;
			case 'r':
				// use alternate root
				o_root = strdup(optarg);
				break;
			case ':':
				printf("pkginfo: option -%c requires an argument.\n", optopt);
				exit(1);
		}
	}

	// check that a useful number of options is passed
	if (o_footprint_mode + o_installed_mode + o_list_mode + o_owner_mode > 1) {
		printf("pkginfo: only one of -f, -i, -l, or -o may be specified!\n");
		exit(1);
	}

	if (o_footprint_mode + o_installed_mode + o_list_mode + o_owner_mode == 0) {
		printf("pkginfo: one of -f, -i, -l, or -o is required!\n");
		exit(1);
	}

	// modes which don't require opening the package database
	if (o_footprint_mode == 1) {
		// print the footprint of the specified package file
		make_footprint(o_arg);
		free(o_arg);
	} else if (o_list_mode == 1 && (access(o_arg, F_OK) == 0)) {
		// list files in the specified package file
		struct package *pkg;
		pkg = create_package_from_archive(o_arg);
		list_files_in_package(pkg);
		free_package(pkg);
	} else {
		// modes which require opening the package database

		// use alternate root if specified
		if (o_root) {
			snprintf(pkgdb, strlen(o_root) + strlen(PKGDB) + 1, "%s%s", o_root, PKGDB);
		} else {
			snprintf(pkgdb, strlen(PKGDB) + 1, "%s", PKGDB);
		}

#ifdef DEBUG
		printf("pkginfo: using package database '%s'\n", pkgdb);
#endif

		// read the package database into memory
		packagedb = read_packagedb(pkgdb);

		if (o_installed_mode == 1) {
			// installed mode - list all installed packages
			for (c = 0; c < packagedb->numpackages; c++) {
				printf("%s %s-%d\n", packagedb->packages[c]->name, packagedb->packages[c]->version, packagedb->packages[c]->release);
			}
		} else if (o_list_mode == 1) {
			// list mode - list files owned by the specified package
			if ((i = package_in_packagedb(o_arg, packagedb)) == -1) {
				printf("pkginfo: %s is neither an installed package nor a package file\n", o_arg);
			} else {
				list_files_in_package(packagedb->packages[i]);
			}
			free(o_arg);
		} else {
			// owner mode - list owners matching specified file pattern
			list_file_owners(packagedb, o_arg);
			free(o_arg);
		}

		// cleanup packagedb mem
		free_packagedb(packagedb);
	}

	// cleanup alternate root if needed
	if (o_root)
		free(o_root);

	return(0);
}

int pkginfo_help() {
	printf("usage: pkginfo [options]\n"
		"options:\n"
		"  -i, --installed             list installed packages\n"
		"  -l, --list <package|file>   list files in <package> or <file>\n"
		"  -o, --owner <pattern>       list owner(s) of file(s) matching <pattern>\n"
		"  -f, --footprint <file>      print footprint for <file>\n"
		"  -r, --root <path>           specify alternative installation root\n"
		"  -v, --version               print version and exit\n"
		"  -h, --help                  print help and exit\n");
	return(0);
}
