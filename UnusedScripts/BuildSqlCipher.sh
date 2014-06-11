set -x
if [ -z "$START_DIR" ]; then

	cd dev
else
	echo "START_DIR defined"
	cd $START_DIR/dev
fi

function cleanUp() {
	if [ -z "$START_DIR" ]; then
		cd ../../
	else
		cd $START_DIR
	fi	
}

trap cleanUp EXIT


cd dev
echo "*******************************"
echo "Copying currently built libcrypto.a for sqlcipher to use"
cp openssl-1.0.1f/libcrypto.a android-database-sqlcipher/external/android-libs/armeabi &&
cp openssl-1.0.1f/libcrypto.a android-database-sqlcipher/external/android-libs/armeabi-v7a &&
cd android-database-sqlcipher &&
echo "*******************************"
echo "make clean"
make clean&&
echo "*******************************"
echo "make"
make &&

trap -EXIT

if [ -z "$START_DIR" ]; then
	cd ../../
else
	cd $START_DIR
fi



set +x