/*
	pkgutil.h
*/

#ifndef _PKGUTIL_H_
#define _PKGUTIL_H_

// package database location
#define PKGDB "/var/lib/pkg/db"

// starting size for dynamic arrays
#define ARRSIZE 256

// basic package file regex
#define PKGREGEX "^([A-Za-z0-9_][A-Za-z0-9_-]*)#(.+)-([0-9]+)\\.pkg\\.tar\\.[gxb]z2?"

// package information structure
struct package {
	char *name, *version; // package name and version
	int release;          // package release/revision number
	char **files;         // list of files owned by the package (dynamic array)
	int numfiles;         // number of files owned by the package
};

// package database structure, holds a dynamic array of struct package pointers
struct packagedb {
	struct package **packages;
	int numpackages;
};

/*
	create_package: returns a struct package pointer from passed package
		information: name, version, release number, file list, number
		of files
*/

struct package *create_package(char *name, char *version, int release, char **files, int numfiles);


/*
	create_package_from_archive: returns a struct package pointer with
		pertinent information gathered from the passed filename
*/

struct package *create_package_from_archive(char *filename);


/*
	free_package: frees memory used by a struct package pointer
*/

void free_package(struct package *pkg);


/*
	read_packagedb: returns a struct packagedb pointer with package
		information read from the on-disk package database
*/

struct packagedb *read_packagedb(char *pkgdb);


/*
	free_packagedb: frees memory used by the package database struct
		packagedb pointer
*/

void free_packagedb(struct packagedb *packagedb);


/*
	package_in_packagedb: returns the numeric index of a package in the
		package database or -1 if not found
*/

int package_in_packagedb(char *pkgname, struct packagedb *packagedb);


/*
	list_files_in_package: prints a list of the files in a struct package
		pointer
*/

void list_files_in_package(struct package *pkg);


/*
	list_file_owners: list owners of any files matching the passed regex,
		if any
*/

void list_file_owners(struct packagedb *packagedb, char *regex);


/*
	make_footprint: print the footprint of the passed package file
*/

void make_footprint(char *filename);


/*
	mtos: returns a string representation of a mode_t
*/

char *mtos(mode_t mode);


/*
	print_version: prints the utility version
*/

void print_version(char *utilname);

#endif
