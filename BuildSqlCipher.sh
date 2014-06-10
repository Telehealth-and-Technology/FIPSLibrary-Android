set -x
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
cd ../../
set +x