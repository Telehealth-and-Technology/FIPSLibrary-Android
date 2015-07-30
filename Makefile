.DEFAULT_GOAL := help
LIBRARY_ROOT := libs
JNI_DIR := ${CURDIR}/jni
EXTERNAL_DIR := ${CURDIR}/external
DEV_DIR := ${CURDIR}/dev
FIPS_DIR := ${CURDIR}/dev/openssl-fips-ecp-2.0.2
SSL_DIR := ${CURDIR}/dev/openssl-1.0.1f
SQLCIPHER_DIR := ${CURDIR}/dev/android-database-sqlcipher
EXTERNAL_DIR := ${CURDIR}/dev/android-database-sqlcipher/external
TEST_DIR := ${CURDIR}/test

LICENSE := SQLCIPHER_LICENSE
ASSETS_DIR := assets
OPENSSL_DIR := ${EXTERNAL_DIR}/openssl
LATEST_TAG := $(shell git tag | sort -r | head -1)
SECOND_LATEST_TAG := $(shell git tag | sort -r | head -2 | tail -1)
RELEASE_DIR := "SQLCipher for Android ${LATEST_TAG}"
CHANGE_LOG_HEADER := "Changes included in the ${LATEST_TAG} release of SQLCipher for Android:"
README := ${RELEASE_DIR}/README



help:
	#  Build Commands:
	#    prepare     - Unzips packages and moved=s files to appropriate places for build"
	#    fips        - Builds and installs the offocial FIPS module (fipscanister.o)
	#    ssl         - Builds the fips compatible ssl package using fip module produces here
	#    sqlcipher   - Builds the bulk of the sqlcipher files
	#    t2          - Links the official openssl capable build into libraries that android can load
	#                  also installs the libraries in the android test project
	# Release Commands
	#    doRelease   - Prepares and creates a release
	#
	# Clean Commands
	#    clean       - Cleans binary/object files
	#    cleanAll    - Cleans binary/object files AND android-database-sqlcipher



doRelease:
	. ./doRelease.sh

buildAllLocal:	copyLocalFipsFiles copyLocalSqlCipherFiles prepare fips ssl sqlcipher t2 check

buildAll:
	. ./buildAll.sh

cleanAll: clean cleanCannedFiles

cleanfipswrapper:
	echo "cleaning fipswrapper files" && \
	cd dev/fipswrapper && \
	./clean.sh
	cd ../..

buildfipswrapper:
	echo "Building fipswrapper files" && \
	cd dev/fipswrapper && \
	ant debug	
	cp dev/fipswrapper/bin/fipswrapper.jar test/FcadsTestAndroidApp/libs


clean:
	make cleanfipswrapper
	echo "Cleaning files" && \
	cd ${DEV_DIR} && \
	rm -rfd openssl-fips-ecp-2.0.2 && \
	rm -rfd openssl-1.0.1f && \
	rm -rf openssl-fips-ecp-2.0.2.tar.gz && \
	rm -rf openssl-1.0.1f.tar.gz && \
	rm -rfd android-database-sqlcipher/libs/armeabi/libsqlcipher_android.so && \
	rm -rfd android-database-sqlcipher/libs/armeabi-v7a/libsqlcipher_android.so && \
	rm -rfd android-database-sqlcipher/libs/x86/libsqlcipher_android.so && \
	rm -rfd ${TEST_DIR}/FcadsTestAndroidApp/libs/*.*
	rm -rfd ${TEST_DIR}/FcadsTestAndroidApp/libs/armeabi/*.*
	rm -rfd ${TEST_DIR}/FcadsTestAndroidApp/libs/armeabi-v7a/*.*
	rm -rfd ${TEST_DIR}/FcadsTestAndroidApp/libs/x86/*.*



cleanCannedFiles:
	rm -rfd ${SQLCIPHER_DIR}

check:
	. ./checkArtifacts.sh

copyLocalFipsFiles:
	cp localFipsSslFiles/openssl-1.0.1f.tar.gz dev
	cp localFipsSslFiles/openssl-fips-ecp-2.0.2.tar.gz dev

copyLocalSqlCipherFiles:
	echo "Copying canned files from local resources at /Users/scoleman/release/sqlCipherDownload_7-9-2014/android-database-sqlcipher"
	cp -r /Users/scoleman/release/sqlCipherDownload_7-9-2014/android-database-sqlcipher* dev/

prepare:
	cd ${DEV_DIR} && \
	gunzip -c openssl-fips-ecp-2.0.2.tar.gz | tar xf - && \
	gunzip -c openssl-1.0.1f.tar.gz | tar xf - && \
	cp SupplementalFiles/Makefile android-database-sqlcipher/ && \
	cp SupplementalFiles/Application.mk android-database-sqlcipher/jni/ && \
	cp SupplementalFiles/*.c android-database-sqlcipher/external && \
	cp SupplementalFiles/*.cpp android-database-sqlcipher/external && \
	cp SupplementalFiles/*.sh android-database-sqlcipher/external	

fips:
	cd ${FIPS_DIR} && \
	. ../Setenv-android.sh && \
	./config && \
	make && \
	sudo make install

ssl:
	cd ${FIPS_DIR} && \
	. ../Setenv-android.sh && \
	cd ${SSL_DIR} && \
	./config fips no-ec2m --with-fipslibdir=/usr/local/ssl/fips-2.0/lib/ && \
	make depend && \
	make

sqlcipher:
	cd ${DEV_DIR}  &&\
	cp openssl-1.0.1f/libcrypto.a android-database-sqlcipher/external/android-libs/armeabi && \
	cp openssl-1.0.1f/libcrypto.a android-database-sqlcipher/external/android-libs/armeabi-v7a && \
	cd ${SQLCIPHER_DIR} && \
	echo "--------------------------------------------------" && \
	echo "------------ Performing make clean ---------------" && \
	make clean && \
	echo "--------------------------------------------" && \
	echo "------------ Performing make ---------------" && \
	make

t2:
	cd ${EXTERNAL_DIR}  && \
	. ./buildit.sh && \
	cd ${DEV_DIR}  &&\
	rsync -a android-database-sqlcipher/libs ${TEST_DIR}/FcadsTestAndroidApp

	

