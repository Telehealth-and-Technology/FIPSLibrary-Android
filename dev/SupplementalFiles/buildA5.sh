# Set ANDROID_NDK_ROOT to you NDK location. For example,
# /opt/android-ndk-r8e. This can be done in a login script. If
# ANDROID_NDK_ROOT is not specified, the script will try to pick it
# up with the value of _ANDROID_NDK_ROOT below. If ANDROID_NDK_ROOT
# is set, then the value is ignored.
_ANDROID_NDK="android-ndk-r9d"

# Set _ANDROID_EABI to the EABI you want to use. You can find the
# list in $ANDROID_NDK_ROOT/toolchains. This value is always used.
_ANDROID_EABI="arm-linux-androideabi-4.6"

# Set _ANDROID_API to the API you want to use. You should set it
# to one of: android-14, android-9, android-8, android-14, android-5
# android-4, or android-3. You can't set it to the latest (for
# example, API-17) because the NDK does not supply the platform. At
# Android 5.0, there will likely be another plaform added (android-18?).
# This value is always used.
_ANDROID_API="android-14"

#####################################################################

# If the user did not specify the NDK location, try and pick it up.
# We expect something like ANDROID_NDK_ROOT=/opt/android-ndk-r8e
# or ANDROID_NDK_ROOT=/usr/local/android-ndk-r8e.

if [ -z "$ANDROID_NDK_ROOT" ]; then

  _ANDROID_NDK_ROOT=""
  if [ -z "$_ANDROID_NDK_ROOT" ] && [ -d "/usr/local/$_ANDROID_NDK" ]; then
    _ANDROID_NDK_ROOT="/usr/local/$_ANDROID_NDK"
  fi

  if [ -z "$_ANDROID_NDK_ROOT" ] && [ -d "/opt/$_ANDROID_NDK" ]; then
    _ANDROID_NDK_ROOT="/opt/$_ANDROID_NDK"
  fi

  if [ -z "$_ANDROID_NDK_ROOT" ] && [ -d "$HOME/$_ANDROID_NDK" ]; then
    _ANDROID_NDK_ROOT="$HOME/$_ANDROID_NDK"
  fi

  if [ -z "$_ANDROID_NDK_ROOT" ] && [ -d "$PWD/$_ANDROID_NDK" ]; then
    _ANDROID_NDK_ROOT="$PWD/$_ANDROID_NDK"
  fi

  # If a path was set, then export it
  if [ ! -z "$_ANDROID_NDK_ROOT" ] && [ -d "$_ANDROID_NDK_ROOT" ]; then
    export ANDROID_NDK_ROOT="$_ANDROID_NDK_ROOT"
  fi
fi


# Error checking
# ANDROID_NDK_ROOT should always be set by the user (even when not running this script)
# http://groups.google.com/group/android-ndk/browse_thread/thread/a998e139aca71d77
if [ -z "$ANDROID_NDK_ROOT" ] || [ ! -d "$ANDROID_NDK_ROOT" ]; then
  echo "Error: ANDROID_NDK_ROOT is not a valid path. Please edit this script.  xxx"
  # echo "$ANDROID_NDK_ROOT"
  # exit 1
fi


# Error checking
if [ ! -d "$ANDROID_NDK_ROOT/toolchains" ]; then
  echo "Error: ANDROID_NDK_ROOT/toolchains is not a valid path. Please edit this script.xxxxx"
  # echo "$ANDROID_NDK_ROOT/toolchains"
  # exit 1
fi


# Error checking
if [ ! -d "$ANDROID_NDK_ROOT/toolchains/$_ANDROID_EABI" ]; then
  echo "Error: ANDROID_EABI is not a valid path. Please edit this script."
  # echo "$ANDROID_NDK_ROOT/toolchains/$_ANDROID_EABI"
  # exit 1
fi

#####################################################################

