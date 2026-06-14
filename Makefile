APP_OBJECTS=src/main.o
RENDERER_OBJECTS=src/Renderer/Renderer.o
MY_MTK_VIEW_DELEGATE_OBJECTS=src/MyMTKViewDelegate/MyMTKViewDelegate.o
MY_APP_DELEGATE_OBJECTS=src/MyAppDelegate/MyAppDelegate.o

ifdef DEBUG
DBG_OPT_FLAGS=-g
else
DBG_OPT_FLAGS=-O2
endif

ifdef ASAN
ASAN_FLAGS=-fsanitize=address
else
ASAN_FLAGS=
endif

CC=clang++
CFLAGS=-Wall -std=c++17 \
	-I./third_party/metal-cpp \
	-I./third_party/metal-cpp-extensions \
	-I./src \
	-fno-objc-arc \
	$(DBG_OPT_FLAGS) $(ASAN_FLAGS)
LDFLAGS=-framework Metal -framework Foundation -framework Cocoa -framework CoreGraphics -framework MetalKit

%.o: %.cpp
	$(CC) -c $(CFLAGS) $< -o $@


all: lps-server

.PHONY: all clean cleanExe

lps-server: $(APP_OBJECTS) $(RENDERER_OBJECTS) $(MY_MTK_VIEW_DELEGATE_OBJECTS) $(MY_APP_DELEGATE_OBJECTS) Makefile
	$(CC) $(CFLAGS) $(LDFLAGS) $(APP_OBJECTS) $(RENDERER_OBJECTS) $(MY_MTK_VIEW_DELEGATE_OBJECTS) $(MY_APP_DELEGATE_OBJECTS) -o $@

clean:
	rm -f $(APP_OBJECTS) \
		$(RENDERER_OBJECTS) \
		$(MY_MTK_VIEW_DELEGATE_OBJECTS) \
		$(MY_APP_DELEGATE_OBJECTS)

cleanExe:
	rm -f lps-server
