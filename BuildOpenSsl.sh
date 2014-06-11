set -x

if [ -z "$START_DIR" ]; then

	cd dev/openssl-fips-ecp-2.0.2
else
	echo "START_DIR defined"
	cd $START_DIR
fi

. ../Setenv-android.sh &&
echo "*****************************"
./config &&
echo "*****************************"
make   &&
sudo make install &&
cd ../.. &&
cd dev/openssl-1.0.1f &&
./config fips no-ec2m --with-fipslibdir=/usr/local/ssl/fips-2.0/lib/ &&
echo "*****************************"
make depend &&
echo "*****************************"
make &&

cd ../../
set +x