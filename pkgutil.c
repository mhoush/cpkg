/*
	pkgutil.c
*/

#include <grp.h>
#include <libgen.h>
#include <limits.h>
#include <pwd.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <archive.h>
#include <archive_entry.h>

#include "pkgutil.h"

struct package *create_package(char *name, char *version, int release, char **files, int numfiles) {

	int c;

	struct package *pkg = malloc(sizeof(struct package));

	pkg->name = strdup(name);
	pkg->version = strdup(version);
	pkg->release = release;
	pkg->numfiles = numfiles;

	pkg->files = (char **)calloc(numfiles, sizeof(char *));
	for (c = 0; c < numfiles; c++) {
		pkg->files[c] = strdup(files[c]);
	}

	return(pkg);
}


struct package *create_package_from_archive(char *filename) {

	int c;

	struct package *pkg = malloc(sizeof(struct package));
	int numfiles = 0;
	char **pkgfiles;
	int arrsize = ARRSIZE;
	pkgfiles = calloc(arrsize, sizeof(char *));

	struct archive *a;
	struct archive_entry *entry;
	int r;

	a = archive_read_new();
	archive_read_support_filter_all(a);
	archive_read_support_format_all(a);
	r = archive_read_open_filename(a, filename, 10240);
	if (r != ARCHIVE_OK) {
		exit(1);
	}
	while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
		if (numfiles == arrsize) {
			arrsize *= 2;
			pkgfiles = realloc(pkgfiles, arrsize * sizeof(char *));
		}
		pkgfiles[numfiles] = strdup(archive_entry_pathname(entry));
		numfiles++;
		archive_read_data_skip(a);
	}
	r = archive_read_free(a);
	if (r != ARCHIVE_OK) {
		exit(1);
	}

	// parse the package name, version, and release from the filename
	char *bname;
	bname = basename(filename);
	//printf("basename: %s\n", bname);
	char *pkgname, *pkgver, *pkgrels;
	int pkgrel;

	int result, nmatch = 4;
	regex_t cregex;
	//const char *regex = "^([A-Za-z0-9_-]+)#(.*)-([0-9]+)\\.pkg.*";
	regmatch_t pmatch[nmatch];

	//printf("regex: %s\n", regex);

	result = regcomp(&cregex, PKGREGEX, REG_EXTENDED);
	if (result != 0) {
		printf("error compiling regular expression '%s', aborting\n", PKGREGEX);
		exit(1);
	}

	result = regexec(&cregex, bname, nmatch, pmatch, 0);

	pkgname = malloc(sizeof(char *));
	pkgname = strncpy(pkgname, bname, pmatch[1].rm_eo - pmatch[1].rm_so);
	pkgname[pmatch[1].rm_eo - pmatch[1].rm_so] = '\0';

	pkgver = malloc(sizeof(char *));
	pkgver = strncpy(pkgver, bname + pmatch[2].rm_so, pmatch[2].rm_eo - pmatch[2].rm_so);
	pkgver[pmatch[2].rm_eo - pmatch[2].rm_so] = '\0';

	pkgrels = malloc(sizeof(char *));
	pkgrels = strncpy(pkgrels, bname + pmatch[3].rm_so, pmatch[3].rm_eo - pmatch[3].rm_so);
	pkgrels[pmatch[3].rm_eo - pmatch[3].rm_so] = '\0';

	pkgrel = atoi(pkgrels);

	pkg = create_package(pkgname, pkgver, pkgrel, pkgfiles, numfiles);

	// clean up
	free(pkgname);
	free(pkgver);
	free(pkgrels);

	regfree(&cregex);

	for (c = 0; c < numfiles; c++) {
		free(pkgfiles[c]);
	}
	free(pkgfiles);

	return(pkg);
}


void free_package(struct package *pkg) {

	int c;

	free(pkg->name);
	free(pkg->version);
	for (c = 0; c < pkg->numfiles; c++) {
		free(pkg->files[c]);
	}
	free(pkg->files);
	free(pkg);
}


