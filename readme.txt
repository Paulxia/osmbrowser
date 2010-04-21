files:
main.c     - test main leest een file in en print alle ways in een simpel formaat
osm.h/.c   - datastructuren en hulpfuncties om OSM data (nodes, ways, relations en tags) in het geheugen te houden.
parse.h/.c - gebruikt expat om de xml data te parsen.

wxmain.cpp  - andere test main. gebruikt wx om de kaart te tekenen

wxcanvas.cpp   - hulpclasses voor het tekenen
osmcanvas.cpp

-----------------------
compile met:
    make fixbuild
    make depend
    make

of de wx versie met:
    make -f makefile.wx fixbuild
    make -f makefile.wx depend
    make -f makefile.wx
