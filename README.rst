
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

cd to the bin directory.

.. code:: bash

 ./lyrebird configutils/test_config.json

At this point trippy patterns should show up on screen.  Enjoy!



Running With Real Data
======================
There are actually three processes that you need to spawn.  A server, kookaburra.py and lyrebird

misc server <- kookaburra.py <- lyrebird


Server Process
--------------

Lyrebird needs to get the data from somewhere.  The computer running lyrebird does *not* need to be the computer collecting the data.

On your control computer you need to have an istance of spt3g_software/examples/data_relay.py running.  This is the server for the data that lyrebird needs.

.. code:: bash

 spt3g_software/examples/data_relay.py ${PATH_TO_PYDFMUX_HWM_YAML_FILE}


kookaburra.py
-------------

kookaburra.py does 3 things.  

- Displays the state of the SQUIDs
- Generates the lyrebird config file
- Relays the housekeeping frames to lyrebird

To run kookaburra.py in the lyrebird/bin directory:

.. code:: bash

./kookaburra.py ${SERVER_RUNNING_DATA_RELAY_HOSTNAME}


lyrebird
--------
At this point just run lyrebird.  In the bin directory:

.. code:: bash

./lyrebird
