/*
	pkginfo.h
*/

#ifndef _PKGINFO_H_
#define _PKGINFO_H_

/*
	pkginfo_run: handles 'pkginfo' tasks such as listing packages,
		files owned by packages, files matching pattern, etc.
*/

int pkginfo_run(int argc, char *argv[]);


/*
	pkginfo_help: prints help/usage for pkginfo
*/

int pkginfo_help();

#endif
