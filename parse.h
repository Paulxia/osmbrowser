#ifndef __PARSE_H__
#define __PARSE_H__

#include "osm.h"
#include <stdio.h>

#ifdef __cplusplus
 extern "C" {
#endif


OSMDATA *parse_osm(FILE *file);

#ifdef __cplusplus
 }
#endif


#endif
