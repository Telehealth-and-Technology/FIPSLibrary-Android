
echo "Checking for existence of delivery release files"
# Check for all of the necessary release files:
# - note there are some header files that we aren't checking but didn't want to list them all
#   Deliverable library files (For each platform)

#   sqlcipher files
#   fips module files (in protected user space : usr/local/....)
#   Compiled FIPS library (libcrypto.a)


MY_FILES="  
    dev/android-database-sqlcipher/libs/commons-codec.jar
    dev/android-database-sqlcipher/libs/guava-r09.jar
    dev/android-database-sqlcipher/libs/sqlcipher.jar

    dev/android-database-sqlcipher/libs/armeabi/libdatabase_sqlcipher.so 
    dev/android-database-sqlcipher/libs/armeabi/libsqlcipher_android.so
    dev/android-database-sqlcipher/libs/armeabi/libdatabase_sqlcipher.so
    dev/android-database-sqlcipher/libs/armeabi/libsqlcipher_android.so
    dev/android-database-sqlcipher/libs/armeabi/libstlport_shared.so
    
    dev/android-database-sqlcipher/libs/armeabi-v7a/libdatabase_sqlcipher.so 
    dev/android-database-sqlcipher/libs/armeabi-v7a/libsqlcipher_android.so
    dev/android-database-sqlcipher/libs/armeabi-v7a/libdatabase_sqlcipher.so
    dev/android-database-sqlcipher/libs/armeabi-v7a/libsqlcipher_android.so
    dev/android-database-sqlcipher/libs/armeabi-v7a/libstlport_shared.so
    
    dev/android-database-sqlcipher/libs/x86/libdatabase_sqlcipher.so 
    dev/android-database-sqlcipher/libs/x86/libsqlcipher_android.so
    dev/android-database-sqlcipher/libs/x86/libdatabase_sqlcipher.so
    dev/android-database-sqlcipher/libs/x86/libsqlcipher_android.so
    dev/android-database-sqlcipher/libs/x86/libstlport_shared.so


    dev/android-database-sqlcipher/libs/armeabi/libdatabase_sqlcipher.so
    dev/android-database-sqlcipher/libs/armeabi/libsqlcipher_android.so
    dev/android-database-sqlcipher/libs/armeabi/libstlport_shared.so
    dev/android-database-sqlcipher/libs/commons-codec.jar
    dev/android-database-sqlcipher/libs/guava-r09.jar
    dev/android-database-sqlcipher/libs/sqlcipher.jar

    /usr/local/ssl/fips-2.0/bin/fips_standalone_sha1     
    /usr/local/ssl/fips-2.0/bin/fipsld
    /usr/local/ssl/fips-2.0/lib/fips_premain.c
    /usr/local/ssl/fips-2.0/lib/fips_premain.c.sha1
    /usr/local/ssl/fips-2.0/lib/fipscanister.o
    /usr/local/ssl/fips-2.0/lib/fipscanister.o.sha1

    dev/openssl-1.0.1f/libcrypto.a

    dev/FipsWrapper/bin/fipswrapper.jar

    "


totalFileCount=0
missingFileCount=0
allFilesPresent=1
for i in $MY_FILES; do
  #echo $i
  ((totalFileCount++))
  if [ ! -f $i ]; then
    echo "Release file is missing!: $i"
    allFilesPresent=0
    ((missingFileCount++))
  fi

done

if [ "$allFilesPresent" == 1 ]; then
  echo ""
  echo "*****************************************************************"
  echo "All $totalFileCount artifact files are present and accounted for!"
  echo "*****************************************************************"

else
  echo ""
  echo "!!!!!!!!!!!!!!!! There are $missingFileCount files missing (Out of a total of $totalFileCount) !!!!!!!!!!!!!!!!"
fi