struct packagedb *read_packagedb(char *pkgdb) {

	FILE *fp;

	fp = fopen(pkgdb, "r");
	if (fp == NULL) {
		printf("Failed to open the package database!\n");
		exit(EXIT_FAILURE);
	}

	// getline(3) setup
	char *line = NULL;
	size_t len = 0;
	ssize_t read;

	// state engine for package database read
	enum state { PKGNAME, PKGVER, PKGFILES };
	enum state pkgstate = PKGNAME;

	char *s;
	char *pkgname, *pkgver;
	int pkgrel;
	int x;

	int numfiles = 0;
	char **pkgfiles;

	int arrsize = ARRSIZE;

	struct package *pkg;
	pkgfiles = calloc(arrsize, sizeof(char *));

	struct package **packages;
	int dbsize = ARRSIZE;
	int numpackages = 0;
	packages = calloc(dbsize, sizeof(struct package *));

	// read the package database into memory
	while ((read = getline(&line, &len, fp)) != -1) {

		// trim the newline character
		line[strlen(line)-1] = '\0';

		switch (pkgstate) {
			case PKGNAME:
				pkgname = strdup(line);
				pkgstate = PKGVER; // change to next state
				break;
			case PKGVER:
				pkgver = strdup(line);

				// split version and release
				s = strrchr(pkgver, '-');
				x = (int)(s - pkgver);
				memmove(s, s + 1, strlen(s));
				pkgrel = atoi(s);
				pkgver[x] = '\0';
				pkgstate = PKGFILES; // change to next state
				break;
			case PKGFILES:
				if (strlen(line) == 0) { // end of package record
					pkg = create_package(pkgname, pkgver, pkgrel, pkgfiles, numfiles);

					packages[numpackages] = pkg;

					numpackages++;
					if (numpackages == dbsize) {
						dbsize *= 2;
						packages = realloc(packages, dbsize * sizeof(struct package *));
					}

					free(pkgname);
					free(pkgver);
					for (x = 0; x < numfiles; x++) {
						free(pkgfiles[x]);
					}
					free(pkgfiles);

					// reset arrsize and numfiles
					arrsize = ARRSIZE;
					numfiles = 0;
					pkgfiles = calloc(arrsize, sizeof(char *));

					// reset state
					pkgstate = PKGNAME;
				} else { // a file owned by this package
					pkgfiles[numfiles] = strdup(line);
					numfiles++;
					if (numfiles == arrsize) { // dynamic array is full, resize it
						arrsize *= 2;
						pkgfiles = realloc(pkgfiles, arrsize * sizeof(char *));
						//printf("realloc: arrsize = %d\n", arrsize);
					}
				}
				break;
		}
	}

#ifdef DEBUG
	printf("Found %d packages in the package database.\n", numpackages);
#endif

	fclose(fp);
	free(line);
	free(pkgfiles);

	// marshal the package list and number of packages into a packagedb struct
	struct packagedb *packagedb = malloc(sizeof(struct packagedb));
	packagedb->packages = packages;
	packagedb->numpackages = numpackages;

	return packagedb;
}


void free_packagedb(struct packagedb *packagedb) {

	int c;

	for (c = 0; c < packagedb->numpackages; c++) {
		free_package(packagedb->packages[c]);
	}

	free(packagedb->packages);

	free(packagedb);
}


int package_in_packagedb(char *pkgname, struct packagedb *packagedb) {

	int c;

	for (c = 0; c < packagedb->numpackages; c++) {
		if (strcmp(pkgname, packagedb->packages[c]->name) == 0) {
			return(c);
		}
	}

	return(-1);
}


void list_files_in_package(struct package *pkg) {

	int c;

	for (c = 0; c < pkg->numfiles; c++) {
		printf("%s\n", pkg->files[c]);
	}
}


