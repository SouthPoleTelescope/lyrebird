CC=clang++ 

current_dir = $(shell pwd)

#CFLAGS=-c -Wall -I../contrib/AntTweakBar/include/ -std=c++11 -I. -O3 -I/usr/local/include/ -I/home/nlharr/spt_code/spt3g_software/hk/ -I/home/nlharr/spt_code/spt3g_software/dfmux/ -I/home/nlharr/spt_code/spt3g_software/core/include/core/ -I/

CFLAGS=-c -Wall -I../contrib/AntTweakBar/include/ -std=c++11 -I. -O3 -I/usr/local/include/ 

#LDFLAGS= -Wl,-rpath=/home/nlharr/spt_code/spt3g_software/build/skunkworks/ /home/nlharr/spt_code/spt3g_software/build/skunkworks/core.so /home/nlharr/spt_code/spt3g_software/build/skunkworks/hk.so /home/nlharr/spt_code/spt3g_software/build/skunkworks/dfmux.so

LDFLAGS = -L../contrib/AntTweakBar/lib/ -L/usr/local/lib/ -lglfw3 -lGL -lGLU -lX11 -lXi -lpthread -lXxf86vm -lXrandr -lAntTweakBar -lGLEW -ffast-math -lpthread -lfftw3 -lXinerama -lXcursor -Wl,-rpath=../contrib/AntTweakBar/lib/

SOURCES=geometryutils.cpp genericutils.cpp shader.cpp nanosvg.cpp simplerender.cpp configparsing.cpp datastreamer.cpp datavals.cpp teststreamer.cpp jsoncpp.cpp visualelement.cpp cameracontrol.cpp polygon.cpp highlighter.cpp equation.cpp plotter.cpp plotbundler.cpp equationmap.cpp logging.cpp


OBJECTS=$(SOURCES:.cpp=.o)
HEADERS=$(SOURCES:.cpp=.h)
EXECUTABLE=lyrebird

all: $(SOURCES) $(EXECUTABLE) 

$(EXECUTABLE): $(OBJECTS) main.o
	$(CC) main.o $(OBJECTS) -o $@ $(LDFLAGS)


.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o
	rm -f *~
	rm -f lyrebird
	rm -f hktest
	rm -f rm \#*


