APP_PROJECT_PATH := $(shell pwd)
# APP_ABI := armeabi
APP_ABI := armeabi
APP_BUILD_SCRIPT := $(APP_PROJECT_PATH)/Android.mk
# fixes this error when building external/android-sqlite/android/sqlite3_android.cpp
#   icu4c/common/unicode/std_string.h:39:18: error: string: No such file or directory
APP_STL := stlport_shared
