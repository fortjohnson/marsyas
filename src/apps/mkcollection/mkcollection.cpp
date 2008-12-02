/*
** Copyright (C) 2000 George Tzanetakis <gtzan@cs.princeton.edu>
**   
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

/* mkcollection:
Create a MARSYAS collection from directories and files 
Daniel German at UVic added recursive reading of subdirectories 
*/

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <assert.h>

#include "CommandLineOptions.h"
#include "FileName.h"
#include "Collection.h"
#include <string>

using namespace std;
using namespace Marsyas;


/* global variables for various commandline options */ 

int helpopt;
int usageopt;
int escapeOpt;
string collectionName;
string labelopt;
CommandLineOptions cmd_options;

void printUsage(string progName)
{
	MRSDIAG("mkcollection.cpp - printUsage");
	cerr << "Usage : " << progName << " [-l label] [-c collectionName] dir1 dir2 ... dirN" << endl;
	cerr << endl;
	cerr << "where dir1, dir2, ..., dirN are directories that will be scanned recursively for sound files in a MARSYAS supported format and added to the collection. " << endl;
	exit(1);
}

void 
printHelp(string progName)
{
	MRSDIAG("mkcollection.cpp - printHelp");
	cerr << "mkcollection, MARSYAS, Copyright George Tzanetakis " << endl;
	cerr << "--------------------------------------------" << endl;
	cerr << "Utility for creating collection files " << endl;
	cerr << endl;
	cerr << "Usage : " << progName << " [-l label] [-c collectionName] dir1 dir2 ... dirN" << endl;
	cerr << endl;
	cerr << "where dir1, dir2, ..., dirN are directories that will be scanned recursively for sound files in a MARSYAS supported format and added to the collection. " << endl;
	cerr << "Help Options:" << endl;
	cerr << "-u --usage      : display short usage info" << endl;
	cerr << "-h --help       : display this information " << endl;
	cerr << "-l --label      : label for the collection " << endl;
	cerr << "-e --escape	 : escape all special characters" << endl;
	cerr << "-c --collectionName : the name of the collection (including the .mf extension) " << endl;
	exit(1);
}

void 
initOptions()
{
	cmd_options.addBoolOption("help", "h", false);
	cmd_options.addBoolOption("usage", "u", false);
	cmd_options.addBoolOption("escape", "e", false);
	cmd_options.addStringOption("label", "l", EMPTYSTRING);
	cmd_options.addStringOption("collectionName", "c", "music.mf");
}


void 
loadOptions()
{
	helpopt = cmd_options.getBoolOption("help");
	usageopt = cmd_options.getBoolOption("usage");
	collectionName = cmd_options.getStringOption("collectionName");
	escapeOpt = cmd_options.getStringOption("escape");
	labelopt = cmd_options.getStringOption("label");
}



#ifndef WIN32
#include <dirent.h>

#define SEPERATOR_CHAR	'/'

#else 
//needed for fileinfo and _findfirst/_findnext functions
#include <io.h>

//simulate some structures and data types that do not exist in WIN32
typedef unsigned short mode_t;
#define lstat _stat
typedef void DIR;
#define stat _stat

//this is needed to use the proper directory
//seperator character for windows/non windows
#define SEPERATOR_CHAR	'\\'

//disable the strcpy deprecated warning
#pragma warning( disable : 4996)

//simulate these macros testing for file or directory attributes
#define S_ISDIR(x) (x & _S_IFDIR)
#define S_ISREG(x) (x & _S_IFREG)

//simulate this data structure for the opendir/readdir functions
struct dirent
{
	char d_name[_MAX_PATH+1];
};

int closedir(DIR* dir) 
{
	// do nothing just a stub // [?]
	return 0; 
}

//Function to simulate the opendir function call.
//returns a handle(typedef'd as a void *) that is used in subsequent
//calls to readdir.
// NULL return means failure to open dir.
DIR *opendir(string dir)
{
	string filespec = dir;
	filespec += "*.*";

	struct _finddata_t fileinfo;
	intptr_t result = _findfirst( filespec.c_str(), &fileinfo);
	if(result==-1)
	{
		return NULL;
	}
	return (void *)result;
}

//Function to simulate the readdir function call.
//Returns a pointer to a static structure where the name of the dir/file
//can be retrieved.
//Takes a handle to the return of opendir call defined above.
//Returns NULL when no more files to read.
struct dirent *readdir(DIR *dirp)
{
	static struct dirent localDirent;

