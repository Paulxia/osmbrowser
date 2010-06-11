compiling  (only tested on ubuntu 9.10 /10.04, though other distro's or cygwin or OSX should work as well )
------------------------
dependencies:
         wxwidgets (version > 2.8)
         cairo	(with pdf support)
         expat (in non-widechar mode)
If you have all the dependencies installed, just running make should do the trick. The executable will be called osmbrowse


running
------------------------

./osmbrowser <mapfile.osm>
this will create a mapfile.osm.cache for faster loading the next time. You can safely delete that if you're not interested in faster loading,
when you specify a - as filename, osmbrowser wil read from stdin (only osm format atm, no cache files). This is useful if you have a alrge osm
file and don't want to unzip it.
for example
bzcat netherlands.osm.bz2 | ./osmbrowser -
this will create a file stdin.cache, which you camn use to open faster the next time
./osmbrowser stdin.cache


interface explanation
------------------------
In the center is the main map display. Dragging the mouse pans the map, the scrollwheel zooms.
The light read square is the currenly selected osm node. It's info is displayed on the right. If you click on the selected nod the selection is
'locked' and won't follow your mouse, you can then click on the info on the right to select a way on the map. Clicking on the map again unlocks the selection.


On the left is the filter display. In the first box you can type a name for your ruleset. If you type a name, the set will be remembered for the next run of the app. The second box is for the draw rule, which determines what is going to be drawn. Below that you can add color rules which
determine the color of what is drawn. Each way is first matched against the drawrule. If this matches then it is matched to the color rules from top to bottom
and the first match is used to determine how to draw this way. If no colorrule matches it is drawn in light gray.

rules
------------------------
A rule is a logical expression written down as a lisp s-expression.

RULE =
        (tag "key")                        // true if the object matched has a tag with this key
        | (tag "key" "value")              // true if the object matched has this exact tag
        | (or SUBRULES)                    // true if any of the SUBRULES is true
        | (and SUBRULES)                   // true if all SUBRULES are true
        | (not RULE)                       // true if RULE is false and vv

SUBRULE = RULE ...                         // one or more rules

examples:

match all naturals, waterways and boundaries

(or
  (tag "natural")
  (tag "boundary")
  (tag "waterway")
)

match everything but buildings and landuse

(not
    (or
        (tag "building")
        (tag "landuse")
    )
)


match only forests and parks

(or
    (tag "landuse" "forest")
    (tag "landuse" "woodland")
    (tag "leisure" "park")
)

