#!/bin/sh

# directories
SOURCE="ffmpeg-3.0"
FAT="FFmpeg-iOS"


CUR_WORK_ROOT_DIR=$(pwd)
TARGET_LIBRARY_DIR=$CUR_WORK_ROOT_DIR/../lib
FFMPEG_OUTPUT_TARGET_DIR=$TARGET_LIBRARY_DIR/ffmpeg

SCRATCH="scratch"

# must be an absolute path
# THIN=`pwd`/"thin"
## =====================================>>>

# absolute path to x264 library
# X264=$TARGET_LIBRARY_DIR/x264

#FDK_AAC=`pwd`/fdk-aac/fdk-aac-ios

CONFIGURE_FLAGS="--enable-shared --enable-cross-compile --disable-doc --enable-pic"

## =====================================>>>

# if [ "$X264" ]
# then
	CONFIGURE_FLAGS="$CONFIGURE_FLAGS --enable-gpl --enable-libx264"
# fi

# if [ "$FDK_AAC" ]
# then
	CONFIGURE_FLAGS="$CONFIGURE_FLAGS --enable-libfdk-aac --enable-nonfree"
# fi

# avresample
CONFIGURE_FLAGS="$CONFIGURE_FLAGS --enable-avresample"
## =====================================>>>


#ARCHS="arm64 armv7 x86_64 i386"
ARCHS="x86_64"

COMPILE="y"
LIPO="y"

DEPLOYMENT_TARGET="6.0"

if [ "$COMPILE" ]
then
	if [ ! `which yasm` ]
	then
		echo 'Yasm not found'
		if [ ! `which brew` ]
		then
			echo 'Homebrew not found. Trying to install...'
                        ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)" \
				|| exit 1
		fi
		echo 'Trying to install Yasm...'
		brew install yasm || exit 1
	fi

#	if [ ! `which gas-preprocessor.pl` ]
#	then
#		echo 'gas-preprocessor.pl not found. Trying to install...'
#		(curl -L https://github.com/libav/gas-preprocessor/raw/master/gas-preprocessor.pl \
#			-o /usr/local/bin/gas-preprocessor.pl \
#			&& chmod +x /usr/local/bin/gas-preprocessor.pl) \
#			|| exit 1
#	fi

	# poll the value from $ARCHS
	for ARCH in $ARCHS
	do
		echo "building $ARCH..."
		mkdir -p "$FFMPEG_OUTPUT_TARGET_DIR/$ARCH"
		cd "$FFMPEG_OUTPUT_TARGET_DIR/$ARCH"

		CFLAGS="-arch $ARCH"
		if [ "$ARCH" = "i386" -o "$ARCH" = "x86_64" ]
		then
		    PLATFORM=""
		    CFLAGS=""
		    # PLATFORM="iPhoneSimulator"
		    # CFLAGS="$CFLAGS -mios-simulator-version-min=$DEPLOYMENT_TARGET"
		else
		    PLATFORM="iPhoneOS"
		    CFLAGS="$CFLAGS -mios-version-min=$DEPLOYMENT_TARGET -fembed-bitcode"
		    if [ "$ARCH" = "arm64" ]
		    then
		        EXPORT="GASPP_FIX_XCODE5=1"
		    fi
		fi

		XCRUN_SDK=`echo $PLATFORM | tr '[:upper:]' '[:lower:]'`
		# CC="xcrun -sdk $XCRUN_SDK clang"
		CC="gcc"
		CXXFLAGS="$CFLAGS"
		LDFLAGS="$CFLAGS"
		if [ "$X264" ]
		then
			CFLAGS="$CFLAGS -I$X264/$ARCH/include"
			LDFLAGS="$LDFLAGS -L$X264/$ARCH/lib"
		fi
		if [ "$FDK_AAC" ]
		then
			CFLAGS="$CFLAGS -I$FDK_AAC/include"
			LDFLAGS="$LDFLAGS -L$FDK_AAC/lib"
		fi
		
		echo "$FFMPEG_OUTPUT_TARGET_DIR/$ARCH"
		
		echo "===================================="
		echo $ARCH
		echo $CC
		echo $CONFIGURE_FLAGS
		echo $CFLAGS
		echo $LDFLAGS
		echo $FFMPEG_OUTPUT_TARGET_DIR/$ARCH
		echo "===================================="
		# exit
	
		# -cc="$CC" 
		TMPDIR=${TMPDIR/%\/} $CUR_WORK_ROOT_DIR/configure \
		    --target-os=darwin \
		    --arch=$ARCH
		    $CONFIGURE_FLAGS \
		    --extra-cflags="$CFLAGS" \
		    --extra-ldflags="$LDFLAGS" \
		    --prefix="$FFMPEG_OUTPUT_TARGET_DIR/$ARCH" \
		|| exit 1

		make -j3 install $EXPORT || exit 1
		cd $CWD
	done
fi

echo Done