	struct _finddata_t fileinfo;
	int result = _findnext( (intptr_t)dirp, &fileinfo);

	if(result==-1)
	{
		_findclose((intptr_t)dirp);
		return NULL;
	}

	strcpy((char *)&localDirent.d_name,(const char *)&fileinfo.name);
	return &localDirent;
}
#endif

mode_t File_Mode(string fileName) 
{
	/* See man stat for a description of the st_mode attribute and the meanings of mode_t */

	struct stat     statbuf;

	if (lstat(fileName.c_str(), &statbuf) == -1)
		/* There is an error getting info on this file, skip. */

		return 0;

	/* I believe 0 is a good value to return in case of error. There is
	no way all the flags in the mode can be zero, but let us be sure */

	assert(statbuf.st_mode != 0);

	return statbuf.st_mode;
}


int accept(string str)
{
	FileName fname(str);
	string wav("wav");
	string au("au");
	string mp3("mp3");

	if ((fname.ext() == wav)
		||(fname.ext() == au)
		||(fname.ext() == mp3))
		return 1;
	else return 0;

}

void read(Collection& cl, string dir, int recursive)
{
	/* THis variable makes the code non-reentrant, but who cares */
	static int levels = 0;

	levels ++;
	if (levels > 30) {
		/* make it safe */
		cerr << "We are already in a" << levels << "of subdirectories " << dir << "exiting\n";
	}
	// add backslash, only if necessary
	//Added reference to defined constant - dale
	if (dir.size() > 0 &&
		dir[dir.size()-1] != SEPERATOR_CHAR)
		dir += SEPERATOR_CHAR;
	cout << "Adding Contents of Directory = " << dir << "\n to collection " << 
		cl.name() << ".mf" << endl;
	DIR* dirp;
	struct dirent *dp;
	dirp = opendir(dir.c_str());

	//added code to bail if diropen fails - dale
	if (dirp == NULL)
	{
		cout << "Problem with opening directory " << dir << endl;
		return;
	}

	while ((dp = readdir(dirp))) {
		string fullPath;
		mode_t mode;

		fullPath = dir + dp->d_name;

		mode = File_Mode(fullPath.c_str());

		if (mode == 0) /* There is an error reading this entry */
			continue;

		if (S_ISDIR(mode)) {
			/* THis is a directory, process it if recursive processing */

			if (strcmp(".", dp->d_name) == 0 ||
				strcmp("..", dp->d_name) == 0) {
					/*We can't process these directories, just skip */
					continue;
			}

			if (recursive) {
				/* Call this function recursively, it is the easiest way
				to do the traversal */
				read(cl, fullPath, 1);
			}
		} else if (S_ISREG(mode)) {
			if (accept(dp->d_name))
			{
				printf("Adding directory entry %s\n", dp->d_name);
				cl.add(fullPath);
			}
		} else {

		}

	}
	closedir(dirp);
	levels--;
}

int 
main(int argc, const char **argv)
{
	string progName = argv[0];  
	if (argc == 1)
		printUsage(progName);

	// handling of command-line options 
	initOptions();
	cmd_options.readOptions(argc, argv);
	loadOptions();

	cout << "CollectionName = " << collectionName << endl;
	Collection cl;
	FileName fname(collectionName);
	// remove .mf
	collectionName = collectionName.substr(0, collectionName.size()-3); 

	cl.setName(collectionName);

	vector<string> soundfiles = cmd_options.getRemaining();
	if (helpopt) 
		printHelp(progName);

	if (usageopt)
		printUsage(progName);

	vector<string>::iterator sfi;  
	for (sfi = soundfiles.begin(); sfi != soundfiles.end(); ++sfi) 
	{
		mode_t mode;
		string fname = *sfi;
		mode = File_Mode(fname.c_str());
		if (S_ISDIR(mode)) {
			read(cl, fname.c_str(), 1);	
		} 
		else 
		{
			cerr << fname << " is not a directory. Skipping...\n";
		}
	}

	string collectionstr;
	collectionstr += collectionName;
	collectionstr += ".mf";
	if (labelopt != EMPTYSTRING) 
		cl.labelAll(labelopt);

	cl.write(collectionstr);

	cout << "Wrote collection " << collectionstr << endl;
	return 0;
}



