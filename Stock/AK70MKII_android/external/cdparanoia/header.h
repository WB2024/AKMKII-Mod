/******************************************************************
 * CopyPolicy: GNU Public License 2 applies
 * Copyright (C) 2008 Monty xiphmont@mit.edu
 ******************************************************************/

#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void WriteWav(int f,long bytes);
extern void WriteAifc(int f,long bytes);
extern void WriteAiff(int f,long bytes);

#ifdef __cplusplus
}
#endif
