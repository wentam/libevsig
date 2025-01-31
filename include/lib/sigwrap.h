#pragma once
#include <stdio.h>

// TODO fmemopen
// TODO fopencookie
// TODO sprintf
// TODO snprintf

void*  sw_malloc (size_t size);
void*  sw_calloc (size_t nmemb, size_t size);
void*  sw_realloc(void* ptr, size_t size);
size_t sw_fread  (void* ptr, size_t size, size_t nmemb, FILE* stream);
size_t sw_fwrite (void* ptr, size_t size, size_t nmemb, FILE* stream);
FILE*  sw_fopen  (const char* pathname, const char* mode);
int    sw_fclose (FILE* stream);
int    sw_fflush (FILE* stream);
int    sw_fseek  (FILE* stream, long offset, int whence);
int    sw_printf (const char* format, ...);
int    sw_fprintf(FILE* stream, const char* format, ...);