void list_file_owners(struct packagedb *packagedb, char *regex) {

	int c, tnc;
	int width = 7; // width of the package name column ("Package")

	// regex setup stuff
	int result;
	regex_t cregex;
	char lsname[PATH_MAX];

	// dynamic array stuff for the matches
	int arrsize = 32; // start small; often this won't need to expand much
	int matches = 0;
	char **owners, **files;
	owners = calloc(arrsize, sizeof(char *));
	files = calloc(arrsize, sizeof(char *));

	result = regcomp(&cregex, regex, REG_EXTENDED | REG_NOSUB);

	// loop through files in the package database and check them against the regex
	for (c = 0; c < packagedb->numpackages; c++) {
		for (tnc = 0; tnc < packagedb->packages[c]->numfiles; tnc++) {
			// add a leading '/' to the filename for the regex check
			snprintf(lsname, strlen(packagedb->packages[c]->files[tnc]) + 2, "/%s", packagedb->packages[c]->files[tnc]);
			result = regexec(&cregex, lsname, 0, 0, 0);
			if (!result) {
				if (matches == arrsize) {
					arrsize *= 2;
					owners = realloc(owners, arrsize * sizeof(char *));
					files = realloc(files, arrsize * sizeof(char *));
				}
				owners[matches] = strdup(packagedb->packages[c]->name);
				files[matches] = strdup(packagedb->packages[c]->files[tnc]);
				matches++;
				// adjust package column width if needed
				if (strlen(packagedb->packages[c]->name) > width) {
					width = strlen(packagedb->packages[c]->name);
				}
			}
		}
	}

	if (matches > 0) {
		printf("%-*s  %s\n", width, "Package", "File");
		for (c = 0; c < matches; c++) {
			printf("%-*s  %s\n", width, owners[c], files[c]);
		}
	}

	for (c = 0; c < matches; c++) {
		free(owners[c]);
		free(files[c]);
	}
	free(owners);
	free(files);

	regfree(&cregex);
}


void make_footprint(char *filename) {

	int c;

	struct fileinfo {
		mode_t mode;
		uid_t uid;
		gid_t gid;
		char *filename;
		int is_hardlink;
		char *target; // symlink target
		int is_empty;
		long unsigned int major, minor;
	};

	// dynamic array of fileinfo structs for archive metadata
	struct fileinfo **files;
	int arrsize = 32, numfiles = 0;
	files = calloc(arrsize, sizeof(struct fileinfo *));

	// libarchive setup
	struct archive *a;
	struct archive_entry *entry;
	int r;

	a = archive_read_new();
	archive_read_support_filter_all(a);
	archive_read_support_format_all(a);

	r = archive_read_open_filename(a, filename, 10240);
	if (r != ARCHIVE_OK) {
		printf("Failed to open archive '%s'.\n", filename);
		exit(1);
	}

	while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {

		// create a fileinfo entry
		struct fileinfo *fi = malloc(sizeof(struct fileinfo));

		// is the current entry a hardlink?
		if (archive_entry_hardlink(entry)) {
			// find the mode of the hardlink's target and use it instead
			for (c = 0; c < numfiles; c++) {
				if (strcmp(files[c]->filename, archive_entry_hardlink(entry)) == 0) {
					//printf("found hardlink target match: %s -> %s\n", archive_entry_pathname(entry), files[c]->filename);
					fi->mode = files[c]->mode;
				}
			}
			fi->is_hardlink = 1;
		} else {
			fi->mode = archive_entry_mode(entry);
			fi->is_hardlink = 0;

			// is the current entry an empty file?
			if (archive_entry_size(entry) == 0)
				fi->is_empty = 1;
			else
				fi->is_empty = 0;

		}

		fi->uid = archive_entry_uid(entry);
		fi->gid = archive_entry_gid(entry);
		fi->filename = strdup(archive_entry_pathname(entry));

		// is the current entry a symlink?
		if (archive_entry_symlink(entry)) {
			// put its target into the 'target' field
			fi->target = strdup(archive_entry_symlink(entry));
		} else {
			fi->target = NULL;
		}

		// major/minor values for devices (0 otherwise)
		fi->major = archive_entry_rdevmajor(entry);
		fi->minor = archive_entry_rdevminor(entry);

		// put it into the files array
		if (arrsize == numfiles) {
			arrsize *= 2;
			files = realloc(files, arrsize * sizeof(struct fileinfo *));
		}
		files[numfiles] = fi;

		numfiles++;

		archive_read_data_skip(a);
	}

	r = archive_read_free(a);
	if (r != ARCHIVE_OK) {
		printf("Failed to free archive!\n");
		exit(1);
	}

	for (c = 0; c < numfiles; c++) {
		char *modestr = mtos(files[c]->mode);

		struct passwd *pw = getpwuid(files[c]->uid);
		struct group *gr = getgrgid(files[c]->gid);

		printf("%s\t%s/%s\t%s", modestr, pw->pw_name, gr->gr_name, files[c]->filename);

		if (S_ISLNK(files[c]->mode)) {
			printf(" -> %s", files[c]->target);
		} else if (S_ISCHR(files[c]->mode) || S_ISBLK(files[c]->mode)) {
			printf(" (%lu, %lu)", files[c]->major, files[c]->minor);
		} else if (S_ISREG(files[c]->mode) && files[c]->is_empty == 1) {
			printf(" (EMPTY)");
		}

		printf("\n");

		free(modestr);
	}

	for (c = 0; c < numfiles; c++) {
		if (files[c]->target)
			free(files[c]->target);
		free(files[c]->filename);
		free(files[c]);
	}
	free(files);
}


