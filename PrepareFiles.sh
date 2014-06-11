
cd dev
export START_DIR="$PWD"

echo "Unzipping openssl-1.0.1f.tar.gz, and openssl-fips-ecp-2.0.2.tar.gz"
gunzip -c openssl-fips-ecp-2.0.2.tar.gz | tar xf -
gunzip -c openssl-1.0.1f.tar.gz | tar xf -

echo
echo "Updating make files"
cp SupplementalFiles/Makefile android-database-sqlcipher/
cp SupplementalFiles/Application.mk android-database-sqlcipher/jni/
cp SupplementalFiles/*.c android-database-sqlcipher/external
cp SupplementalFiles/*.sh android-database-sqlcipher/external

cd ..
