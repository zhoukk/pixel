#include "readfile.h"

#include <stdio.h>
#include <stdlib.h>

int readable(const char *file) {
	FILE *f;
	f = fopen(file, "r");
	if (!f) {
		return 0;
	}
	fclose(f);
	return 1;
}

char *readfile(const char *file, int *psize) {
	char *data;
	int size;
	FILE *f;
	f = fopen(file, "rb");
	if (!f) {
		return 0;
	}
	fseek(f, 0, SEEK_END);
	size = ftell(f);
	fseek(f, 0, SEEK_SET);
	data = malloc(size);
	size = fread(data, 1, size, f);
	fclose(f);
	if (psize) {
		*psize = size;
	}
	return data;
}