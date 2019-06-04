CC = gcc
FLAGS = -o -Wall

all: my_file_search

my_file_search: my_file_search.c
	$(CC) $(FLAGS) my_file_search my_file_search.c -lpthread

clean:
	rm my_file_search
