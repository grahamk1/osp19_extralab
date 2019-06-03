#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>
#include <pthread.h> //To use for threads! Yay! I am very tired

//my_file_search.c
//Based off of Dingler's given file

//Takes a file/directory as an argument, recurses, 
//prints name if empty directory or not a directory
void recur_file_search(char *file);

//share search term globally rather than passing recursively
const char *search_term;

int main(int argc, char **argv)
{
	if(argc !=3)
	{
		printf("Usage: my_file_search <search_term> <dir>\n");
		printf("Performs recursive file search for files/directories\
				matching <search_term> starting at <dir>\n");
		exit(1);
	}

	//share four threads and declare array
	const int THREADS = 4;
	pthread_t threads[THREADS];

	//Count number of threads working
	int threadCount = 0;

	//Don't need to bother checkin if term/directory are swapped
	//Since we don't know for sure which is which anyway
	search_term = argv[1];

	//Open top-level directory
	DIR *dir = opendir(argv[2]);

	//Determine if top level directory is openable
	if(dir == NULL)
	{
		perror("opendir failed");
		exit(1);
	}

	//Declare pointer for current file
	struct dirent *curFile;

	//Start timer for recursive search
	struct timeval start, end;
	gettimeofday(&start, NULL);

	//Loop through directory and assign threads
	while((curFile = readdir(dir)) != NULL)
	{
		//Recursing on . or .. is no bueno lets not
		if(strcmp(curFile->d_name, "..") != 0 && strcmp(curFile->d_name, ".") !=0)
		{
			//Format the filenames to be a full filepath
			char *nextFile = malloc(sizeof(char)*strlen(curFile->d_name)+strlen(argv[2])+2);
			strncpy(nextFile, argv[2], strlen(argv[2]));
			strncpy(nextFile+strlen(argv[2]), "/", 1);
			strncat(nextFile, curFile->d_name, strlen(curFile->d_name));
			//This is the end *insert eerie music*...again I am very tired
			strncat(nextFile, "\0", 1);

			if (threadCount == THREADS)
			{
				//Let threads finish their thing before they get reassigned
				for (int i=0; i < THREADS; i++)
				{
					pthread_join(threads[i], NULL);
				}

				//Now we reset the thread count
				threadCount = 0;
			}

			//Create the thread, run the recursive file search using the next file
			pthread_create(&threads[threadCount], NULL, (void*)&recur_file_search, (void*)nextFile);
			//Increment threadCount since one is running now
			threadCount++;
		}
	}

	closedir(dir);

	//Print runtime
	gettimeofday(&end, NULL);
	printf("Time: %ld\n", (end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec));

	return 0;
}

//This function takes a path to recurse on, searching for matches to the
//global search_term. The base for recursion is when *file is not a directory.
//Parameters: the starting path for recursion (char*), which could be a directory
//or a regular file 
//Returns: nothing
//Effects: prints the filename if the base case is reached and search_term is
//found in the filename, otherwise prints the directory name if the directory
//matches the search_term.
void recur_file_search(char *file)
{
	//check if directory is actually a file
	DIR *d = opendir(file);

	//NULL means not a directory (or another unlikely error)
	if(d == NULL)
	{
		//opendir SHOULD error with ENOTDIR, but if it did something else,
		//we have a problem
		if(errno != ENOTDIR)
		{
			perror("Something weird happened!");
			fprintf(stderr, "While looking at %s\n", file);
			exit(1);
		}

		//nothing weird happened, check if the file contains the search term 
		//and if so, print the file to the screen w/ full path
		if(strstr(file, search_term) != NULL)
			printf("%s\n", file);

		//no need to close d since its NULL
		return;
	}

	//we have a directory, not a file, so check if it matches the search term
	if(strstr(file, search_term) != NULL)
		printf("%s/\n", file);

	//call recur_file_search for each file in d
	//readdir "discovers" all the files in d one by one and we recurse on those
	//until we run out and readdir returns NULL
	struct dirent *cur_file;
	while((cur_file = readdir(d)) != NULL)
	{
		//make sure we don't recurse on . or ..
		if(strcmp(cur_file->d_name, "..") != 0 &&\
				strcmp(cur_file->d_name, ".") != 0)
		{
			//we need to pass a full path to the recursive function
			//so here we append the discovered filename
			//to the current path
			char *next_file_str = malloc(sizeof(char) * strlen(cur_file->d_name) + strlen(file) +2);
			strncpy(next_file_str, file, strlen(file));
			strncpy(next_file_str + strlen(file), "/", 1);
			strncpy(next_file_str + strlen(file) + 1, cur_file->d_name, strlen(cur_file->d_name) +1);

			//recurse on the file
			recur_file_search(next_file_str);

			//free the dynamically-allocated string
			free(next_file_str);
		}
	}

	//close the directory, or we will have too many files opened (bad times)
	closedir(d);
}