# Based on ANDROID_NDK_ROOT, try and pick up the required toolchain. We expect something like:
# /opt/android-ndk-r83/toolchains/arm-linux-androideabi-4.7/prebuilt/linux-x86_64/bin
# Once we locate the toolchain, we add it to the PATH. Note: this is the 'hard way' of
# doing things according to the NDK documentation for Ice Cream Sandwich.
# https://android.googlesource.com/platform/ndk/+/ics-mr0/docs/STANDALONE-TOOLCHAIN.html

ANDROID_TOOLCHAIN=""
for host in "linux-x86_64" "linux-x86" "darwin-x86_64" "darwin-x86"
do
  if [ -d "$ANDROID_NDK_ROOT/toolchains/$_ANDROID_EABI/prebuilt/$host/bin" ]; then
    ANDROID_TOOLCHAIN="$ANDROID_NDK_ROOT/toolchains/$_ANDROID_EABI/prebuilt/$host/bin"
    break
  fi
done

# Error checking
if [ -z "$ANDROID_TOOLCHAIN" ] || [ ! -d "$ANDROID_TOOLCHAIN" ]; then
  echo "Error: ANDROID_TOOLCHAIN is not valid. Please edit this script."
  echo "$ANDROID_TOOLCHAIN"
  # exit 1
fi

# Error checking
if [ ! -e "$ANDROID_TOOLCHAIN/arm-linux-androideabi-gcc" ]; then
  echo "Error: Failed to find Android gcc. Please edit this script."
  # echo "$ANDROID_TOOLCHAIN/arm-linux-androideabi-gcc"
  # exit 1
fi

# Error checking
if [ ! -e "$ANDROID_TOOLCHAIN/arm-linux-androideabi-ranlib" ]; then
  echo "Error: Failed to find Android ranlib. Please edit this script."
  # echo "$ANDROID_TOOLCHAIN/arm-linux-androideabi-ranlib"
  # exit 1
fi

# Only modify/export PATH if ANDROID_TOOLCHAIN good
if [ ! -z "$ANDROID_TOOLCHAIN" ]; then
  export ANDROID_TOOLCHAIN="$ANDROID_TOOLCHAIN"
  export PATH="$ANDROID_TOOLCHAIN":"$PATH"
fi

#####################################################################

# For the Android SYSROOT. Can be used on the command line with --sysroot
# https://android.googlesource.com/platform/ndk/+/ics-mr0/docs/STANDALONE-TOOLCHAIN.html
export ANDROID_SYSROOT="$ANDROID_NDK_ROOT/platforms/$_ANDROID_API/arch-arm"
export SYSROOT="$ANDROID_SYSROOT"
export NDK_SYSROOT="$ANDROID_SYSROOT"

# Error checking
if [ -z "$ANDROID_SYSROOT" ] || [ ! -d "$ANDROID_SYSROOT" ]; then
  echo "Error: ANDROID_SYSROOT is not valid. Please edit this script."
  # echo "$ANDROID_SYSROOT"
  # exit 1
fi

#####################################################################

# If the user did not specify the FIPS_SIG location, try and pick it up
# If the user specified a bad location, then try and pick it up too.
if [ -z "$FIPS_SIG" ] || [ ! -e "$FIPS_SIG" ]; then

  # Try and locate it
  _FIPS_SIG=""
  if [ -d "/usr/local/ssl/$_ANDROID_API" ]; then
    _FIPS_SIG=`find "/usr/local/ssl/$_ANDROID_API" -name incore`
  fi

  if [ ! -e "$_FIPS_SIG" ]; then
    _FIPS_SIG=`find $PWD -name incore`
  fi

  # If a path was set, then export it
  if [ ! -z "$_FIPS_SIG" ] && [ -e "$_FIPS_SIG" ]; then
    export FIPS_SIG="$_FIPS_SIG"
  fi
fi

# Error checking
if [ -z "$FIPS_SIG" ] || [ ! -e "$FIPS_SIG" ]; then
  echo "Error: FIPS_SIG does not specify incore module. Please edit this script."
  # echo "$FIPS_SIG"
  # exit 1
