APP_PROJECT_PATH := $(shell pwd)

# Note: by changing the following line you can reduce (or add to) the number
# of platforms to build. This makes sense when debugging build so you don't have to 
# wait for all three platforms.
# HOWEVER, you MUST include armeabi as at least one of the platforms or you will
# get an error
# APP_ABI := armeabi armeabi-v7a x86   <-- this builds 3 platforms
APP_ABI := armeabi armeabi-v7a x86 
APP_BUILD_SCRIPT := $(APP_PROJECT_PATH)/Android.mk
# fixes this error when building external/android-sqlite/android/sqlite3_android.cpp
#   icu4c/common/unicode/std_string.h:39:18: error: string: No such file or directory
APP_STL := stlport_shared
