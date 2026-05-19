// Utility Functions

#include <stdio.h>
#include <string.h>
#include "../include/process.h"

// Convert string to uppercase in-place
void str_to_upper(char *s)
{
    for (; *s; s++) {
        if (*s >= 'a' && *s <= 'z')
            *s = (char)(*s - 32);
    }
}