fi

#####################################################################

# Most of these should be OK (MACHINE, SYSTEM, ARCH). RELEASE is ignored.
export MACHINE=armv7
export RELEASE=2.6.37
export SYSTEM=android
export ARCH=arm

# For the Android toolchain
# https://android.googlesource.com/platform/ndk/+/ics-mr0/docs/STANDALONE-TOOLCHAIN.html
export ANDROID_SYSROOT="$ANDROID_NDK_ROOT/platforms/$_ANDROID_API/arch-arm"
export SYSROOT="$ANDROID_SYSROOT"
export NDK_SYSROOT="$ANDROID_SYSROOT"
export ANDROID_NDK_SYSROOT="$ANDROID_SYSROOT"
export ANDROID_API="$_ANDROID_API"

# CROSS_COMPILE and ANDROID_DEV are DFW (Don't Fiddle With). Its used by OpenSSL build system.
export CROSS_COMPILE="arm-linux-androideabi-"
export ANDROID_DEV="$ANDROID_NDK_ROOT/platforms/$_ANDROID_API/arch-arm/usr"
export HOSTCC=gcc

VERBOSE=1
if [ ! -z "$VERBOSE" ] && [ "$VERBOSE" != "0" ]; then
  echo "ANDROID_NDK_ROOT: $ANDROID_NDK_ROOT"
  echo "ANDROID_EABI: $_ANDROID_EABI"
  echo "ANDROID_API: $ANDROID_API"
  echo "ANDROID_SYSROOT: $ANDROID_SYSROOT"
  echo "ANDROID_TOOLCHAIN: $ANDROID_TOOLCHAIN"
  echo "FIPS_SIG: $FIPS_SIG"
  echo "CROSS_COMPILE: $CROSS_COMPILE"
  echo "ANDROID_DEV: $ANDROID_DEV"
fi
echo ""





rm /Users/scoleman/dev/OpenSSLTestCompiles/4-3-14/android-database-sqlcipher/obj/local/armeabi/libsqlcipher.a
rm /Users/scoleman/dev/OpenSSLTestCompiles/4-3-14/android-database-sqlcipher/obj/local/armeabi/libsqlcipher_android.so
find android-sqlite -exec touch {} \;


# This is the build script command we're trying to replicate below:
#ndk-build V=1 NDK_LIBS_OUT=./junk/junk1


#arm-linux-androideabi-gcc





echo --------
echo --------


libsqlite3_android_local_src_files="android-sqlite/android/sqlite3_android.cpp \
	android-sqlite/android/PhonebookIndex.cpp \
	android-sqlite/android/PhoneNumberUtils.cpp \
	android-sqlite/android/OldPhoneNumberUtils.cpp \
	android-sqlite/android/PhoneticStringUtils.cpp \
	String16.cpp \
	String8.cpp"
#	wrapper.c "


PROJECT_ROOT_PATH=$PWD
LOCAL_PATH=$PROJECT_ROOT_PATH

LOCAL_C_INCLUDES="-I$LOCAL_PATH/includes \
	-I$LOCAL_PATH/sqlcipher \
	-I$LOCAL_PATH/icu4c/i18n \
	-I$LOCAL_PATH/icu4c/common \
	-I$LOCAL_PATH/platform-system-core/include \
	-I$LOCAL_PATH/icu4c/common/unicode \
	-I$LOCAL_PATH/platform-system-core/include \
	-I$LOCAL_PATH/platform-frameworks-base/include \
	-I/Users/scoleman/android-ndk-r9d/sources/cxx-stl/stlport/stlport \
	-I/usr/local/ssl/android-14/include"
#	-I/Users/scoleman/dev/OpenSSLTestCompiles/4-3-14/android-database-sqlcipher/external/platform-frameworks-base/include/utils"
	

