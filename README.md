compress
========

The "ultimate" file compressor, even capable of infinite compression
(0-length files).

How to use
==========

This program compresses and uncompresses files.  Its compression ratio
is very near to no compression (a fraction of a bit), but it ALWAYS
compresses, so you can ALWAYS get 0-length compressed files if you
compress a sufficient number of times.

Beware: this program will modify your file in-place!  Make a backup!

Experiment with very small files first (1 or 2 bytes).
Use repeatedly (billions of times or even more).
Usage: compress [-v] -c FILENAME       compress once
       compress      -C 5 FILENAME     compress 5 times
       compress      -D 5 FILENAME     decompress 5 times
       compress      -d FILENAME       decompress once
Options:
 -v  debug mode.


Licence
=======
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