char *mtos(mode_t mode) {

	char out[11] = "----------";
	char *ret;

	// file type
	switch (mode & S_IFMT) {
		case S_IFREG:  out[0] = '-'; break; // regular file
		case S_IFDIR:  out[0] = 'd'; break; // directory
		case S_IFLNK:  out[0] = 'l'; break; // symbolic link
		case S_IFCHR:  out[0] = 'c'; break; // character special
		case S_IFBLK:  out[0] = 'b'; break; // block special
		case S_IFSOCK: out[0] = 's'; break; // socket
		case S_IFIFO:  out[0] = 'p'; break; // FIFO
		default:       out[0] = '?'; break; // unknown
	}

	// user permissions
	out[1] = (mode & S_IRUSR) ? 'r' : '-';
	out[2] = (mode & S_IWUSR) ? 'w' : '-';
	switch (mode & (S_IXUSR | S_ISUID)) {
		case S_IXUSR:           out[3] = 'x'; break;
		case S_ISUID:           out[3] = 'S'; break;
		case S_IXUSR | S_ISUID: out[3] = 's'; break;
		default:                out[3] = '-'; break;
	}

	// group permissions
	out[4] = (mode & S_IRGRP) ? 'r' : '-';
	out[5] = (mode & S_IWGRP) ? 'w' : '-';
	switch (mode & (S_IXGRP | S_ISGID)) {
		case S_IXGRP:           out[6] = 'x'; break;
		case S_ISGID:           out[6] = 'S'; break;
		case S_IXGRP | S_ISGID: out[6] = 's'; break;
		default:                out[6] = '-'; break;
	}

	// other permissions
	out[7] = (mode & S_IROTH) ? 'r' : '-';
	out[8] = (mode & S_IWOTH) ? 'w' : '-';
	switch (mode & (S_IXOTH | S_ISVTX)) {
		case S_IXOTH:           out[9] = 'x'; break;
		case S_ISVTX:           out[9] = 'T'; break;
		case S_IXOTH | S_ISVTX: out[9] = 't'; break;
		default:                out[9] = '-'; break;
	}

	ret = strdup(out);

	return(ret);
}


void print_version(char *utilname) {
	printf("%s (%s) %s\n", utilname, "pkgutils", VERSION);
}
