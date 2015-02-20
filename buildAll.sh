# Checks for missing downloaded files
#
# files are necessary from fips/openssl, and sqlcipher

fipsSslFiles="1"
sqlCipherFiles="1"
  echo ""

# Checks for the existance of opssl/fips files and offers to copy them from local resources 
# sets abortBuild = 1 if there are missing files and user chooses to abort build
checkforFipsFiles()
{
  if [ ! -e "$sslFiles" ] ||  [ ! -e "$fipsFiles" ]; then 

      echo ""
      echo "!!!! One or more of the fips download files are missing !!!!!"
      echo "" 
      echo "sslFiles = $sslFiles"
      echo "fipsFiles = $fipsFiles"
      echo ""

      echo "You have two options:"

      echo "Option y - Copy local (unofficial) openssl downlad files form this distribution"
      echo "Option n - Stop build so you can download the official files"
    echo ""

      read -p "press y or n : " yn
      case $yn in
        [Yy]* ) 
      echo "Copying local files"
      cp localFipsSslFiles/*.gz ./dev
        ;;
        [Yn]* ) 
          echo "you chose no"
          abortBuild=1
          ;;
        * ) echo "please choose";;
      esac

  fi
}

# Checks for the existance of sqlcipher files and offers to copy them from local resources 
# sets abortBuild = 1 if there are missing files and user chooses to abort build
checkForSqlcipherFiels()
{
  if [ ! -d "$sqlcipherFiles" ]; then 


      echo ""
      echo "!!!! One or more of the sqlCipher download files are missing !!!!!"
      echo "" 
      echo "sqlcipherFiles = $sqlcipherFiles"
      echo ""
      echo "You have two options:"
      echo "Option y - Copy local (unofficial) sqlCipher downlad files form this distribution"
      echo "Option n - Stop build so you can download the official files"
      echo ""

      read -p "press y or n : " yn
      case $yn in
        [Yy]* ) 
      echo "Copying local files"
      cp -r localFipsSslFiles/android-database-sqlcipher dev/android-database-sqlcipher && \
      cd dev/android-database-sqlcipher
        # Make sure cqlcipher files are RW
      chmod -R 777 ./
      cd ../..
      

        ;;
        [Yn]* ) 
          echo "you chose no"
          abortBuild=1
          ;;
        * ) echo "please choose";;
      esac

  fi
}

# This sets up environment vars to point to which openssl/fips we want to use
. ./setEnvOpensslFiles.sh



echo "using $OPENSSL_BASE.tar.gz"
echo "using $FIPS_BASE.tar.gz"

# fips/openssl files
sslFiles="dev/$OPENSSL_BASE.tar.gz"
fipsFiles="dev/$FIPS_BASE.tar.gz"


echo $sslFiles
echo $fipsFiles

# sqlcipher files

sqlcipherFiles="dev/android-database-sqlcipher"

# Checkfor existance of necessary files
checkforFipsFiles
checkForSqlcipherFiels 

if [ "$abortBuild" == 1 ]; then
  echo "******** Aborting build ************"

else
    echo ""
    echo "--- All files present and accounted for, proceeding with build ---"
    echo ""
    echo "----------------------------------------------------------------------------------------------------------"
    echo "Preparing files"
    echo "----------------------------------------------------------------------------------------------------------"
   pwd
    cd dev/android-database-sqlcipher
       pwd
    android update project -p . --target 1
    cd ../..
    make prepare
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
   echo "" 


 fi













# if [ ! -n "$fipsSslFiles" ]; then 

#   echo ""
#   echo "!!!! One or more of the fips download files are missing !!!!!"
#   echo "" 
#   echo "You have two options:"

#   echo "Option 1 - get the official disk and copy $sslFiles, and fipsFiles to the dev directory"
#   echo "Option 2 - copy the files from the local distribution (use command: make copyLocalFipsFiles)"
#   echo ""



# else
#   if [ ! -n "$sqlCipherFiles" ]; then 
#   echo ""
#   echo "!!! TThe sqlCipher download files are missing !!!!!"
#   echo "" 
#   echo "You have three options:"
#   echo "1 - git the official build from git"
#   echo "    Clone sqlcipher source files from Github - into fcads/dev/"
#   echo "     git clone git://github.com/sqlcipher/android-database-sqlcipher.git"
#   echo "      make init (Downloads all dependencies - LOTS OF FILES ~ 3 gB)"

#   echo "2 - Copy canned files from enclave at:"
#   echo "       /Volumes/shares/Groups/Tech Team/Current Projects/FIPS/resources/android-database-sqlcipher"
#   echo "       to fcads/dev/"
#   echo "       cp -r \"/Volumes/shares/Groups/Tech Team/Current Projects/FIPS/resources/android-database-sqlcipher\" ./dev"
#   echo "       (Takes several minutes)"
#   echo ""

#   echo "3 - Copy canned files from local hard disk at:"
#   echo "       ~/release/sqlCipherDownload_6-12_14/android-database-sqlcipher"
#   echo "       to fcads/dev/"
#   echo "      Use command: make copyLocalSqlCipherFiles"
#   echo "      (Takes a minute or so "
#   echo ""


#   else
#   echo ""
# 	echo "*************************************************"
# 	echo "All download files are present and accounted for!"
#   echo " Use command make buildAll"
# 	echo "*************************************************"

#     read -p "press y to continue build " yn
#     case $yn in
#       [Yy]* ) 

#         echo "----------------------------------------------------------------------------------------------------------"
#         echo "Preparing files"
#         echo "----------------------------------------------------------------------------------------------------------"
#         cd dev/android-database-sqlcipher
#         android update project -p . --target 1
#         cd ../..
#         make prepare
#         echo "----------------------------------------------------------------------------------------------------------"
#         echo "Compiling FIPS Module"
#         echo "----------------------------------------------------------------------------------------------------------"
#         make fips
#         echo "----------------------------------------------------------------------------------------------------------"
#         echo "Compiling ssl files"
#         echo "----------------------------------------------------------------------------------------------------------"
#         make ssl
#         echo "----------------------------------------------------------------------------------------------------------"
#         echo "Compiling sqlcipher files"
#         echo "----------------------------------------------------------------------------------------------------------"
#         make sqlcipher
#         echo "----------------------------------------------------------------------------------------------------------"
#         echo "Compiling t2 files"
#         echo "----------------------------------------------------------------------------------------------------------"
#         make t2
#         echo "----------------------------------------------------------------------------------------------------------"
#         echo "Checking for artifacts produced"
#         echo "----------------------------------------------------------------------------------------------------------"
#         echo ""
#         make check

#         ;;
      

#       [Yn]* ) 
#         echo "you chose no"

#         ;;
#       * ) echo "please choose";;
#     esac






#   fi
# fi
#   echo ""