#	-I/Users/scoleman/android-ndk-r9d/platforms/android-3/arch-arm/usr/include"
	
#echo $LOCAL_C_INCLUDES	

TARGET_ARCH_ABI=armeabi
	
LOCAL_ALLOW_UNDEFINED_SYMBOLS=false

# TODO this needs to depend on libsqlcipher being built, how to do that?
#LOCAL_REQUIRED_MODULES += libsqlcipher libicui18n libicuuc 
LOCAL_STATIC_LIBRARIES="libsqlcipher libicui18n libicuuc static-libcrypto"

android_sqlite_cflags="-DHAVE_USLEEP=1 \
	-DSQLITE_DEFAULT_JOURNAL_SIZE_LIMIT=1048576 -DSQLITE_THREADSAFE=1 -DNDEBUG=1 \
	-DSQLITE_ENABLE_MEMORY_MANAGEMENT=1 -DSQLITE_TEMP_STORE=3 \
	-DSQLITE_ENABLE_FTS3 -DSQLITE_ENABLE_FTS3_BACKWARDS \
	-DSQLITE_ENABLE_LOAD_EXTENSION \
  -DANDROID"

LOCAL_CFLAGS="$android_sqlite_cflags $sqlite_cflags -DOS_PATH_SEPARATOR='/' -DHAVE_SYS_UIO_H -fno-exceptions"
	
## *** need to fix absolute references here!	and elsewhere!

LOCAL_LDFLAGS="-L$LOCAL_PATH/android-libs/$TARGET_ARCH_ABI/ -L$LOCAL_PATH/libs/$TARGET_ARCH_ABI/"

#-L/Users/scoleman/android-ndk-r9d/platforms/android-3/arch-arm/usr/lib/

LOCAL_LDLIBS="-llog -lutils -lcutils"
LOCAL_MODULE=libsqlcipher_android
LOCAL_MODULE_FILENAME=libsqlcipher_android
LOCAL_SRC_FILES=$libsqlite3_android_local_src_files


#echo $LOCAL_SRC_FILES
echo ---now the actual compile--
echo

export CC=`find /usr/local/ssl/$ANDROID_API -name fipsld`
export FIPSLD_CC="$ANDROID_TOOLCHAIN/arm-linux-androideabi-gcc" 
/Users/scoleman/junk/gccxxx
#echo CC = $CC
#echo FIPSLD_CC = $FIPSLD_CC
echo LOCAL_LDFLAGS  = $LOCAL_LDFLAGS 
echo
echo LOCAL_LDLIBS  = $LOCAL_LDLIBS 
echo
echo LOCAL_CFLAGS = $LOCAL_CFLAGS
echo
echo

set -x	
#this one works fine
#arm-linux-androideabi-gcc $libsqlite3_android_local_src_files \
# $CC $libsqlite3_android_local_src_files \
arm-linux-androideabi-g++ $libsqlite3_android_local_src_files \
  -fpic -ffunction-sections -funwind-tables -fstack-protector -no-canonical-prefixes \
  -march=armv5te -mtune=xscale -msoft-float -fno-exceptions -fno-rtti \
  -mthumb -Os -g -DNDEBUG -fomit-frame-pointer -fno-strict-aliasing \
  -finline-limit=64 \
  $LOCAL_C_INCLUDES \
  -DANDROID \
  -DHAVE_USLEEP=1 \
  -DSQLITE_DEFAULT_JOURNAL_SIZE_LIMIT=1048576 \
  -DSQLITE_THREADSAFE=1 \
  -DNDEBUG=1 \
  -DSQLITE_ENABLE_MEMORY_MANAGEMENT=1 \
  -DSQLITE_TEMP_STORE=3 \
  -DSQLITE_ENABLE_FTS3 \
  -DSQLITE_ENABLE_FTS3_BACKWARDS \
  -DSQLITE_ENABLE_LOAD_EXTENSION \
  -DOS_PATH_SEPARATOR="'/'" \
  -DHAVE_SYS_UIO_H \
  -Wa,--noexecstack -Wformat -Werror=format-security \
  -frtti -fexceptions \
  -Wl,--no-undefined \
  $LOCAL_LDFLAGS \
  $LOCAL_LDLIBS \
  -shared -o libsqlcipher_android.so --sysroot=$ANDROID_SYSROOT \
	/Users/scoleman/dev/OpenSSLTestCompiles/4-21-14/android-database-sqlcipherNoX86/obj/local/armeabi/libsqlcipher.a \
	/Users/scoleman/dev/OpenSSLTestCompiles/4-21-14/android-database-sqlcipherNoX86/obj/local/armeabi/libicui18n.a \
  /Users/scoleman/dev/OpenSSLTestCompiles/4-21-14/android-database-sqlcipherNoX86/obj/local/armeabi/libicuuc.a \
	/usr/local/ssl/android-14/lib/libcrypto.a

