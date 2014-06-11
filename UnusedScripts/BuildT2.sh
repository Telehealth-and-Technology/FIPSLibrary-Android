set -x
echo "*******************************"
echo "Building T2 Addon to Sqlcipher"
cd dev/android-database-sqlcipher/external &&
. ./buildit.sh &&
cd ../../.. &&

echo "*******************************"
echo "Copying lib files to Android test app "
cp -R dev/android-database-sqlcipher/libs test/FcadsTestAndroidApp &&



set +x