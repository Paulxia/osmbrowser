#include <stdlib.h>
#include <stdio.h>
#include "osm.h"
#include "parse.h"

int main(int argc, char **argv)
{
    OSMDATA *data;
    FILE *infile;
    int i, j;

    if (argc < 2)
    {
        printf("usage %s < osmfile>\n", argv[0]);
        exit(1);
    }

    infile = fopen(argv[1], "r");

    if (!infile)
    {
        printf("could not open file '%s'\n", argv[1]);
        abort();
    }

    data = parse_osm(infile);

    fclose(infile);

    for (i = 0; i < data->numWays; i++)
    {
        for (j = 0; j < data->ways[i].numResolvedNodes; j++)
        {
            if (data->ways[i].resolvedNodes[j]) // some nodes cannot be resolved, probably because the nodes are mentioned in a way/relation, but are outside the area I downloaded. I simply skip them
            {
                double lat = data->ways[i].resolvedNodes[j]->lat;
                double lon = data->ways[i].resolvedNodes[j]->lon;
                printf("( %f, %f) ", lat, lon);
            }
        }
        printf("\n");
    }


    osmdata_destroy(data);

    return 0;
}

