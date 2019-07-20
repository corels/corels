#pragma once

#ifdef _WIN32

// This code was graciously provided for free by this StackOverflow answer: https://stackoverflow.com/a/26085827

typedef struct timeval {
    long tv_sec;
    long tv_usec;
} timeval;

int gettimeofday(struct timeval * tp, struct timezone * tzp);

#else

#include <sys/time.h>

#endif
