CXX := x86_64-w64-mingw32-g++
CC := x86_64-w64-mingw32-gcc
TARGET := gdrender-raylib.exe
SOURCES := $(filter-out src/stb_image.cpp,$(wildcard src/*.cpp))
INCLUDES := -I../raylib -Iinclude -Iinclude/stb -Iinclude/portable-file-dialogs
CXXFLAGS := -O2 -Wall -std=c++20
LDFLAGS := ../raylib/libraylib.a -lopengl32 -lgdi32 -lwinmm -lpthread -lwinhttp -lshell32 -lole32 -lcomdlg32 -luuid -static -static-libgcc -static-libstdc++

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(SOURCES) -o $@ $(CXXFLAGS) $(INCLUDES) $(LDFLAGS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