#   -fno-exceptions -fno-rtti \

## in work
# arm-linux-androideabi-g++ $libsqlite3_android_local_src_files \
#   -fPIC  \
#   -ffunction-sections -funwind-tables -fstack-protector -no-canonical-prefixes \
#   -march=armv5te -mtune=xscale -msoft-float \
#   -mthumb -Os -g -DNDEBUG -fomit-frame-pointer -fno-strict-aliasing \
#   -finline-limit=64  \
#   -shared -o libsqlcipher_android.so --sysroot=$ANDROID_SYSROOT \
#   $LOCAL_C_INCLUDES \
#   $LOCAL_LDFLAGS \
#   $LOCAL_LDLIBS \
#   $LOCAL_CFLAGS \
#   -Wa,--noexecstack -Wformat -Werror=format-security \
#   -frtti -fno-exceptions \
#   /Users/scoleman/dev/OpenSSLTestCompiles/4-21-14/android-database-sqlcipherNoX86/obj/local/armeabi/libsqlcipher.a \
#   /Users/scoleman/dev/OpenSSLTestCompiles/4-21-14/android-database-sqlcipherNoX86/obj/local/armeabi/libicui18n.a \
#   /Users/scoleman/dev/OpenSSLTestCompiles/4-21-14/android-database-sqlcipherNoX86/obj/local/armeabi/libicuuc.a \
#   /usr/local/ssl/android-14/lib/libcrypto.a



## Save the original command line before I change it too mush
# arm-linux-androideabi-g++ $libsqlite3_android_local_src_files \
#   -fPIC  -shared -o libsqlcipher_android.so --sysroot=$ANDROID_SYSROOT \
#   $LOCAL_C_INCLUDES \
#   $LOCAL_LDFLAGS \
#   $LOCAL_LDLIBS \
#   $LOCAL_CFLAGS \
#   /Users/scoleman/dev/OpenSSLTestCompiles/4-21-14/android-database-sqlcipherNoX86/obj/local/armeabi/libsqlcipher.a \
#   /Users/scoleman/dev/OpenSSLTestCompiles/4-21-14/android-database-sqlcipherNoX86/obj/local/armeabi/libicui18n.a \
#   /Users/scoleman/dev/OpenSSLTestCompiles/4-21-14/android-database-sqlcipherNoX86/obj/local/armeabi/libicuuc.a \
#   /usr/local/ssl/android-14/lib/libcrypto.a



# do it with fipsld instead of compiling direct (also works fine)
#$CC $libsqlite3_android_local_src_files \
#	-fPIC -shared -o Builda4.so --sysroot=$ANDROID_SYSROOT \
#	$LOCAL_C_INCLUDES \
#	$LOCAL_LDFLAGS \
#	$LOCAL_LDLIBS \
#	$LOCAL_CFLAGS
	
#TODO - we need to add more here to cause it to create a static library instead of dynamic
# but i realized this is not the step I want - i really want BuildA4 instead

cp libsqlcipher_android.so ../libs/armeabi

set +x