LAG - LiDAR Analysis GUI
========================

This file contains the following sections:

GENERAL NOTES
INSTALLATION
LICENSE

GENERAL NOTES
=============
LAG is a software for visualisation, analysis and processing of LiDAR data. It has
been initially created at Airborne Reasearch and Survey faciliy in Plymouth Marine
Laboratory to help with data processing.

For more information see: [website?]
For user's guide see: http://arsf-dan.nerc.ac.uk/trac/wiki/Processing/laguserguide
If you are a developer see: http://arsf-dan.nerc.ac.uk/trac/wiki/Processing/lagdevelopersfaq

INSTALLATION
============

Required Dependencies
---------------------

To be able to build LAG you will need a compiled version of lidarquadtree and
you may need to modify Makefile.am to point to its location.

The following libraries are required to make LAG compile.
   * boost
   * GTKmm
   * GTKGLextmm
   * GThread
   * libgeotiff
   * laslib
   * lidarquadtree

Compiling on GNU/Linux x86
--------------------------

This codebase uses the GNU packaging tools.  To get things to a sane state
after checkout, do:

libtoolize && autoheader && aclocal && automake --add-missing && autoconf

Depending on where you've installed laslib, you may need to do the following 
to allow configure to find it:

export PKG_CONFIG_PATH=/usr/local/lib64/pkgconfig/

Then you can do the normal:

./configure
make
make install

Cross compiling for win32 with mingw32
--------------------------------------

If necessary, after checkout (if there is no file named configure)

libtoolize && autoheader && aclocal && automake --add-missing && autoconf

(Ensure that PKG_CONFIG_PATH is set correctly to reflect the needs of the
cross compiler at this point)

And then a standard compiling:
./configure --host=i686-pc-mingw32 --prefix=<where you want to stage an install>

Now edit the Makefile generated to ensure that, during linking, the arguments
passed into mingw32 are ordered such that $(LIBS), $(lag_LDFLAGS), and
$(LDFLAGS) appear after $(lag_OBJECTS) and $(lag_LDADD).

This step will become redundant in future if autotools is configured to do this
automatically, or mingw32 receives a bugfix.

   i.e.
   $(lag_LINK) $(lag_OBJECTS) $(lag_LDADD) $(LIBS)

   is changed to something like..
   
	#$(lag_LINK) $(lag_OBJECTS) $(lag_LDADD) $(LIBS)
	$(CXXLD) $(AM_CXXFLAGS) $(CXXFLAGS) $(lag_OBJECTS) $(lag_LDADD) $(LIBS) $(lag_LDFLAGS) $(LDFLAGS) -o $@

Now make as normal:
make

Installing files to a staging directory can make it easier to see which files
and dlls are necessary for lag to run, aswell as the relative positioning of
those files in earlier, less stable, versions of lag:
make install

Compiling on win32
------------------

Hopefully in the future. :-)

LICENSE
=======

LAG is licensed under the GNU General Public License, version 2. 
Read the file COPYING in the source distribution for details.
