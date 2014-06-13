MissingFiles=""
  echo ""
# First check for all of the fips module files
# - note there are some header files that we aren't checking but didn't want to list them all

# FIP module files
fips_standalone_sha1="/usr/local/ssl/fips-2.0/bin/fips_standalone_sha1"				
fipsLd="/usr/local/ssl/fips-2.0/bin/fipsld"			
premainC="/usr/local/ssl/fips-2.0/lib/fips_premain.c"
premainSha="/usr/local/ssl/fips-2.0/lib/fips_premain.c.sha1"
canisterO="/usr/local/ssl/fips-2.0/lib/fipscanister.o"
fanisterSha="/usr/local/ssl/fips-2.0/lib/fipscanister.o.sha1"


#openssl files
ssl="dev/openssl-1.0.1f/libcrypto.a"

#sqlcipher files
  l1="dev/android-database-sqlcipher/libs/armeabi/libdatabase_sqlcipher.so"
  l2="dev/android-database-sqlcipher/libs/armeabi/libsqlcipher_android.so"
  l3="dev/android-database-sqlcipher/libs/armeabi/libstlport_shared.so"
  l4="dev/android-database-sqlcipher/libs/commons-codec.jar"
  l5="dev/android-database-sqlcipher/libs/guava-r09.jar"
  l6="dev/android-database-sqlcipher/libs/sqlcipher.jar"

if [ ! -f "$fips_standalone_sha1" ]; then
  echo "$fips_standalone_sha1 is missing!"
  MissingFiles="0"
fi
if [ ! -f "$fipsLd" ]; then
  echo "$fipsLd is missing!"
  MissingFiles="0"
fi
if [ ! -f "$premainC" ]; then
  echo "$premainC is missing!"
    MissingFiles="0"
fi
if [ ! -f "$premainSha" ]; then
  echo "$premainSha is missing!"
    MissingFiles="0"
fi
if [ ! -f "$canisterO" ]; then
  echo "$canisterOis missing!"
    MissingFiles="0"
fi
if [ ! -f "$fanisterSha" ]; then
  echo "$fanisterSha is missing!"
    MissingFiles="0"
fi


# Check for the openssl compatible file 
if [ ! -f "$ssl" ]; then
  echo "$ssl is missing!"
    MissingFiles="0"
fi

# Check for sqlcipher files
if [ ! -f "$l1" ]; then
  echo "$l1 is missing!"
    MissingFiles="0"
fi
if [ ! -f "$l2" ]; then
  echo "$l2 is missing!"
    MissingFiles="0"
fi
if [ ! -f "$l3" ]; then
  echo "$l3 is missing!"
    MissingFiles="0"
fi
if [ ! -f "$l4" ]; then
  echo "$l4 is missing!"
    MissingFiles="0"
fi
if [ ! -f "$l5" ]; then
  echo "$l5 is missing!"
    MissingFiles="0"
fi
if [ ! -f "$l6" ]; then
  echo "$l6 is missing!"
    MissingFiles="0"
fi






if [ -n "$MissingFiles" ]; then 
  echo ""
  echo "!!!!!!!!!!!!!!!! There are some files missing !!!!!!!!!!!!!!!!"


else
  echo ""
	echo "*************************************************"
	echo "All artifact files are present and accounted for!"
	echo "*************************************************"
fi
  echo ""

