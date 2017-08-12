SRCPATH=.
prefix=/usr/local
exec_prefix=${prefix}
bindir=${exec_prefix}/bin
libdir=${exec_prefix}/lib
includedir=${prefix}/include
SYS_ARCH=X86_64
SYS=MACOSX
CC=gcc
CFLAGS=-Wshadow -O3 -ffast-math -m64  -Wall -I. -I$(SRCPATH) -mdynamic-no-pic -arch x86_64 -std=gnu99 -D_GNU_SOURCE  -I/usr/local/Cellar/ffmpeg/3.2.1/include  -I/usr/local/Cellar/ffmpeg/3.2.1/include -fomit-frame-pointer -fno-tree-vectorize
COMPILER=GNU
COMPILER_STYLE=GNU
DEPMM=-MM -g0
DEPMT=-MT
LD=gcc -o 
LDFLAGS=-m64  -lm -arch x86_64 -lpthread -ldl
LIBX264=libx264.a
AR=ar rc 
RANLIB=ranlib
STRIP=strip
INSTALL=install
AS=
ASFLAGS= -I. -I$(SRCPATH) -DARCH_X86_64=1 -I$(SRCPATH)/common/x86/ -f macho64 -DPIC -DPREFIX -DSTACK_ALIGNMENT=16 -DHIGH_BIT_DEPTH=0 -DBIT_DEPTH=8
RC=
RCFLAGS=
EXE=
HAVE_GETOPT_LONG=1
DEVNULL=/dev/null
PROF_GEN_CC=-fprofile-generate
PROF_GEN_LD=-fprofile-generate
PROF_USE_CC=-fprofile-use
PROF_USE_LD=-fprofile-use
HAVE_OPENCL=yes
default: cli
install: install-cli
LDFLAGSCLI = -ldl -L.  -L/usr/local/Cellar/ffmpeg/3.2.1/lib -lavformat -lavcodec -lavutil -lswscale  -L/usr/local/Cellar/ffmpeg/3.2.1/lib -lswscale -lavutil 
CLI_LIBX264 = $(LIBX264)
