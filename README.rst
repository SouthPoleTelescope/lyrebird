
Installation
============

Environment Variables needed to be set
---------------------------------------


.. code:: bash

 export SPT3G_SOFTWARE_PATH=[path to spt3g_software]
 export SPT3G_SOFTWARE_BUILD_PATH=[path to where you built spt3g software]


Program dependencies
---------------------

- cmake
- c++ compiler
- netcat (nc)

Library dependencies
---------------------

- libGL
- libGLU
- libGLEW
- libfftw3
- libpthread
- libXi 
- libXxf86vm
- libXrandr
- libX11
- libXinerama 
- libXcursor
- libglfw3

I know that's a lot so just type:

.. code:: bash

 sudo apt-get install xorg-dev
 sudo apt-get install libglu1-mesa-dev 
 sudo apt-get install libglew-dev
 sudo apt-get install libfftw3-dev
 sudo apt-get install libglfw3-dev

The code also depends on *spt3g_software*.  You will need an up-to-date version of that library.


Building
--------

Once that is out of the way in the lyrebird directory type

.. code:: bash

 mkdir build
 cd build
 cmake ..
 make


If you would like to build with a different compiler like clang use:

.. code:: bash

 cmake .. -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++

Instead of:

.. code:: bash

 cmake ..


Testing
-------

