# Checks for missing downloaded files
#
# files are necessary from fips/openssl, and sqlcipher

fipsSslFiles="1"
sqlCipherFiles="1"
  echo ""


# fips/openssl files
sslFiles="dev/openssl-1.0.1f.tar.gz"
fipsFiles="dev/openssl-fips-ecp-2.0.2.tar.gz"
sqlFiles="dev/android-database-sqlcipher"


if [ ! -f "$sslFiles" ]; then
  echo "$sslFiles is missing!"
  fipsSslFiles=""
fi

if [ ! -f "$fipsFiles" ]; then
  echo "$fipsFiles is missing!"
  fipsSslFiles=""
fi

# note we're just checking for the top level directory here - there should be LOTS of files under it
if [ ! -d "$sqlFiles" ]; then
  echo "$sqlFiles is missing!"
    sqlCipherFiles=""
fi


if [ ! -n "$fipsSslFiles" ]; then 

  echo ""
  echo "!!!! One or more of the fips download files are missing !!!!!"
  echo "" 
  echo "You have two options:"

  echo "Option 1 - get the official disk and copy $sslFiles, and fipsFiles to the dev directory"
  echo "Option 2 - copy the files from the local distribution (use command: make copyLocalFipsFiles)"
  echo ""



else
  if [ ! -n "$sqlCipherFiles" ]; then 
  echo ""
  echo "!!! TThe sqlCipher download files are missing !!!!!"
  echo "" 
  echo "You have three options:"
  echo "1 - git the official build from git"
  echo "    Clone sqlcipher source files from Github - into fcads/dev/"
  echo "     git clone git://github.com/sqlcipher/android-database-sqlcipher.git"
  echo "      make init (Downloads all dependencies - LOTS OF FILES ~ 3 gB)"

  echo "2 - Copy canned files from enclave at:"
  echo "       /Volumes/shares/Groups/Tech Team/Current Projects/Android-FIPS/resources/android-database-sqlcipher"
  echo "       to fcads/dev/"
  echo "       (Takes several minutes)"
  echo ""

  echo "3 - Copy canned files from local hard disk at:"
  echo "       ~/release/sqlCipherDownload_6-12_14/android-database-sqlcipher"
  echo "       to fcads/dev/"
  echo "      Use command: make copyLocalSqlCipherFiles"
  echo "      (Takes a minute or so "
  echo ""


  else
  echo ""
	echo "*************************************************"
	echo "All download files are present and accounted for!"
  echo " Use command make buildAll"
	echo "*************************************************"

    read -p "press y to continue build " yn
    case $yn in
      [Yy]* ) 

        echo "----------------------------------------------------------------------------------------------------------"
        echo "Preparing files"
        echo "----------------------------------------------------------------------------------------------------------"
        cd dev/android-database-sqlcipher
        android update project -p . --target 1
        make prepare
        cd ../..
        echo "----------------------------------------------------------------------------------------------------------"
        echo "Compiling FIPS Module"
        echo "----------------------------------------------------------------------------------------------------------"
        make fips
        echo "----------------------------------------------------------------------------------------------------------"
        echo "Compiling ssl files"
        echo "----------------------------------------------------------------------------------------------------------"
        make ssl
        echo "----------------------------------------------------------------------------------------------------------"
        echo "Compiling sqlcipher files"
        echo "----------------------------------------------------------------------------------------------------------"
        make sqlcipher
        echo "----------------------------------------------------------------------------------------------------------"
        echo "Compiling t2 files"
        echo "----------------------------------------------------------------------------------------------------------"
        make t2
        echo "----------------------------------------------------------------------------------------------------------"
        echo "Checking for artifacts produced"
        echo "----------------------------------------------------------------------------------------------------------"
        echo ""
        make check

        ;;
      

      [Yn]* ) 
        echo "you chose no"

        ;;
      * ) echo "please choose";;
    esac






  fi
fi
  echo ""



