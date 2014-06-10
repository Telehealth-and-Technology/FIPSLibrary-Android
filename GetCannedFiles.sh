
if [ ! -d "dev" ]; then
	echo "dev direcotry doesn't exist, creating it"
	mkdir dev
fi


echo "Copying canned files from local resources at /Users/scoleman/dev/OpenSSLTestCompiles/5-28-14FcadsResources"
cp /Users/scoleman/dev/OpenSSLTestCompiles/5-28-14FcadsResources/*.gz dev
cp -r /Users/scoleman/dev/OpenSSLTestCompiles/5-28-14FcadsResources/android-database-sqlcipher* dev/
cd dev
# set -x
# echo "Unzipping openssl-1.0.1f.tar.gz, and openssl-fips-ecp-2.0.2.tar.gz"
# gunzip -c openssl-fips-ecp-2.0.2.tar.gz | tar xf -
# gunzip -c openssl-1.0.1f.tar.gz | tar xf -
# set +x
cd ..
