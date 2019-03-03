/*
	main.c
*/

#include <getopt.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pkginfo.h"
#include "pkgutil.h"

int main(int argc, char *argv[]) {

	// name of the called utility
	char *utilname = basename(argv[0]);

	// help flag
	int helpwanted = 0;

	// getopt(3) setup
	int opt = 0, option_index = 1;
	extern int optind, opterr, optopt;
	opterr = 0;
	optind = 1;

	static struct option long_options[] = {
		{ "help",    no_argument, NULL, 'h' },
		{ "version", no_argument, NULL, 'v' },
		{ 0,         0,           0,    0   }
	};

	while ((opt = getopt_long(argc, argv, "+:hv", long_options, &option_index)) != -1) {
		switch (opt) {
			case 'h':
				// help
				helpwanted = 1;
				break;
			case 'v':
				// version
				print_version(utilname);
				exit(0);
				break;
		}
	}

	/*
		change behavior based on called utility name (pkginfo, pkgadd, pkgrm)
	*/
	int (*runfunc)();
	runfunc = NULL;

	// only 'pkginfo' implemented so far
	if (strcmp(utilname, "pkginfo") == 0) {
		runfunc = helpwanted ? pkginfo_help : pkginfo_run;
	} else {
		printf("no util specified; cpkg mode.\n");
	}

	if (runfunc != NULL) {
		runfunc(argc, argv);
	}

	return(0);
}
