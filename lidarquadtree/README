lidarquadtree library
=====================

This file contains the following sections:

GENERAL NOTES
INSTALLATION
LICENSE

GENERAL NOTES
=============

Lidarquadtree is a library initially created for LiDAR Analysis GUI (LAG). 
It provides an efficient data structure for storing and indexing points.
For more information see: [website?]

INSTALLATION
============

Dependencies
------------

A few libraries are required to make this library compile.
   * liblzo2
   * laslib

Compiling on GNU/Linux x86
--------------------------

This codebase uses the GNU packaging tools.  To get things to a sane state
after checkout, do:

libtoolize && autoheader && aclocal && automake --add-missing && autoconf

Then you can do the normal:

./configure
make
make install

Use make clean if you want a fresh rebuild. The resulting .so library will be put in .libs
and installed in (prefix)/lib/lidar/quadtree.

Cross compiling for win32 with mingw32
--------------------------------------

If necessary (if there is no file named configure):
libtoolize && autoheader && aclocal && automake --add-missing && autoconf

Ensure that PKG_CONFIG_PATH is set to reflect the needs of the cross compiler

Run:
./configure --host=i686-pc-mingw32 --prefix=<where you want to install to>
make

And before running make install, check the file lidarquadtree.pc for correct
settings, typically it may point to a local linux installation of lidarquadtree,
ignoring its own propper configuration. This may be fixed in future, but
otherwise; set prefix, exec_prefix, libdir, and includedir to reflect the
locations of your soon-to-be windows installation of lidarquadtree

Then finally run:
make install

Compiling on win32
------------------

Hopefully in the future. :-)

Documentation
-------------

To make the docs, type:

doxygen doxygen_settings

Which will create html documentation in doc/html.

LICENSE
=======

See COPYING for the GNU GENERAL PUBLIC LICENSE.
