#!/bin/sh

cat $* | awk 'BEGIN { \
    printf "/*\n * This file is generated from %s.  Do not edit.\n */\n", \
	   "$(INCLUDESRC)/keysymdef.h";\
} \
/^#define/ { \
	len = length($2)-3; \
	printf("{ \"%s\", %s },\n", substr($2,4,len), $3); \
}' 

