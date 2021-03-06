// this file is part of osmbrowser
// copyright Martijn Versteegh
// osmbrowser is licenced under the gpl v3
#ifndef __PARSE_H__
#define __PARSE_H__

#include "osm.h"
#include <stdio.h>

OsmData *parse_osm(FILE *file, bool skipAttribs = false);

OsmData *parse_binary(FILE *file, bool skipAttribs = false);

void write_binary(OsmData *d, FILE *f);

#endif
