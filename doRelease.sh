ReleaseName="testReleaseName"
  read -p "Enter your release name (Ex: V1.1_06_13-2014): " ReleaseName



fipsDir="/usr/local/ssl/fips-2.0"			
premainC="/usr/local/ssl/fips-2.0/lib/fips_premain.c"
premainSha="/usr/local/ssl/fips-2.0/lib/fips_premain.c.sha1"
canisterO="/usr/local/ssl/fips-2.0/lib/fipscanister.o"
fanisterSha="/usr/local/ssl/fips-2.0/lib/fipscanister.o.sha1"
sslSrc="dev/openssl-1.0.1f/libcrypto.a"
sslDst="release/$ReleaseName/object/openssl"
sqlcipherBinSev="dev/android-database-sqlcipher/libs"
sqlcipherBinDst=release/$ReleaseName/object/android-database-sqlcipher


docDst="release/$ReleaseName/"

mkdir -p release
mkdir -p release/$ReleaseName
mkdir -p release/$ReleaseName/source
mkdir -p release/$ReleaseName/object
mkdir -p release/$ReleaseName/object/fipsWrapper
mkdir -p release/$ReleaseName/object/usr/local/ssl
mkdir -p release/$ReleaseName/doc
mkdir -p $sslDst

# copy object files
cp -r $fipsDir* release/$ReleaseName/object/usr/local/ssl
cp -f $sslSrc $sslDst 
cp -r $sqlcipherBinSev* $sqlcipherBinDst
cp -r "dev/FipsWrapper/bin/fipswrapper.jar" "release/$ReleaseName/object/fipsWrapper"


#copy doc files
cp -r doc* $docDst
cp -r *.txt* $docDst/doc
cp -r *.log* $docDst/doc

#copy source files
cp -r dev/SupplementalFiles* $docDst/source

#copy tewst files
cp -r test $docDst/test

#Remove lib build instructions so they dont confuse end app developers
rm -f release/$ReleaseName/doc/FcadsLibBuildInstructions.txt

#--------------------------------------------------
#build the FIPS140-2ObjectModuleRecord.txt file
#--------------------------------------------------

fipsCanisterSha="/usr/local/ssl/fips-2.0/lib/fipscanister.o.sha1"
docDst="release/$ReleaseName/doc"

# get filename of the fips object module source tar file
nameAndPath=$(ls dev/openssl-fips*.gz)
nameOnly=$(basename $nameAndPath)

# get sha1 of the fips object modu;e
fipsSha1=$(cat $fipsCanisterSha)


objectModelFileRecordName="$docDst/FIPS140-2ObjectModuleRecord.txt"



echo "1.	The $nameOnly file distribution file which was used as the basis for the production of the FIPS 
		object module was obtained from  the  FIPS compatible OpenSSL library from physical media (CD) 
		obtained directly from the OpenSSL foundation." > $objectModelFileRecordName
echo "2.	The host platform on which the fipscanister.o, fipscanister.o.sha1,fips_premain.c, 
		and fips_premain.c.sha1 files were generated is OX-X. The compiler used was gcc version 4.6." >> $objectModelFileRecordName
echo "3.	The fipscanister.o module was generated with exactly the three commands:
			./config
			make                      
			make install
		No other build-time options were specified." >> $objectModelFileRecordName
echo "4.	The HMAC SHA-1 digest of the produced fipscanister.o is: 
		$fipsSha1" >> $objectModelFileRecordName
echo "5.	The contents of the distribution file used to create 
		fipscanister.o was not manually modified in any way at any time during the build process." >> $objectModelFileRecordName

echo ""
echo "Release was created at: release/$ReleaseName"
echo ""

# while true; do
#   read -p "Install? " yn
#   case $yn in
#     [Yy]* ) 
#       echo "you chose yes"
#       make help;
#       ;;
    

#     [Yn]* ) 
#       echo "you chose no"

#       ;;
#     * ) echo "please choose";;
#   esac
# done