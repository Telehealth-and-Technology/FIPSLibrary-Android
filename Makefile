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
	#  Commands:
	#       getCannedFiles - Grabs canned files from a local director
	#       prepare        - Unzips packages and moved=s files to appropriate places for build"
	#       fips           - Builds and installs the offocial FIPS module (fipscanister.o)
	#		ssl            - Builds the fips compatible ssl package using fip module produces here
	#       sqlcipher      - Builds the bulk of the sqlcipher files
	#       t2             - Links the official openssl capable build into libraries that android can load
	#                        also installs the libraries in the android test project

clean:
	echo "Cleaning files" && \
	cd ${DEV_DIR} && \
	rm -rfd openssl-fips-ecp-2.0.2 && \
	rm -rfd openssl-1.0.1f && \
	rm -rfd android-database-sqlcipher/libs/armeabi/libsqlcipher_android.so && \
	rm -rfd ${TEST_DIR}/FcadsTestAndroidApp/libs

cleanCannedFiles:
	rm -rfd ${SQLCIPHER_DIR}


getCannedFiles:
	echo "Copying canned files from local resources at /Users/scoleman/dev/OpenSSLTestCompiles/5-28-14FcadsResources"
	cp /Users/scoleman/dev/OpenSSLTestCompiles/5-28-14FcadsResources/*.gz dev
	cp -r /Users/scoleman/dev/OpenSSLTestCompiles/5-28-14FcadsResources/android-database-sqlcipher* dev/

scott1:
	if test -d ${TEST_DIR}/FcadsTestAndroidApp/libs ; then echo "hello there"; fi && \
	if test -d ${SQLCIPHER_DIR} ; then echo "hi there"; fi



prepare:
	cd ${DEV_DIR} && \
	gunzip -c openssl-fips-ecp-2.0.2.tar.gz | tar xf - && \
	gunzip -c openssl-1.0.1f.tar.gz | tar xf - && \
	cp SupplementalFiles/Makefile android-database-sqlcipher/ && \
	cp SupplementalFiles/Application.mk android-database-sqlcipher/jni/ && \
	cp SupplementalFiles/*.c android-database-sqlcipher/external && \
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
	cp -R android-database-sqlcipher/libs ${TEST_DIR}/FcadsTestAndroidApp

	

