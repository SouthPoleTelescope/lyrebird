
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
Mainly it needs:

- libfftw3
- libpthread
- libglfw3

glfw3 has a bunch of dependencies on *nix.

- libGL
- libGLU
- libGLEW
- libXi 
- libXxf86vm
- libXrandr
- libX11
- libXinerama 
- libXcursor

I know that's a lot so if you are on a debian system just type:

.. code:: bash

 sudo apt-get install xorg-dev
 sudo apt-get install libglu1-mesa-dev 
 sudo apt-get install libglew-dev
 sudo apt-get install libfftw3-dev
 sudo apt-get install libglfw3-dev


If you are on a different system, just make certain that you have the big three installed.

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
- Relays the housekeeping/timepoint frames to lyrebird

To run kookaburra.py in the lyrebird/bin directory:

.. code:: bash

 ./kookaburra.py ${SERVER_RUNNING_DATA_RELAY_HOSTNAME}


lyrebird
--------
At this point just run lyrebird.  In the bin directory:

.. code:: bash

 ./lyrebird lyrebird_config_file.json

- lyrebird gets the housekeeping information from kookaburra so that needs to be running.
- kookaburra is used to write the lyrebird config file.  Because infinite speed computers/networks don't exist yet, kookaburra.py takes a little bit of time to make the config file which it will tell you about.

General Note
------------

Because of latent parnoia about the act of collecting housekeeping injecting noise in the system, housekeeping collection isn't automatic with the data_relay.py script.  There is a button in lyrebird, but if you are just running kookaburra you will need to use a script to request housekeeping data.  A script to do this is generated when you run kookaburra.  It's called get_hk.sh


Optimizing Network Traffic
--------------------------
Networking is complicated.  The G3NetworkSender will send that packets that it can to the receiving computers.  If it has failed to send enough of the previous data it will skip sending some frames.  It sends the frames it can via the TCProtocol.

You can adjust how many Timepoint frames are sent with the setup of the G3NetworkSender.  
The frame_decimation argument specifies this.  In the following command 1 out of every 40
timepoint frames is sent.

.. code:: python
 pipe.Add(core.G3NetworkSender,
          port = 8675,
          maxsize = 10,
          frame_decimation = {core.G3FrameType.Timepoint: 40},
          max_connections = 0 )

If you are dropping frames you will want to increase the frame_decimation number.  Weirdly enough, if the data is sporadically being transmitted, but you are not dropping frames, you may want to lower the frame_decimation number.  Depending on TCP implementation it will bundle the data before sending.  If the data rate is low, that might take a while. 
   

Authors
-------

Lyrebird was developed at UC Berkeley for use with the South Pole
Telescope.  The original author of Lyrebird is Nick Harrington, but he
is no longer involved with development.

Please send questions or comments about Lyrebird to the maintainers of
the repository where you obtained it.

