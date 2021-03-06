.PHONY : mingw pixel linux undefined

CFLAGS = -g -Wall -I./ -Isrc -I../lua-5.3.2/src -DPIXEL_LUA -DLUA_USE_DLOPEN -DLUA_COMPAT_MATHLIB
LDFLAGS :=

SRC := array.c color.c font.c font_ctx.c lgeometry.c glsl.c hash.c label.c log.c matrix.c \
	particle.c pixel.c readfile.c render.c renderbuffer.c scissor.c screen.c shader.c \
	sprite.c spritepack.c stream.c texture.c

UNAME=$(shell uname)
SYS=$(if $(filter Linux%,$(UNAME)),linux,\
	    $(if $(filter MINGW%,$(UNAME)),mingw,\
	    $(if $(filter Darwin%,$(UNAME)),macosx,\
	        undefined\
)))

all: $(SYS)

undefined:
	@echo "I can't guess your platform, please do 'make PLATFORM' where PLATFORM is one of these:"
	@echo "      linux mingw macosx"

mingw : TARGET := pixel.exe
mingw : CFLAGS += -I/usr/include
mingw : LDFLAGS += -L/usr/bin -L/usr/local/bin -lgdi32 -lglew32 -lopengl32 -llua53
mingw : PLATFORM := platform/win32.c
mingw : pixel

core : TARGET := pixel.dll
core : CFLAGS += -I/usr/include --shared
core : LDFLAGS += -L/usr/bin -L/usr/local/bin -lgdi32 -lglew32 -lopengl32 -llua53
core : PLATFORM := src/lcore.c
core : pixel_core

linux : TARGET := pixel
linux : CFLAGS +=
linux : LDFLAGS += -L../lua-5.3.2/src -Wl,-E -Wl,-rpath,../lua-5.3.2/src -lGLEW -lGL -lX11 -lm -ldl -llua
linux : PLATFORM := platform/linux.c
linux : pixel

pixel_core : $(foreach v, $(SRC), src/$(v))
	gcc $(CFLAGS) -o $(TARGET) $^ $(PLATFORM) $(LDFLAGS)

pixel : $(foreach v, $(SRC), src/$(v))
	gcc $(CFLAGS) -o $(TARGET) $^ $(PLATFORM) $(LDFLAGS)

clean :
	-rm -f pixel.exe
	-rm -f pixel.dll
	-rm -f pixel
