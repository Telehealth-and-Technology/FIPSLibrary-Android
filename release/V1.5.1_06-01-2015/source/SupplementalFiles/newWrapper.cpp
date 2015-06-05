#include <string.h>
#include <jni.h>
#include <openssl/des.h>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <android/log.h>
#include <openssl/crypto/rand/rand.h>


// t2Crypto functionality (Lost password functionality)

// (A) Random Intermediate Key (RIKey) is created and database is initialized to use this key.
// (B) Locking  Key is derived from the PIN using KDF algorithm. Master Key is created by encrypting  
//     the RIKey with the Locking Key. Master Key is saved to device storage.
// (D) Secondary Locking key is derived from the Security question answers using KDF algorithm. 
//     Backup Key is created by encrypting  the RIKey with the Locking Key. Backup Key is saved to device storage.
// (F) Locking Key is derived from the PIN.
// (G) RIKey is re-constructed by decrypting the Master Key using the Locking Key.
//     If the RIKey matches the key that the database was initialized with then Login/Data access is successful.
// (H) Secondary Locking Key is derived from the Security Question Answers.
// (G) RIKey is re-constructed by decrypting the Backup Key using the Secondary Locking Key.
//     If the RIKey matches the key that the database was initialized with then Login/Data access is successful.


//    RIKey is never saved anywhere. It is generated at initialization and used. 
//    When it is needed again it is derived from either the Master Key and PIN, or 
//    the Backup Key and Sequrity Question Answers.

//    Security Questions stored statically unencrypted 
//    Answers are not stored direcly but via encrypted Backup Key



// JNI Stuff
//     Assumptions, The project includes:
//         1. A JAVA class FipsWrapper.java in package com.t2.fcads
//         2. A native file called wrapper.c

//     To add a static java method that a native routine can call
//         1. Add method to class com.t2.fcads 
//             take note of it's signature, it will be used indirectly in the native code
//             in the method call GetStaticMethodId.
//         2. In the native code call like this:
//             jclass cls = env->FindClass( "com/t2/fcads/FipsWrapper");
//             jmethodID id = env->GetStaticMethodID( cls, "clearAllData", "()V");
//             env->CallStaticVoidMethod( cls, id);

//         2a. Note in this case that GetStaticMethodID is looking for the method called
//            clearAllData that returns void (The V after parentheses)
//            and is send no parameters (empty parentheses).

//            Here are a couple of examples of how signatures translate to the codes used in GetStaticMethodID.
//             "(Ljava/lang/String;Ljava/lang/String;)V"   ->> void putString(String key, String value)
//             "(Ljava/lang/String;)Ljava/lang/String;"    ->> String getString(String key)
//             "(Ljava/lang/String;[B)V"                   ->> void putData(String key, byte[] values)
//             "(Ljava/lang/String;)[B"                    ->> byte[] getData(String key)

//     To add a native method callable from JAVA
//         1. Add method to native file, ex:
//             jstring Java_com_t2_fcads_FipsWrapper_example(JNIEnv* env, jobject thiz, jstring jPin) {
//                 const char *pin= env->GetStringUTFChars( jPin, 0);
//                 env->ReleaseStringUTFChars( jPin, pin);
//                 return env->NewStringUTF( (char*)formattedKey);
//             }        
//         1a. Note that the method must be named according to the class and package name that will be calling it:
//                 JAVA_{package name}_{class name}_{method name}
//             in the above: 
//                 package = com_t2_fcads
//                 class = FipsWrapper
//                 method = example
//         1b. It is important that you use env->Getxx() and env->Release() before using variables
//             and that you only return environment variables ex:
//                 return env->NewStringUTF( (char*)formattedKey);
//         2. In the project class that will be calling the native method add a declaration of the method
//             public native String example(String pin);

//             You can then call the native method from that JAVA class, ex:
//                 String result = example(new String("string to process"));


extern "C" {

// ----------------- Constants -----------------------
#define APPNAME "wrapper"

#define ENCRYPT_PREFERENCES

#define boolean int
#define TRUE 1
#define FALSE 0
#define SIGNATURE_SIZE 20
#define GENERIC_BUFFER_SIZE 1024

// #define CANNED_RI_KEY_BYTES_LENGTH strlen(CANNED_RI_KEY_BYTES)
// #define CANNNED_SALT {0x39, 0x30, 0x00, 0x00, 0x31, 0xD4, 0x00, 0x00}
// #define CANNED_RI_KEY_BYTES "password"


// Note: this next set fails when using the wrong malloc routines
#define CANNED_RI_KEY_BYTES_LENGTH MAX_KEY_LENGTH
#define CANNNED_SALT {0x93, 0x0e, 0x4b, 0x4f, 0x72, 0x62, 0xaf, 0x75} 
#define CANNED_RI_KEY_BYTES {0x57, 0x1b, 0x2d, 0x38, 0x26, 0x52, 0xdd, 0x42, 0xfe, 0x15, 0x5f, 0xbf, 0x1f, 0x8d, 0x2c, 0x46, 0xc7, 0xc0, 0x67, 0xdb, 0x29, 0xed, 0xa3, 0x01, 0x55, 0x4e, 0x7f, 0x0c, 0x35, 0x57, 0x0e, 0x87} 

#define MAX_KEY_LENGTH 32
#define SALT_LENGTH 8

#define EVP_aes_256_cbc_Key_LENGTH 32
#define EVP_aes_256_cbc_Iv_LENGTH 16

#define KEY_MASTER_KEY "MasterKey"
#define KEY_BACKUP_KEY "BackupKey"
#define KEY_DATABASE_PIN "KEY_DATABASE_PIN"
#define KEY_DATABASE_CHECK "KEY_DATABASE_CHECK"

#define KCHECK_VALUE "check"
const char KEY_SALT[] = "KEY_SALT";

const char emptyString[] = {0};

#define PREFERENCES_SALT {0x39, 0x30, 0x00, 0x00, 0x31, 0xD4, 0x00, 0x00}
#define PREFERENCES_PASSWORD "preferencespassword"

int const T2Error = -1;
int const T2Success = 0;

int const T2True = 1;
int const T2False = 0;

int const OpenSSLError = 0;
int const OpenSSLSuccess = 1;

// ----------------- Data Structures -----------------------
/*!
 * @typedef T2Key
 * @discussion Structure containing elements necessary for an encryption key
 *              "opaque" encryption, decryption ctx structures that libcrypto
 *              uses to record status of enc/dec operations
 */
typedef struct {
    EVP_CIPHER_CTX encryptContext;
    EVP_CIPHER_CTX decryptContext;
    int ivLength;
    int keyLength;
    unsigned char key[MAX_KEY_LENGTH], iv[MAX_KEY_LENGTH];
} T2Key;

// ----------------- External variables -----------------------
extern const void*          FIPS_text_start(),  *FIPS_text_end();
extern const unsigned char  FIPS_rodata_start[], FIPS_rodata_end[];
extern unsigned char        FIPS_signature[SIGNATURE_SIZE];
extern unsigned int         FIPS_incore_fingerprint (unsigned char *, unsigned int);

// ----------------- Local variables -----------------------
static unsigned char        Calculated_signature[SIGNATURE_SIZE];
unsigned char *_initializedPin;
unsigned char * formattedKey[256];
unsigned char * genericBuffer[GENERIC_BUFFER_SIZE];
unsigned char mainSalt[SALT_LENGTH];
unsigned char *_salt = &mainSalt[0];
unsigned char *preferencesSalt;
boolean useTestVectors;
boolean verboseLogging;

T2Key preferencesKey; // Key used to encrypt user preferences preferences - putData() and putString()



// -------------------- Function prototypes -----------------------
//   ======= External FIPS methods =================
jint Java_com_t2_fcads_FipsWrapper_FIPSmode( JNIEnv* env, jobject thiz );
jstring Java_com_t2_fcads_FipsWrapper_T2FIPSVersion( JNIEnv* env, jobject thiz );

//   ======= External T2Crypto methods =================
jint Java_com_t2_fcads_FipsWrapper_checkPin( JNIEnv* env,jobject thiz, jstring jPin);
jint Java_com_t2_fcads_FipsWrapper_checkAnswers( JNIEnv* env,jobject thiz, jstring jAnswers);
void Java_com_t2_fcads_FipsWrapper_init(JNIEnv* env,jobject thiz);
void Java_com_t2_fcads_FipsWrapper_setVerboseLogging(JNIEnv* env,jobject thiz, jboolean jvl);
void Java_com_t2_fcads_FipsWrapper_prepare( JNIEnv* env,jobject thiz, jboolean testVectors);
jint Java_com_t2_fcads_FipsWrapper_initializeLogin( JNIEnv* env,jobject thiz, jstring jPin,jstring jAnswers);
jstring Java_com_t2_fcads_FipsWrapper_getDatabaseKeyUsingPin(JNIEnv* env, jobject thiz, jstring jPin); 
jint Java_com_t2_fcads_FipsWrapper_isInitialized( JNIEnv* env,jobject thiz );
void Java_com_t2_fcads_FipsWrapper_deInitializeLogin( JNIEnv* env,jobject thiz );
void Java_com_t2_fcads_FipsWrapper_cleanup(JNIEnv* env,jobject thiz);
jint Java_com_t2_fcads_FipsWrapper_changePinUsingPin( JNIEnv* env,jobject thiz,jstring jOldPin,jstring jNewPin);
jint Java_com_t2_fcads_FipsWrapper_changePinUsingAnswers( JNIEnv* env,jobject thiz, jstring jNewPin,jstring jAnswers);


//   ======= Internal methods =================
int checkPin_I(JNIEnv* env, unsigned char *pin);
int checkAnswers_I(JNIEnv* env, unsigned char *answers);
void generateMasterOrRemoteKeyAndSave(JNIEnv* env, T2Key * RIKey, T2Key * LockingKey, const char * keyType);
char * generateMasterOrRemoteKey_malloc(JNIEnv* env, T2Key * RIKey, T2Key * LockingKey, const char * keyType);
unsigned char * encryptStringUsingKey_malloc(T2Key * credentials, unsigned char * pUencryptedText, int *outLength);
unsigned char * encryptBinaryUsingKey_malloc1(T2Key * credentials, unsigned char * pUencryptedText, int inLength, int * outLength);
unsigned char * decryptUsingKey_malloc1(T2Key * credentials, unsigned char * encryptedText, int * inLength);
int key_init(unsigned char * key_data, int key_data_len, unsigned char * salt, T2Key * aCredentials);
int getRIKeyUsing(JNIEnv* env, T2Key * RiKey, char * answersOrPin, char * keyType);
unsigned char * aes_decrypt_malloc(EVP_CIPHER_CTX * decryptContext, unsigned char * ciphertext, int *len);
unsigned char * aes_encrypt_malloc(EVP_CIPHER_CTX * encryptContext , unsigned char * plaintext, int * len);


void logAsHexString(unsigned char * bin, unsigned int binsz, char * message);

unsigned char * binAsHexString_malloc(unsigned char * bin, unsigned int binsz);
unsigned char * hexStringAsBin_malloc(unsigned char * hex, int *stringLength);
int MY_FIPS_mode();

// -------------------- Macros -----------------------

// Macro to throw java exction from "c" code
#define THROW_T2_EXCEPTION(_LABEL_) \
    jclass cls = env->FindClass("java/lang/RuntimeException");  \
    if (cls == 0) {                                                     \
      env->ThrowNew(cls, "Cannot Find RuntimeException class"); \
    } else {                                                            \
      env->ThrowNew(cls, _LABEL_);                              \
    }                                                                   \
                                


#define T2Assert(_EXPRESSION_, _TEXT_)                                 \
   if(!(_EXPRESSION_))    {                                            \
      THROW_T2_EXCEPTION(_TEXT_);                                      \
} 

//#define LOGI(...)  if (verboseLogging) ((void)__android_log_print(ANDROID_LOG_INFO, "native-activity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "native-activity", __VA_ARGS__))



#define LOGI(...) \
    do { \
    if (verboseLogging) ((void)__android_log_print(ANDROID_LOG_INFO, "native-activity", __VA_ARGS__));  \
    } while (0);
  

#define LOGE(...) \
    do { \
    ((void)__android_log_print(ANDROID_LOG_ERROR, "native-activity", __VA_ARGS__));  \
    } while (0);
  

// --------------------------------------------------------------
// ------------ Native methods to facilitate data storage
// --------------------------------------------------------------

/*!
 * @brief Saves a string to Android sharedPreferences via Java call;
 * @discussion Corresponding java method is static void com.t2.fcads.putString(String key, String value) 
 * @param env Jni environment
 * @param pKey Key to save under
 * @param pValue Value to save
 */
void putString(JNIEnv* env, const char *pKey, const char *pValue) {
  jclass cls = env->FindClass("com/t2/fcads/FipsWrapper");
  if (cls == 0) {
      T2Assert((cls != 0), "Can't find class FipsWrapper");
      return;
  }

  jmethodID id = env->GetStaticMethodID( cls, "putString", "(Ljava/lang/String;Ljava/lang/String;)V");
  if (id == 0) {
    T2Assert((cls != 0), "Can't find method putString");
    return;
  }

  jstring key = env->NewStringUTF( pKey);
  jstring value = env->NewStringUTF( pValue);

  env->CallStaticVoidMethod( cls, id, key, value);
} 

/*!
 * @brief Retrieves a string from Android sharedPReferences via Java call;
 * @discussion Corresponding java method is static String com.t2.fcads.getString(String key) 
 * @param env Jni environment
 * @param pKey Key Key of variable to retrieve
 * @return  String value associated with key
 */
const char * getString(JNIEnv* env, const char *pKey) {
  jstring key = env->NewStringUTF( pKey);

  jclass cls = env->FindClass( "com/t2/fcads/FipsWrapper");
  if (cls == 0) {
      T2Assert((cls != 0), "Can't find class FipsWrapper");
      return emptyString;
  }

  jmethodID id = env->GetStaticMethodID( cls, "getString", "(Ljava/lang/String;)Ljava/lang/String;");
  if (id == 0) {
    T2Assert((cls != 0), "Can't find method getString");
    return emptyString;
  }

  jstring returnedString = (jstring) env->CallStaticObjectMethod( cls, id, key);

  const char *utfChars = env->GetStringUTFChars( returnedString, 0);

  return utfChars;

} 


/*!
 * @brief Saves a bytes to Android sharedPreferences via Java call;
 * @discussion Corresponding java method is static void com.t2.fcads.putData(String key, byte[] value) 
 * @param env Jni environment
 * @param pKey Key to save under
 * @param pValue bytes to save
 */
void putData(JNIEnv* env, const char *pKey, const char *pValue, int data_size ) {
  jclass cls = env->FindClass("com/t2/fcads/FipsWrapper");
  if (cls == 0) {
      T2Assert((cls != 0), "Can't find class FipsWrapper");
      return;
  }

  jmethodID id = env->GetStaticMethodID(cls, "putData", "(Ljava/lang/String;[B)V");
  if (id == 0) {
    T2Assert((cls != 0), "Can't find method putData");
    return;
  }

  // // Test hex conversion
  // unsigned char testBinary[] = {0,1,2,3,4,5,6,7,8,9,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10};
  // logAsHexString((unsigned char *)testBinary, 17, "  today  binary ");
  // char *hex =  binAsHexString_malloc(testBinary, 17);
  // if (hex != NULL) {
  //   logAsHexString((unsigned char *)hex, strlen(hex), "  today  hex ");

  //   int resultLength = strlen(hex);
  //   LOGI("resultLength = %d", resultLength);
  //   unsigned char *resultBinary = hexStringAsBin_malloc(hex, &resultLength);
  //   if (resultBinary != NULL) {
  //     logAsHexString((unsigned char *)resultBinary, resultLength, "  recovered binary ");
  //     free(resultBinary);
  //   }
  //   free(hex);
  // }





  #ifdef ENCRYPT_PREFERENCES

    // Need to encrypt the key string before saving
    int encryptedKeyLength;
    unsigned char *encryptedKey = encryptBinaryUsingKey_malloc1(&preferencesKey, (unsigned char *) pKey, strlen(pKey), &encryptedKeyLength);
    T2Assert((encryptedKey != NULL), "Memory allocation error");
 
    // the encrypted key is BINARY - we need to make a string out of it, this is a cheap way
    char *encryptedKeyHex =  (char*)binAsHexString_malloc(encryptedKey, encryptedKeyLength);
    T2Assert((encryptedKeyHex != NULL), "Memory allocation error");


    // Need to encrypt the data before saving
    int outLength;
    unsigned char *encryptedData = encryptBinaryUsingKey_malloc1(&preferencesKey, (unsigned char *) pValue, data_size, &outLength);
    T2Assert((encryptedData != NULL), "Memory allocation error");

      // Now convert to jni variables and proceed
    jstring key = env->NewStringUTF( encryptedKeyHex);
//    jstring key = env->NewStringUTF(env, pKey);
    jbyteArray retArray = env->NewByteArray( outLength);
    void *temp = env->GetPrimitiveArrayCritical((jarray)retArray, 0);
    memcpy(temp, encryptedData, outLength);

    free(encryptedData);
    free(encryptedKey);
    free(encryptedKeyHex);

  #else

    // Now convert to jni variables and proceed
    jstring key = env->NewStringUTF( pKey);
    jbyteArray retArray = env->NewByteArray(data_size);
    void *temp = env->GetPrimitiveArrayCritical((jarray)retArray, 0);
    memcpy(temp, pValue, data_size);

  #endif


  env->ReleasePrimitiveArrayCritical(retArray, temp, 0);
  env->CallStaticVoidMethod(cls, id, key, retArray);

} 


/*!
 * @brief Retrieves bytes from Android sharedPReferences via Java call;
 * @discussion Corresponding java method is static String com.t2.fcads.getBytes(String key) 
 * @param env Jni environment
 * @param pKey Key Key of bytes to retrieve
 * @return  Bytes associated with key
 */
char * getData_malloc(JNIEnv* env, const char *pKey, int * length) {
  jclass cls = env->FindClass("com/t2/fcads/FipsWrapper");
  if (cls == 0) {
      T2Assert((cls != 0), "Can't find class FipsWrapper");
      return NULL;
  }

  jmethodID id = env->GetStaticMethodID(cls, "getData", "(Ljava/lang/String;)[B");
  if (id == 0) {
    T2Assert((cls != 0), "Can't find method getData");
    return NULL;
  }


    // Need to encrypt the key string before saving
    int encryptedKeyLength;
    unsigned char *encryptedKey = encryptBinaryUsingKey_malloc1(&preferencesKey, (unsigned char *) pKey, strlen(pKey), &encryptedKeyLength);
    T2Assert((encryptedKey != NULL), "Memory allocation error");
 
    // the encrypted key is BINARY - we need to make a string out of it, this is a cheap way
    char *encryptedKeyHex =  (char*)binAsHexString_malloc(encryptedKey, encryptedKeyLength);
    T2Assert((encryptedKeyHex != NULL), "Memory allocation error");

    jstring key = env->NewStringUTF(encryptedKeyHex);
  //jstring key = (*env)->NewStringUTF(env, pKey);


  jbyteArray ar = (jbyteArray) env->CallStaticObjectMethod(cls, id, key);
  jboolean isCopy;
  signed char *returnedElements = env->GetByteArrayElements(ar, &isCopy);
  *length = env->GetArrayLength(ar);

  unsigned char *returnedValue  = (unsigned char*) malloc(sizeof(char) * *length);
  T2Assert(returnedValue != NULL , "ERROR: memory allocation");

  memcpy(returnedValue, returnedElements, *length);


  env->ReleaseByteArrayElements(ar, returnedElements, JNI_ABORT);

  #ifdef ENCRYPT_PREFERENCES 
    free(encryptedKey);
    free(encryptedKeyHex);

    // Need to decrypt the data before saving
    unsigned char *decryptedData = decryptUsingKey_malloc1(&preferencesKey, returnedValue, length);
    T2Assert((decryptedData != NULL), "Memory allocation error");
    free(returnedValue);
    return (char*) decryptedData;

  #else
    return returnedValue;    
  #endif

}


/*!
 * @brief Clears all data from NVM (preferences storage)
 */
void clearAllData(JNIEnv* env) {
  jclass cls = env->FindClass("com/t2/fcads/FipsWrapper");
  if (cls == 0) {
      T2Assert((cls != 0), "Can't find class FipsWrapper");
      return;
  }

  jmethodID id = env->GetStaticMethodID(cls, "clearAllData", "()V");
  if (id == 0) {
    T2Assert((cls != 0), "Can't find method clearAllData");
    return;
  }

  env->CallStaticVoidMethod(cls, id);
} 



/*!
 * @brief Retrieves salt based on Android package signature
 * @discussion Note: sets passed length to the length of the returned character array
 * @param env Jni environment
 * @param length Pointer to length variable
 * @return Salt character array
 */
char * getPackageBasedSalt_malloc(JNIEnv* env, int * length) {
  jclass cls = env->FindClass("com/t2/fcads/FipsWrapper");
  if (cls == 0) {
      T2Assert((cls != 0), "Can't find class FipsWrapper");
      return NULL;
  }

  jmethodID id = env->GetStaticMethodID(cls, "getPackageBasedSalt", "()[B");
  if (id == 0) {
    T2Assert((cls != 0), "Can't find method clearAllData");
    return NULL;
  }

  jbyteArray ar = (jbyteArray) env->CallStaticObjectMethod(cls, id);
  jboolean isCopy;
  signed char *returnedElements = env->GetByteArrayElements(ar, &isCopy);
  *length = env->GetArrayLength(ar);

  char *returnedValue  = (char*) malloc(sizeof(char) * *length);
  T2Assert(returnedValue != NULL , "ERROR: memory allocation");

  memcpy(returnedValue, returnedElements, *length);
  env->ReleaseByteArrayElements(ar, returnedElements, JNI_ABORT);
  return returnedValue;
}

// --------------------------------------------------------------
// ------------ t2crypto methods
// --------------------------------------------------------------

/*!
 * @brief Sets verbose logging mode
 * @discussion 
 * @param env Jni environment
 * @param thiz Passed jni object
 * @param jvl Parameter telling too enable or not
 */
void Java_com_t2_fcads_FipsWrapper_setVerboseLogging(JNIEnv* env,jobject thiz, jboolean jvl) {
  int vl = (boolean) jvl;
  verboseLogging = vl;
}

// MUST be the last thing called
/*!
 * @brief Cleans up any leftover resources and memory
 * @param env Jni environment
 * @param thiz Passed jni object
 */
void Java_com_t2_fcads_FipsWrapper_cleanup(JNIEnv* env,jobject thiz) {
  EVP_CIPHER_CTX_cleanup(&preferencesKey.encryptContext);
  EVP_CIPHER_CTX_cleanup(&preferencesKey.decryptContext);

  if (preferencesSalt != NULL) {
    free(preferencesSalt);
  }
}



// MUST be called before anything else!
/*!
 * @brief Initializes FIPS wrapper
 * @param env Jni environment
 * @param thiz Passed jni object
 */
void Java_com_t2_fcads_FipsWrapper_init(JNIEnv* env,jobject thiz) {

  LOGI("INFO: Initializing");

  // Now set up key used for saving preference data 
  char *RIKeyBytes;
  int RIKeyBytesLen;
  RIKeyBytes = (char *) malloc(sizeof(char) * strlen(PREFERENCES_PASSWORD));
  T2Assert((RIKeyBytes != NULL), "Memory allocation error for RIKey!");
      
  preferencesSalt = (unsigned char*) malloc(sizeof(unsigned char) * SALT_LENGTH);
  T2Assert(preferencesSalt != NULL, "Memory allocation error - Memory not allocated")

  unsigned char tmp[] = PREFERENCES_SALT;
  memcpy(preferencesSalt,tmp,SALT_LENGTH);
      
  char tmp1[] = PREFERENCES_PASSWORD;
  strcpy(RIKeyBytes, tmp1);
  RIKeyBytesLen = strlen(RIKeyBytes);


  LOGI("INFO: *** Generating preferencesKey ***");
  if ((key_init((unsigned char*) RIKeyBytes, RIKeyBytesLen, (unsigned char *) preferencesSalt, &preferencesKey))) {
      free(RIKeyBytes);
      T2Assert(0, "ERROR: initializing preferences key");
      return;
  }
  free(RIKeyBytes);
}



/*!
 * @brief Tells t2Cyrpto whether to use test vectors or random bytes for key generation
 * @param env Jni environment
 * @param thiz Passed jni object
 * @param textVectors - Boolean telling whether to use test vectors or random bytes for key generation
 * @discussion This may also be called to change use of test vectors or not
  */
void Java_com_t2_fcads_FipsWrapper_prepare( JNIEnv* env,jobject thiz, jboolean testVectors) {

    useTestVectors = (boolean) testVectors;

    if (useTestVectors) {
        LOGI("Java_com_t2_fcads_FipsWrapper_prepare, useTestVectors  = TRUE"); 
    }
    else {
       LOGI("Java_com_t2_fcads_FipsWrapper_prepare, useTestVectors  = FALSE"); 
    }
}

/*!
 * @brief Clears all login data
 * @param env Jni environment
 * @param thiz Passed jni object
 */
void Java_com_t2_fcads_FipsWrapper_deInitializeLogin( JNIEnv* env,jobject thiz ) {

  LOGI("Java_com_t2_fcads_FipsWrapper_deInitializeLogin");
  clearAllData(env);

}



/*!
 * @brief Changes to a new PIN using the previous PIN to authenticate
 * @discussion Generates and saves new masterKey
 * @param env Jni environment
 * @param thiz Passed jni object
 * @param jOldPin Previous Pin
 * @param jNewPin New Pin
 * @return T2Success or T2Error
 */
jint Java_com_t2_fcads_FipsWrapper_changePinUsingPin( JNIEnv* env,jobject thiz, 
          jstring jOldPin,
          jstring jNewPin
           ) {
    
    T2Key LockingKey;
    T2Key acredential;
    T2Key *rIKey_1 = &acredential;
    


    LOGI("Java_com_t2_fcads_FipsWrapper_changePinUsingPin");
    if (!Java_com_t2_fcads_FipsWrapper_isInitialized(env, thiz)) {
      return T2Error;
    }
    
      // Retrieve jni variables
    const char *oldPin= env->GetStringUTFChars(jOldPin, 0);
    const char *newPin = env->GetStringUTFChars(jNewPin, 0);

    LOGI("INFO: === changePinUsingPin ===");
    
    int result = checkPin_I(env, (unsigned char*) oldPin);
    if (result != T2Success) {
        return T2Error;
    }
    
    // Generate the RIKey based on oldPin and Master Key
    result = getRIKeyUsing(env, rIKey_1, (char*) oldPin, (char *) KEY_MASTER_KEY);
    if (result != T2Success) {
      return result;
    }
    
    // Generate LockingKey = kdf(newPin)
    // ------------------------------
    LOGI("INFO: *** Generating LockingKey  kdf(%s) ***", newPin);
    
    {
        unsigned char *key_data = (unsigned char *)newPin;
        int key_data_len = strlen(newPin);
        
        /* gen key and iv. init the cipher ctx object */
        if (key_init(key_data, key_data_len, (unsigned char *)_salt, &LockingKey)) {
            T2Assert(FALSE, "ERROR: initalizing key");
            return T2Error;
        }
    }
    
    // Generate MasterKey = encrypt(RI Key, LockingKey)
    // ------------------------------
    LOGI("INFO: *** Generating and saving to NVM MasterKey ***");
    generateMasterOrRemoteKeyAndSave(env, rIKey_1, &LockingKey, KEY_MASTER_KEY);
    
    _initializedPin = (unsigned char*) newPin;

    int outLength;
    unsigned char *encryptedPin = encryptStringUsingKey_malloc(rIKey_1, (unsigned char *) newPin, &outLength);
    T2Assert((encryptedPin != NULL), "Memory allocation error");

    LOGI("outLength = %d", outLength);
     logAsHexString(encryptedPin, outLength, (char*) "    Encrypted pin ");

    putData(env, (char const *)KEY_DATABASE_PIN, (char const *) encryptedPin, outLength );
    free(encryptedPin);


    EVP_CIPHER_CTX_cleanup(&rIKey_1->encryptContext);
    EVP_CIPHER_CTX_cleanup(&rIKey_1->decryptContext);
    EVP_CIPHER_CTX_cleanup(&LockingKey.encryptContext);
    EVP_CIPHER_CTX_cleanup(&LockingKey.decryptContext);
    
    // Clean up jni variables
    env->ReleaseStringUTFChars(jOldPin, oldPin);
    env->ReleaseStringUTFChars(jNewPin, newPin);

    return T2Success;
}

/*!
 * @brief Changes to a new PIN using ANSWERS to authenticate
 * @discussion Generates and saves new backup key
 * @param env Jni environment
 * @param thiz Passed jni object
 * @param jNewPin New pin
 * @param jAnswers Answers used to initialize t2Ctrypto
 * @return T2Success or T2Error
 */
jint Java_com_t2_fcads_FipsWrapper_changePinUsingAnswers( JNIEnv* env,jobject thiz, 
          jstring jNewPin,
          jstring jAnswers
           ) {

    T2Key LockingKey;
    T2Key acredential;
    T2Key *rIKey_1 = &acredential;
    LOGI("Java_com_t2_fcads_FipsWrapper_changePinUsingAnswers");

    if (!Java_com_t2_fcads_FipsWrapper_isInitialized(env, thiz)) {
      return T2Error;
    }   

    //   // Retrieve jni variables
    const char *answers= env->GetStringUTFChars(jAnswers, 0);
    const char *newPin = env->GetStringUTFChars(jNewPin, 0);

    int result = checkAnswers_I(env, (unsigned char*)answers);
    if (result != T2Success) {
        return T2Error;
    }

    
    LOGI("INFO: === changePinUsingAnswers ===");
    
    LOGI("INFO: *** getRIKeyUsing answers and backup key %s ***", answers);
    result = getRIKeyUsing(env, rIKey_1, (char*) answers, (char *) KEY_BACKUP_KEY);
    if (result != T2Success) {
      return result;
    }
    
    // Generate LockingKey = kdf(newPin)
    // ------------------------------
    LOGI("INFO: *** Generating New LockingKey  kdf(%s) ***", newPin);
    
    {
        unsigned char *key_data = (unsigned char *)newPin;
        int key_data_len = strlen(newPin);
        
        /* gen key and iv. init the cipher ctx object */
        if (key_init(key_data, key_data_len, (unsigned char *)_salt, &LockingKey)) {
            T2Assert(FALSE, "ERROR: initalizing key");
            return T2Error;
        }
    }
    
    // Generate MasterKey = encrypt(RI Key, LockingKey)
    // ------------------------------
    LOGI("INFO: *** Generating and saving to NVM  new MasterKey ***");
    generateMasterOrRemoteKeyAndSave(env, rIKey_1, &LockingKey, KEY_MASTER_KEY);
    
    _initializedPin = (unsigned char*)newPin;
    
    int outLength;
    unsigned char *encryptedPin = encryptStringUsingKey_malloc(rIKey_1, (unsigned char *) newPin, &outLength);
    T2Assert((encryptedPin != NULL), "Memory allocation error");

    LOGI("outLength = %d", outLength);
     logAsHexString(encryptedPin, outLength, (char *) "    Encrypted pin ");

    putData(env, KEY_DATABASE_PIN, (const char *)encryptedPin, outLength );
    free(encryptedPin);
    
    // Clean up jni variables
    env->ReleaseStringUTFChars(jAnswers, answers);
    env->ReleaseStringUTFChars(jNewPin, newPin);

    EVP_CIPHER_CTX_cleanup(&rIKey_1->encryptContext);
    EVP_CIPHER_CTX_cleanup(&rIKey_1->decryptContext);
    EVP_CIPHER_CTX_cleanup(&LockingKey.encryptContext);
    EVP_CIPHER_CTX_cleanup(&LockingKey.decryptContext);
    
    return T2Success;

}

/*!
 * @brief Performs initialization and login using pin and answers
 * @discussion This will set up the encrypted Master Key and Backup Key
 *             and save them  to NVM
 * @param env Jni environment
 * @param thiz Passed jni object
 * @param jpin User supplied pin
 * @param jAnswers User supplied answers
 * @answers Concatenated answers
 * @result Master Key is saved to NVM
 * @result Backup Key is saved to NVM
 * @return  T2Success or T2Error
 */
jint Java_com_t2_fcads_FipsWrapper_initializeLogin( JNIEnv* env,jobject thiz, 
          jstring jPin,
          jstring jAnswers
           ) {
  T2Key RIKey;
  T2Key LockingKey;
  T2Key SecondaryLockingKey;    
 
  // Retrieve jni variables
  const char *pin= env->GetStringUTFChars(jPin, 0);
  const char *answers = env->GetStringUTFChars(jAnswers, 0);

  /**
   * @brief RIKeyBytes - Bytes used as input for RIKey calculation
   * This is the initial set of bytes (password) used to create the Random Initialization Key (RIKey)
   * which will be used as a basis for all of the encryption/decryption
   *
   * There are two cases
   * if _useTestVectors is true
   *   RIKeyBytes will consist of the test password CANNED_RI_KEY_BYTES = "password"
   * else (normal operation)
   *   RIKeyBytes will be a random set of bytes
   */
  char *RIKeyBytes;
  int RIKeyBytesLen;

    _initializedPin = (unsigned char*) pin;

    if (useTestVectors) {
      LOGI(" -- Using test vectors for key ---");
      RIKeyBytes = (char*) malloc(sizeof(char) * CANNED_RI_KEY_BYTES_LENGTH );
      T2Assert((RIKeyBytes != NULL), "Memory allocation error for RIKey!");

      RIKeyBytesLen = CANNED_RI_KEY_BYTES_LENGTH; 
      char tmp1[] = CANNED_RI_KEY_BYTES;
      memcpy(RIKeyBytes, tmp1, RIKeyBytesLen);

      logAsHexString((unsigned char *) RIKeyBytes, CANNED_RI_KEY_BYTES_LENGTH, (char *) "xx     Password =  ");


      unsigned char tmp[] = CANNNED_SALT;
      memcpy(_salt,tmp,SALT_LENGTH);
      logAsHexString(_salt, SALT_LENGTH, (char *) "xx    salt =  ");
     }
    else {
        LOGI(" -- Using random vectors for key ---");
        //RAND_poll();
        //int result = RAND_bytes(_salt, SALT_LENGTH);
        //T2Assert(result == OpenSSLSuccess, "ERROR: - Can't calculate random salt");
        
        // Get salt based on package name
        int length;
        char *tmp = getPackageBasedSalt_malloc(env, &length);
        T2Assert(length  == SALT_LENGTH, "ERROR: - Incorrect salt length received from application");
        memcpy(_salt,tmp,SALT_LENGTH);
        logAsHexString(_salt, length, (char *) "xx    salt =  ");
        free(tmp);

        // Get random password for RiKey
        RIKeyBytes = (char*) malloc(sizeof(char) * MAX_KEY_LENGTH);
        RIKeyBytesLen = MAX_KEY_LENGTH;
        T2Assert(RIKeyBytes != NULL , "ERROR: memory allocation");
        int result = RAND_bytes((unsigned char*)RIKeyBytes, RIKeyBytesLen);
        T2Assert(result == OpenSSLSuccess, "ERROR: - Can't calculate RIKeyBytes");

        logAsHexString((unsigned char*) RIKeyBytes, RIKeyBytesLen, (char *) "xx     Password =  ");

    }
   

    // Now save salt to nvm
    putData(env, (char const *) KEY_SALT, (char const *) _salt, SALT_LENGTH );

    // Generate RIKey
    // RIKey is the main key used to encrypt and decrypt everything. Note, it is never stored unencrypted.
    // Only the master key and backup key are stored, from which the RIKey can be derived using
    // the pin and answers respectively.
    // -----------------
    LOGI("INFO: *** Generating RIKey ***");

    /* gen key and iv. init the cipher ctx object */
    if ((key_init((unsigned char*) RIKeyBytes, RIKeyBytesLen, (unsigned char *) _salt, &RIKey))) {
        T2Assert(0, "ERROR: initializing key");
        free(RIKeyBytes);
        // Clean up jni variables
        env->ReleaseStringUTFChars(jPin, pin);
        env->ReleaseStringUTFChars(jAnswers, answers);            
        return T2Error;
    }

    free(RIKeyBytes);


    // Generate LockingKey = kdf(PIN)
    // ------------------------------
    LOGI("INFO: *** Generating LockingKey kdf(%s) **n", pin);
    {
        unsigned char *key_data = (unsigned char *)pin;
        int key_data_len = strlen(pin);
        
        /* gen key and iv. init the cipher ctx object */
        if (key_init(key_data, key_data_len, (unsigned char *)_salt, &LockingKey)) {
            EVP_CIPHER_CTX_cleanup(&RIKey.encryptContext);
            EVP_CIPHER_CTX_cleanup(&RIKey.decryptContext);
            T2Assert(FALSE, "ERROR: initializing key");
            // Clean up jni variables
            env->ReleaseStringUTFChars(jPin, pin);
            env->ReleaseStringUTFChars(jAnswers, answers);            
            return T2Error;
        }
    }

    // Generate SecondaryLockingKey = kdf(Answers)
    // ------------------------------
    LOGI("INFO: *** Generating SecondaryLockingKey ***");
    {
        unsigned char *key_data = (unsigned char *)answers;
        int key_data_len = strlen(answers);
        
        /* gen key and iv. init the cipher ctx object */
        if (key_init(key_data, key_data_len, (unsigned char *)_salt, &SecondaryLockingKey)) {
            EVP_CIPHER_CTX_cleanup(&RIKey.encryptContext);
            EVP_CIPHER_CTX_cleanup(&RIKey.decryptContext);
            EVP_CIPHER_CTX_cleanup(&LockingKey.encryptContext);
            EVP_CIPHER_CTX_cleanup(&LockingKey.decryptContext);
            T2Assert(FALSE, "ERROR: initializing key");
            // Clean up jni variables
            env->ReleaseStringUTFChars(jPin, pin);
            env->ReleaseStringUTFChars(jAnswers, answers);            
            return T2Error;
        }
    }

    // Generate MasterKey = encrypt(RI Key, LockingKey)
    // ------------------------------
    LOGI("INFO: *** Generating and saving to NVM MasterKey ***");
    generateMasterOrRemoteKeyAndSave(env, &RIKey, &LockingKey, KEY_MASTER_KEY);
  
    // Generate BackupKey = encrypt(RI Key, SecondaryLockingKey)
    // ------------------------------
    LOGI("INFO: *** Generating and saving to nvm BackupKey ***");
    generateMasterOrRemoteKeyAndSave(env, &RIKey, &SecondaryLockingKey, KEY_BACKUP_KEY);
    
    // Encrypt the PIN and save to NVM
    int outLength;
    unsigned char *encryptedPin = encryptStringUsingKey_malloc(&RIKey, (unsigned char *) pin, &outLength); // OK to use this version because we know pin is zero terminated!
    T2Assert((encryptedPin != NULL), "Memory allocation error");

    LOGI("outLength = %d", outLength);
    logAsHexString((unsigned char*) encryptedPin, outLength, (char *) "    Encrypted pin ");
    
    putData(env, (char const *) KEY_DATABASE_PIN, (char const *) encryptedPin, outLength );
    free(encryptedPin);

    unsigned char *encryptedCheck = encryptBinaryUsingKey_malloc1(&RIKey, (unsigned char *) KCHECK_VALUE, strlen(KCHECK_VALUE), &outLength); // check is NOT zero terminated
    T2Assert((encryptedCheck != NULL), "Memory allocation error");

    putData(env, (char const *) KEY_DATABASE_CHECK, (char const *) encryptedCheck, outLength );

    // Use this for testing that decrypt works properly
    // LOGI("outLength length = %d", outLength);
    // unsigned char *decryptedCheck = decryptUsingKey_malloc1(&RIKey, encryptedCheck, &outLength);
    // logAsHexString(decryptedCheck, outLength, " xx   decryptedCheck = ");
    // LOGI("decryptedCheck length = %d", outLength);
    // free(decryptedCheck);
    free(encryptedCheck);



    EVP_CIPHER_CTX_cleanup(&RIKey.encryptContext);
    EVP_CIPHER_CTX_cleanup(&RIKey.decryptContext);
    EVP_CIPHER_CTX_cleanup(&LockingKey.encryptContext);
    EVP_CIPHER_CTX_cleanup(&LockingKey.decryptContext);
    EVP_CIPHER_CTX_cleanup(&SecondaryLockingKey.encryptContext);
    EVP_CIPHER_CTX_cleanup(&SecondaryLockingKey.decryptContext);
    
    // Clean up jni variables
    env->ReleaseStringUTFChars(jPin, pin);
    env->ReleaseStringUTFChars(jAnswers, answers);
    return T2Success;
}

/*!
 * @brief Checks to see if t2Crytp has been previously initialized
 * @param env Jni environment
 * @param thiz Passed jni object
 * @return  T2True or T2False
 */
jint Java_com_t2_fcads_FipsWrapper_isInitialized( JNIEnv* env,jobject thiz ) {
    int length;
    int retValue = T2True;
    unsigned char *masterKey = (unsigned char *) getData_malloc(env, KEY_MASTER_KEY, &length);
    if (length == 0)  {
      retValue =  T2False;
    } 
    free(masterKey);
    return retValue;
}


// This one uses pIN as wraw (doesn't use previously initialized RIKey

/*!
 * @brief Encrypts string using given pin
 * @discussion Note that this method does NOT depend on T2Crypto being previously initialized
 *   meaning is doesn't use masterKey
 * @param env Jni environment
 * @param thiz Passed jni object
 * @param jPin Password use
 * @param jPlainText Text to encrypt
 * @return  Encrypted string (or blank string if error)
 */
jstring Java_com_t2_fcads_FipsWrapper_encryptRaw(JNIEnv* env, jobject thiz, jstring jPin, jstring jPlainText) {
    T2Key RawKey; 
    genericBuffer[0] = 0;   // Clear out generic buffer in case we fail
    int result;

       // Retrieve jni variables
    const char *plainText= env->GetStringUTFChars(jPlainText, 0);
    const char *pin= env->GetStringUTFChars(jPin, 0);
 
    // Generate RawKey = kdf(PIN)
    // ------------------------------
    unsigned char *key_data = (unsigned char *)pin;
    int key_data_len = strlen(pin);
    

    unsigned char rawSalt[SALT_LENGTH];
    unsigned char tmp[] = CANNNED_SALT;
    memcpy(rawSalt,tmp,SALT_LENGTH);
    logAsHexString(rawSalt, SALT_LENGTH, (char *) "xx    salt =  ");


    /* gen key and iv. init the cipher ctx object */
    if (key_init(key_data, key_data_len, (unsigned char *)rawSalt, &RawKey)) {
        T2Assert(FALSE, "ERROR: initializing key");
        // Clean up jni variables
        env->ReleaseStringUTFChars(jPin, pin);
        env->ReleaseStringUTFChars(jPlainText, plainText);            
        return NULL;
    } else {

      int outLength;
      unsigned char *encryptedString = encryptStringUsingKey_malloc(&RawKey, (unsigned char *) plainText, &outLength);
      T2Assert((encryptedString != NULL), "Memory allocation error");

      // Note: we can't return the encrhypted string directoy because JAVA will try to 
      // interpret it as a string and fail UTF-8 conversion if any of the encrypted characters
      // have the high bit set. Therefore we must return a hex string equivalent of the binary
      unsigned char *tmp = binAsHexString_malloc(encryptedString, outLength);
      T2Assert((tmp != NULL), "Memory allocation error");
     
      if (strlen((char *) tmp) < GENERIC_BUFFER_SIZE) {
          sprintf((char*) genericBuffer, "%s", tmp);
      } else {
          LOGE("String to encrypt is too large!");
      }
      free(tmp);

      // Clean up jni variables
      env->ReleaseStringUTFChars(jPin, pin);
      env->ReleaseStringUTFChars(jPlainText, plainText);
      return env->NewStringUTF((char*)genericBuffer);

    }
}


/*!
 * @brief Decrypts a string using given pin
 * @discussion Note that this method does NOT depend on T2Crypto being previously initialized
 *   meaning  masterKey MUST have been [previously initialized!] 
 * @param thiz Passed jni object
 * @param jPin Password use
 * @param jCipherText Text to decrypt
 * @return  Decrypted string (or blank string if error)
 */
jbyteArray Java_com_t2_fcads_FipsWrapper_decryptRaw(JNIEnv* env, jobject thiz, jstring jPin, jstring jCipherText) {
    T2Key RawKey;   
    genericBuffer[0] = 0;   // Clear out generic buffer in case we fail
    int result;


    // Retrieve jni variables
    // Note: re are receiving a hex string equivilant of the encrypted binary
    const char *cipherText= env->GetStringUTFChars(jCipherText, 0);
    const char *pin= env->GetStringUTFChars(jPin, 0);

    unsigned char *key_data = (unsigned char *)pin;
    int key_data_len = strlen(pin);


    unsigned char rawSalt[SALT_LENGTH];
    unsigned char tmp[] = CANNNED_SALT;
    memcpy(rawSalt,tmp,SALT_LENGTH);
    logAsHexString(rawSalt, SALT_LENGTH, (char *) "xx    salt =  ");
    
    // Generate RawKey = kdf(PIN)
    // ------------------------------

    /* gen key and iv. init the cipher ctx object */
    if (key_init(key_data, key_data_len, (unsigned char *)rawSalt, &RawKey)) {
        T2Assert(FALSE, "ERROR: initializing key");
        // Clean up jni variables
        env->ReleaseStringUTFChars(jPin, pin);
        env->ReleaseStringUTFChars(jCipherText, cipherText);          
        return NULL;
    } else {

      int resultLength = strlen(cipherText);


      unsigned char *resultBinary = hexStringAsBin_malloc((unsigned char*)cipherText, &resultLength);
      if (resultBinary != NULL) {

        unsigned char *decrypted = decryptUsingKey_malloc1(&RawKey, (unsigned char*)resultBinary, &resultLength);
        
        T2Assert((decrypted != NULL), "Memory allocation error");

        if (resultLength < GENERIC_BUFFER_SIZE) {
            memcpy(genericBuffer, decrypted, resultLength);
        } else {
          LOGE("String to decrypt is too large!");
        }
        free(resultBinary);


    jbyteArray retArray = env->NewByteArray(resultLength);
    void *temp = env->GetPrimitiveArrayCritical((jarray)retArray, 0);
    memcpy(temp, genericBuffer, resultLength);
    env->ReleasePrimitiveArrayCritical(retArray, temp, 0);

        // Clean up jni variables
        env->ReleaseStringUTFChars(jPin, pin);
        env->ReleaseStringUTFChars(jCipherText, cipherText);





        //jstring retVal = env->NewStringUTF((char*)genericBuffer)


        //return retVal;
    return retArray;
      }
    }
}


/*!
 * @brief Encrypts string using given pin
 * @discussion Note that this method DOES depend on T2Crypto being previously initialized
 *   meaning  masterKey MUST have been [previously initialized!]
 * @param env Jni environment
 * @param thiz Passed jni object
 * @param jPin Password use
 * @param jPlainText Text to encrypt
 * @return  Encrypted string (or blank string if error)
 */
jstring Java_com_t2_fcads_FipsWrapper_encryptUsingT2Crypto(JNIEnv* env, jobject thiz, jstring jPin, jstring jPlainText) {
    T2Key aRIKey;
    T2Key *RIKey = &aRIKey;    
    genericBuffer[0] = 0;   // Clear out generic buffer in case we fail
    int result;

    if (!Java_com_t2_fcads_FipsWrapper_isInitialized(env, thiz)) {
      //return "";
      return NULL;
    }

    // Retrieve jni variables
    const char *plainText= env->GetStringUTFChars(jPlainText, 0);
    const char *pin= env->GetStringUTFChars(jPin, 0);

    // get the RIKey based in the given pin
    result = getRIKeyUsing(env, RIKey, (char *)pin, (char *) KEY_MASTER_KEY);
    if (result == T2Success) {

      int outLength;
      unsigned char *encryptedString = encryptStringUsingKey_malloc(RIKey, (unsigned char *) plainText, &outLength);
      T2Assert((encryptedString != NULL), "Memory allocation error");

      // Note: we can't return the encrhypted string directoy because JAVA will try to 
      // interpret it as a string and fail UTF-8 conversion if any of the encrypted characters
      // have the high bit set. Therefore we must return a hex string equivalent of the binary
      unsigned char *tmp = binAsHexString_malloc(encryptedString, outLength);
      T2Assert((tmp != NULL), "Memory allocation error");
     
      if (strlen((char const *) tmp) < GENERIC_BUFFER_SIZE) {
          sprintf((char*) genericBuffer, "%s", tmp);
      } else {
          LOGE("String to encrypt is too large!");
      }
      free(tmp);
    }

    // Clean up jni variables
    env->ReleaseStringUTFChars(jPin, pin);
    env->ReleaseStringUTFChars(jPlainText, plainText);
    return env->NewStringUTF((char*)genericBuffer);

}


/*!
 * @brief Decrypts a string using given pin
 * @discussion Note that this method does NOT depend on T2Crypto being previously initialized
 *   meaning is doesn't use masterKey
 * @param thiz Passed jni object
 * @param jPin Password use
 * @param jCipherText Text to decrypt
 * @return  Decrypted string (or blank string if error)
 */
jbyteArray Java_com_t2_fcads_FipsWrapper_decryptUsingT2Crypto(JNIEnv* env, jobject thiz, jstring jPin, jstring jCipherText) {
    T2Key aRIKey;
    T2Key *RIKey = &aRIKey;    
    genericBuffer[0] = 0;   // Clear out generic buffer in case we fail
    int result;
    int resultLength = 0;

    if (!Java_com_t2_fcads_FipsWrapper_isInitialized(env, thiz)) {
      //return "";
      return NULL;
    }

    // Retrieve jni variables
    // Note: re are receiving a hex string equivilant of the encrypted binary
    const char *cipherText= env->GetStringUTFChars(jCipherText, 0);
    const char *pin= env->GetStringUTFChars(jPin, 0);

    // get the RIKey based in the given pin
    result = getRIKeyUsing(env, RIKey, (char *)pin, (char *) KEY_MASTER_KEY);
    if (result == T2Success) {
      resultLength = strlen(cipherText);

      unsigned char *resultBinary = hexStringAsBin_malloc((unsigned char*)cipherText, &resultLength);
      if (resultBinary != NULL) {

        unsigned char *decrypted = decryptUsingKey_malloc1(RIKey, (unsigned char*)resultBinary, &resultLength);
        T2Assert((decrypted != NULL), "Memory allocation error");

        if (resultLength < GENERIC_BUFFER_SIZE) {
            memcpy(genericBuffer, decrypted, resultLength);
        } else {
          LOGE("String to decrypt is too large!");
        }
        free(resultBinary);

      }
    }

    jbyteArray retArray = env->NewByteArray(resultLength);
    void *temp = env->GetPrimitiveArrayCritical((jarray)retArray, 0);
    memcpy(temp, genericBuffer, resultLength);
    env->ReleasePrimitiveArrayCritical(retArray, temp, 0);



    // Clean up jni variables
    env->ReleaseStringUTFChars(jPin, pin);
    env->ReleaseStringUTFChars(jCipherText, cipherText);
    //return env->NewStringUTF((char*)genericBuffer);
    return retArray;

}


/*!
 * @brief Retrieves database key (to use in SqlCipher) using PIN to authenticate
 * @discussion 
 * @param env Jni environment
 * @param thiz Passed jni object
 * @param jPin Password to authenticate with (should match most recent password)
 * @return  Database key
 */
jstring Java_com_t2_fcads_FipsWrapper_getDatabaseKeyUsingPin(JNIEnv* env, jobject thiz, jstring jPin) {

    T2Key aRIKey;
    T2Key *RIKey = &aRIKey;    formattedKey[0] = 0;   // Clear out formatted key in case we fail


    // Retrieve jni variables
    const char *pin= env->GetStringUTFChars(jPin, 0);
 
    // Make sure the pin is correct
    int result = checkPin_I(env, (unsigned char*) pin);
   
    if (result == T2Success) {
      // Pin is correct, now get the RIKey based in the given pin
      result = getRIKeyUsing(env, RIKey, (char*) pin, (char *) KEY_MASTER_KEY);

      if (result == T2Success) {
          
          // RIKey is now set up
          // Now create the database key by appending the key and iv

          unsigned char *RIKeyAndIv = (unsigned char *) malloc((RIKey->keyLength + RIKey->ivLength) + 1);
          T2Assert(RIKeyAndIv != NULL , "ERROR: memory allocation");
          int i;
          for (i = 0; i < RIKey->keyLength; i++) {
              RIKeyAndIv[i] = RIKey->key[i];
          }
          for (i = 0; i < RIKey->ivLength; i++) {
              RIKeyAndIv[i + RIKey->keyLength] = RIKey->iv[i];
          }
          RIKeyAndIv[RIKey->keyLength + RIKey->ivLength] = 0; // Terminate it with a zero
        
          unsigned char *key = binAsHexString_malloc(RIKeyAndIv, RIKey->keyLength + RIKey->ivLength);
        
          if (key != NULL && strlen((char const *) key) < 256 - 4) {
            // Note that the format of this string is specific
            // IT tells SQLCipher to use this key directly instead of
            // using it as a password to derive a key from 
            sprintf((char*) formattedKey, "x'%s'", key);
          }


          if (key != NULL) free(key);
          if (RIKeyAndIv != NULL) free(RIKeyAndIv);
          EVP_CIPHER_CTX_cleanup(&RIKey->encryptContext);
          EVP_CIPHER_CTX_cleanup(&RIKey->decryptContext);

      } 

    }
    
    // Clean up jni variables
    env->ReleaseStringUTFChars(jPin, pin);
    return env->NewStringUTF((char*)formattedKey);

}



// ------------------------------------
// FIPS  Interface Routines
// ------------------------------------

/*!
 * @brief Returns the current FIPS mode 
 * @param env Jni environment
 * @param thiz Passed jni object
 * @return  1 = in FIPS mode, 0 = Not in FIPS mode
 */
jint Java_com_t2_fcads_FipsWrapper_FIPSmode( JNIEnv* env, jobject thiz ) {
    return MY_FIPS_mode();
}

/*!
 * @brief Returns the current t2Wrapper version
 * @param env Jni environment
 * @param thiz Passed jni object
 * @return Version string
 */
jstring Java_com_t2_fcads_FipsWrapper_T2FIPSVersion( JNIEnv* env, jobject thiz ) {
    return env->NewStringUTF("1.5.1");
}



// -----------------------------------------------------------------------------------------------------------
// Internal Routines
// -----------------------------------------------------------------------------------------------------------

/*!
 * @brief Returns the current FIPS mode 
 * @return  1 = in FIPS mode, 0 = Not in FIPS mode
 */
int MY_FIPS_mode() {
    int i;
    int len = 0;
    int mode = 0;
    int ret = 0;
  const unsigned int p1 = (unsigned int)FIPS_rodata_start;
  const unsigned int p2 = (unsigned int)FIPS_rodata_end;

  LOGI(".rodata start: 0x%08x\n", p1);
  LOGI(".rodata end: 0x%08x\n", p2);
 
  logAsHexString(FIPS_signature, SIGNATURE_SIZE, (char *) "embedded fingerprint  ");
 
  len = FIPS_incore_fingerprint(Calculated_signature, sizeof(Calculated_signature));
  
  if (len < 0) {
    LOGI("Failed to calculate expected signature");
    return -1;
  }

  logAsHexString(Calculated_signature, len, (char *) "calculated fingerprint");


  mode = FIPS_mode();
  LOGI("FIPS mode: %d\n", mode);
 
  if (!mode) {
    LOGI("Attempting to enable FIPS mode\n");
    mode = FIPS_mode_set(1);
    LOGI("FIPS_mode_set(1) returned %d", mode);
    
    if(mode) {
      LOGI("FIPS mode enabled\n");
    }
    else {
      LOGI("ERROR trying to enable FIPS mode\n");
     // ERR_load_crypto_strings();
     // ERR_print_errors_fp(stderr);
    }
  }
   return mode;
}


/*!
 * @brief encrypts plain text
 * @param encryptContext Encryption context
 * @param plaintext bytes to encrypt
 * @param len Length of input (also gets set as length of output)
 * @return  encrypted bytes
 */
unsigned char * aes_encrypt_malloc(EVP_CIPHER_CTX * encryptContext , unsigned char * plaintext, int * len) {
    /* max ciphertext len for a n bytes of plaintext is n + AES_BLOCK_SIZE -1 bytes */
    int c_len = *len + AES_BLOCK_SIZE, f_len = 0;
    unsigned char *pCiphertext = (unsigned char *) malloc(c_len);
    if (pCiphertext == NULL) {
      
      return NULL;
    }

    
    /* allows reusing of 'encryptContext' for multiple encryption cycles */
    EVP_EncryptInit_ex(encryptContext, NULL, NULL, NULL, NULL);
    
    /* update ciphertext, c_len is filled with the length of ciphertext generated,
     *len is the size of plaintext in bytes */
    EVP_EncryptUpdate(encryptContext, pCiphertext, &c_len, plaintext, *len);
    
    /* update ciphertext with the final remaining bytes */
    EVP_EncryptFinal_ex(encryptContext, pCiphertext+c_len, &f_len);
    
    *len = c_len + f_len;
    return pCiphertext;
}


/*!
 * @brief Decrypts encrypted test
 * @discussion 
 * @param decryptContext Decryption context
 * @param ciphertext bytes to decrypt
 * @param len Length of input (also gets set as length of output)
 * @return  Decrypted bytes
 */
unsigned char * aes_decrypt_malloc(EVP_CIPHER_CTX * decryptContext, unsigned char * ciphertext, int *len) {
    /* plaintext will always be equal to or lesser than length of ciphertext*/
    int p_len = *len, f_len = 0;
    unsigned char *plaintext = (unsigned char *)  malloc(p_len);
    if (plaintext == NULL) {
      LOGE("xxFailxx Memory allocation error");
      return NULL;
    }

    EVP_DecryptInit_ex(decryptContext, NULL, NULL, NULL, NULL);
    EVP_DecryptUpdate(decryptContext, plaintext, &p_len, ciphertext, *len);
    EVP_DecryptFinal_ex(decryptContext, plaintext+p_len, &f_len);

    *len = p_len + f_len;

    return plaintext;
}


/*!
 * @brief Encrypts binary array using a T2Key
 * @discussion 
 * @param credentials T2Key credentials to use in encrypt/decrypt functions
 * @param  pUencryptedText bytes to encrypt
 * @param inLength Length of input byte array
 * @param outlength Gets set to length of output
 * @return  Encrypted bytes
 */
unsigned char * encryptBinaryUsingKey_malloc1(T2Key * credentials, unsigned char * pUencryptedText, int inLength, int * outLength) {
    unsigned char* szEncryptedText =  aes_encrypt_malloc(&credentials->encryptContext, pUencryptedText, &inLength);
    *outLength = inLength;
    return szEncryptedText;
}

/*!
 * @brief encrypts string using a T2Key
 * @discussion ** Note that the plaintext input to this routine MUST be zero terminated
 * @param credentials T2Key credentials to use in encrypt/decrypt functions
 * @param pUencryptedText Zero terminated input string
 * @param outlength Gets set to length of output
 * @return  Encrypted text
 */
unsigned char * encryptStringUsingKey_malloc(T2Key * credentials, unsigned char * pUencryptedText, int * outLength) {
    int len1 = strlen((char const *) pUencryptedText) + 1; // Make sure we encrypt the terminating 0 also!
    unsigned char* szEncryptedText =  aes_encrypt_malloc(&credentials->encryptContext, pUencryptedText, &len1);
    *outLength = len1;
    return szEncryptedText;
}

/*!
 * @brief Decrypts binary array using a T2Key
 * @discussion 
 * @param credentials T2Key credentials to use in encrypt/decrypt functions
 * @param encryptedText Bytes to decrypt
 * @param inLength length if input (Also gets set to length of output)
 * @return  Decrypted binary
 */
unsigned char * decryptUsingKey_malloc1(T2Key * credentials, unsigned char * encryptedText, int * inLength) {
    unsigned char* decryptedText =  aes_decrypt_malloc(&credentials->decryptContext, encryptedText, inLength);
    return decryptedText;
}


/*!
 * @brief Generates master or remote key from T2Key and saves to NVM
 * @discussion The master/remote key is a string consisting of the concatenated key and iv or a T2Key
 * @param env Jni environment
 * @param RIKey
 * @param LockingKLey Locking key (for master key) or Secondary locking key key (for Backup key)
 * @param KeyType Text indicating master or remote key
 */
void generateMasterOrRemoteKeyAndSave(JNIEnv* env, T2Key * RIKey, T2Key * LockingKey, const char * keyType) {

    // This input to encrypt wil be the RIKey (key and iv concatenated)
    int i; 
    unsigned char *RIKeyAndIv = (unsigned char* )malloc((RIKey->ivLength + RIKey->keyLength) + 1);
    T2Assert(RIKeyAndIv != NULL , "ERROR: memory allocation");

    for (i = 0; i < RIKey->keyLength; i++) {
        RIKeyAndIv[i] = RIKey->key[i];
    }
    for (i = 0; i < RIKey->ivLength; i++) {
        RIKeyAndIv[i + RIKey->keyLength] = RIKey->iv[i];
    }
    RIKeyAndIv[RIKey->ivLength + RIKey->keyLength] = 0; // Terminate it with a zero
    
    /* The enc/dec functions deal with binary data and not C strings. strlen() will
     return length of the string without counting the '\0' string marker. We always
     pass in the marker byte to the encrypt/decrypt functions so that after decryption
     we end up with a legal C string */
    int len = (RIKey->ivLength + RIKey->keyLength) + 1;
    unsigned char *rawMasterKey = aes_encrypt_malloc(&LockingKey->encryptContext, RIKeyAndIv, &len);
    T2Assert(rawMasterKey != NULL, "Memory Allocation error");
    free (RIKeyAndIv);

    logAsHexString((unsigned char *) rawMasterKey, RIKey->ivLength + RIKey->keyLength,  (char *)keyType);

    // Save Master Key to User Preferences
    // ------------------------------
    LOGI("INFO:   *** Save %s Key to User Preferences ***", keyType);
    LOGI("INFO:   length = %d", RIKey->ivLength + RIKey->keyLength);

    putData(env, keyType, (char const *) rawMasterKey, RIKey->ivLength + RIKey->keyLength);

    free(rawMasterKey);

}

/*!
 * @discussion The master/remote key is a string consisting of the concatenated key and iv or a T2Key
 * @param env Jni environment
 * @param RIKey
 * @param LockingKLey Locking key (for master key) or Secondary locking key key (for Backup key)
 * @param KeyType Text indicating master or remote key
 * @return Master or Remove key text
 */
char * generateMasterOrRemoteKey_malloc(JNIEnv* env, T2Key * RIKey, T2Key * LockingKey, const char * keyType) {

    // This input to encrypt wil be the RIKey (key and iv concatenated)
    int i; 
    unsigned char *RIKeyAndIv = (unsigned char *) malloc((RIKey->ivLength + RIKey->keyLength) + 1);
    T2Assert(RIKeyAndIv != NULL , "ERROR: memory allocation");

    for (i = 0; i < RIKey->keyLength; i++) {
        RIKeyAndIv[i] = RIKey->key[i];
    }
    for (i = 0; i < RIKey->ivLength; i++) {
        RIKeyAndIv[i + RIKey->keyLength] = RIKey->iv[i];
    }
    RIKeyAndIv[RIKey->ivLength + RIKey->keyLength] = 0; // Terminate it with a zero
    
    /* The enc/dec functions deal with binary data and not C strings. strlen() will
     return length of the string without counting the '\0' string marker. We always
     pass in the marker byte to the encrypt/decrypt functions so that after decryption
     we end up with a legal C string */
    int len = (RIKey->ivLength + RIKey->keyLength) + 1;
    unsigned char *rawMasterKey = aes_encrypt_malloc(&LockingKey->encryptContext, RIKeyAndIv, &len);
    T2Assert(rawMasterKey != NULL, "Memory Allocation error");
    free (RIKeyAndIv);
    return (char *) rawMasterKey;
}


/*!
 * @brief Initializes a T2Key based on a password
 * @discussion Performs KDF function on a password and salt to initialize a T2Key. 
     This is used to generate a T2Key from a password (or pin)
 * @param key_data Password to use in KDF function
 * @param key_data_len Length of password
 * @param salt Salt to use in KDF function
 * @param aCredentials T2Key to initialize
 * @return  T2Success or T2Error
 */
int key_init(unsigned char * key_data, int key_data_len, unsigned char * salt, T2Key * aCredentials) {
    int i, nrounds = 5;
    
    /*
     * Gen key & IV for AES 256 CBC mode. A SHA1 digest is used to hash the supplied key material.
     * rounds is the number of times the we hash the material. More rounds are more secure but
     * slower.
     * This uses the KDF algorithm to derive key from password phrase
     */
    i = EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha1(), salt, key_data, key_data_len, nrounds, aCredentials->key, aCredentials->iv);
    if (i != EVP_aes_256_cbc_Key_LENGTH) {
        LOGI("ERROR: Key size is %d bits - should be %d bits\n", i, EVP_aes_256_cbc_Key_LENGTH * 8);
        return T2Error;
    }
    
    // For EVP_aes_256_cbc, key length = 32 bytes, iv length = 16 types
    aCredentials->keyLength = EVP_aes_256_cbc_Key_LENGTH;
    aCredentials->ivLength = EVP_aes_256_cbc_Iv_LENGTH;
    
    logAsHexString(( unsigned char *)aCredentials->key, aCredentials->keyLength, (char *) "    key");
    logAsHexString((unsigned char *)aCredentials->iv, aCredentials->ivLength, (char *) "     iv");
    
    // Setup encryption context
    EVP_CIPHER_CTX_init(&aCredentials->encryptContext);                                     // Initialize ciipher context
    EVP_EncryptInit_ex(&aCredentials->encryptContext, EVP_aes_256_cbc(), NULL, aCredentials->key, aCredentials->iv);    // Set up context to use specific cyper type
    
    EVP_CIPHER_CTX_init(&aCredentials->decryptContext);                                     // Initialize ciipher context
    EVP_DecryptInit_ex(&aCredentials->decryptContext, EVP_aes_256_cbc(), NULL, aCredentials->key, aCredentials->iv);    // Set up context to use specific cyper type
    
    return T2Success;
}

/*!
 * @brief Initializes a T2Key based on bytes from masterkey or backup key
 * @discussion Performs KDF function on a password and salt to initialize a T2Key. 
     This is used to generate a T2Key from the bytes of a masterkey or a backup key
 * @param key_data Password to use in KDF function
 * @param key_data_len Length of password
 * @param aCredentials T2Key to initialize
 * @return   T2Success or T2Error
 */
int keyCredentialsFromBytes(unsigned char * key_data, int key_data_len, int iv_data_len, T2Key * aCredentials) {

    int i;
    for (i = 0; i < key_data_len; i++) {
        aCredentials->key[i] = key_data[i];
    }
    
    for (i = 0; i < iv_data_len; i++) {
        aCredentials->iv[i] = key_data[i + key_data_len];
    }
    
    logAsHexString((unsigned char *)aCredentials->key, key_data_len, (char *) "    key");
    logAsHexString((unsigned char *)aCredentials->iv, iv_data_len, (char *) "   iv");
    
    // Setup encryption context
    EVP_CIPHER_CTX_init(&aCredentials->encryptContext);                                     // Initialize ciipher context
    EVP_EncryptInit_ex(&aCredentials->encryptContext, EVP_aes_256_cbc(), NULL, aCredentials->key, aCredentials->iv);    // Set up context to use specific cyper type
    
    EVP_CIPHER_CTX_init(&aCredentials->decryptContext);                                     // Initialize ciipher context
    EVP_DecryptInit_ex(&aCredentials->decryptContext, EVP_aes_256_cbc(), NULL, aCredentials->key, aCredentials->iv);    // Set up context to use specific cyper type
    
    return T2Success;
}


/*!
 * @brief Retrieves an RIKey (Random Intermediate Key) based on answers or pin
 * @discussion The RIKey is NEVER saved in NVM, it must always be recovered using the pin or answers
 * @param 
 * @param 
 * @return T2Success or T2Error
 */
int getRIKeyUsing(JNIEnv* env, T2Key * RiKey, char * answersOrPin, char * keyType) {
    int retVal = T2Success;
    T2Key LockingKey;

    if (strcmp(keyType, KEY_MASTER_KEY) == 0) {
        LOGI("INFO: ***  Generate Locking key ***");
    }
    else {
        LOGI("INFO: ***  Generate Secondarylocking key ***");
    }

    // Generate LockingKey = kdf(PIN)
    // ------------------------------
    unsigned char *key_data;
    int key_data_len;
    key_data = (unsigned char *)answersOrPin;
    key_data_len = strlen(answersOrPin);

    /* gen key and iv. init the cipher ctx object */
    if (key_init(key_data, key_data_len, (unsigned char *)_salt, &LockingKey)) {
        T2Assert(FALSE, "ERROR: initalizing key");
        return T2Error;
    }

    // Recall Masterkey or BackupKey from NVM based on keyType
    // --------------------------
    LOGI("INFO: ***  Recall %s from NVM ***\n", keyType);
    int length;
    unsigned char *masterKey = (unsigned char *) getData_malloc(env, keyType, &length);
    logAsHexString(masterKey, length, (char *) "INFO: masterKey Stored value = ");
    
    // Double check the master key length
    if (length != EVP_aes_256_cbc_Key_LENGTH + EVP_aes_256_cbc_Iv_LENGTH) {
      LOGI(" ERROR master key length incorrect, is: %d, should be: %d", length, EVP_aes_256_cbc_Key_LENGTH + EVP_aes_256_cbc_Iv_LENGTH);
      free(masterKey);
      return T2Error;
    }

    int len = length + 1;
 // TODO check here for alloc

    char *RIKeyAndIv = (char *) malloc(len);
    T2Assert(RIKeyAndIv != NULL , "ERROR: memory allocation");

    RIKeyAndIv[length] = 0; // Terminate it with a zero
    
    if (strcmp(keyType, KEY_MASTER_KEY) == 0) {
      LOGI("INFO: *** Generate RIKey = decrypt(MasterKey, LockingKey) ***");
    }
    else {
      LOGI("INFO: *** Generate RIKey = decrypt(BackupKey, SecondaryLockingKey) ***");
    }
    
     RIKeyAndIv = (char *) aes_decrypt_malloc(&LockingKey.decryptContext, masterKey, &len);

    // We now have the raw data, be must recreate the actual key contect from this
    keyCredentialsFromBytes((unsigned char*) RIKeyAndIv, EVP_aes_256_cbc_Key_LENGTH, EVP_aes_256_cbc_Iv_LENGTH, RiKey);
    RiKey->ivLength = EVP_aes_256_cbc_Iv_LENGTH;
    RiKey->keyLength = EVP_aes_256_cbc_Key_LENGTH;

    free(masterKey);
    free(RIKeyAndIv);
    
    EVP_CIPHER_CTX_cleanup(&LockingKey.encryptContext);
    EVP_CIPHER_CTX_cleanup(&LockingKey.decryptContext);
    
    return retVal;
}


/*!
 * @brief Checks to see if the answers give and the same answers used to initialize t2Cypto
 * @discussion 
 * @param env Jni environment
 * @param thiz Passed jni object
 * @param jAnswers Answers to check
 * @return  T2Success if answers are correct or T2Error if not
 */
jint Java_com_t2_fcads_FipsWrapper_checkAnswers( JNIEnv* env,jobject thiz, jstring jAnswers) {
  int retVal = T2Error;

  
  if (!Java_com_t2_fcads_FipsWrapper_isInitialized(env, thiz)) {
    return T2Error;
  }

  const char *answers = env->GetStringUTFChars(jAnswers, 0);

  retVal = checkAnswers_I(env, (unsigned char *)answers);

  env->ReleaseStringUTFChars(jAnswers, answers);
  return retVal;
}


/*!
 * @brief Internal version of Java_com_t2_fcads_FipsWrapper_checkAnswers
 * @discussion 
 * @param env Jni environment
 * @param jAnswers Answers to check
 * @return  T2Success if answers are correct or T2Error if not  
 */
int checkAnswers_I(JNIEnv* env, unsigned char *answers) {
  int retVal = T2Error;
  T2Key acredential;
  T2Key *rIKey_1 = &acredential;
  
  // Re-initialize salt from nvm
  int length;
  unsigned char *tmp  = (unsigned char*) getData_malloc(env, KEY_SALT, &length);
  memcpy(_salt,tmp,SALT_LENGTH);
  free(tmp);
 
  LOGI("INFO: === checkAnswers ===");
  LOGI("INFO: *** Generating Secondary LockingKey  kdf(%s) ***", answers);
  
  // Generate the RIKey based on Pin and Master Key
  int result = getRIKeyUsing(env, rIKey_1, (char*)answers, (char *) KEY_BACKUP_KEY);
  if (result != T2Success) {
    return result;
  }


  EVP_CIPHER_CTX_cleanup(&acredential.encryptContext);
  EVP_CIPHER_CTX_cleanup(&acredential.encryptContext);
  
  // Read the CHECK from NVM and make sure we can decode it properly
  // if not fail login
  unsigned char *encryptedCheck = (unsigned char*) getData_malloc(env, KEY_DATABASE_CHECK, &length);
  LOGI("encryptedCheck length = %d", length);
  logAsHexString(encryptedCheck, length, (char *) "    encryptedCheck = ");

  if (encryptedCheck == NULL) {
    return T2Error;
  }
  if (length == 0) {
    LOGI("xxFailxx - encryptedcheck length is zero!!")
    free (encryptedCheck);
    return T2Error;
  }
  unsigned char *decryptedCheck = decryptUsingKey_malloc1(rIKey_1, encryptedCheck, &length);
  T2Assert((decryptedCheck != NULL), "Memory allocation error");

  LOGI("q decryptedCheck length = %d", length);
  logAsHexString(decryptedCheck, length, (char *) "    decryptedCheck = ");

  EVP_CIPHER_CTX_cleanup(&rIKey_1->encryptContext);
  EVP_CIPHER_CTX_cleanup(&rIKey_1->decryptContext);
  if (length > 0 && memcmp(decryptedCheck, KCHECK_VALUE, length) == 0) {
      retVal = T2Success;
  }
  else {
      LOGI("WARNING: answers does not match");
  }

  free(encryptedCheck);
  free(decryptedCheck);


  return retVal;
}


/*!
 * @brief Checks pin or password to see if it matches the latest pin or password set
 * @discussion 
 * @param env Jni environment
 * @param thiz Passed jni object
 * @param jPin Pin to check
 * @return  T2Success if pin is correct or T2Error if not
 */
jint Java_com_t2_fcads_FipsWrapper_checkPin( JNIEnv* env,jobject thiz, jstring jPin) {
  if (!Java_com_t2_fcads_FipsWrapper_isInitialized(env, thiz)) {
    return T2Error;
  }

  const char *pin = env->GetStringUTFChars(jPin, 0);

  int retVal = checkPin_I(env, (unsigned char *)pin);

  env->ReleaseStringUTFChars(jPin, pin);
  return retVal;
}

/*!
 * @brief Internal version of Java_com_t2_fcads_FipsWrapper_checkPin
 * @discussion 
 * @param jPin Pin to check
 * @return  T2Success if pin is correct or T2Error if not 
 */
int checkPin_I(JNIEnv* env, unsigned char *pin) {
    int retVal = T2Error;
    T2Key acredential;
    T2Key *rIKey_1 = &acredential;

   
    // Re-initialize salt from nvm
    int length;
    unsigned char *tmp  = (unsigned char*) getData_malloc(env, KEY_SALT, &length);
    memcpy(_salt,tmp,SALT_LENGTH);
    free(tmp);

    
    LOGI("INFO: === checkPin ===");
    LOGI("INFO: *** Generating LockingKey  kdf(%s) ***", pin);
    
    // Generate the RIKey based on Pin and Master Key
    int result = getRIKeyUsing(env, rIKey_1, (char *) pin, (char *) KEY_MASTER_KEY);
    if (result != T2Success) {
      return result;
    }

    // Read the PIN from NVM and make sure we can decode it properly
    // if not fail login
    unsigned char *encryptedPin = (unsigned char *) getData_malloc(env, KEY_DATABASE_PIN, &length);
    if (encryptedPin == NULL) {
      LOGI("xxFailxx ---------------- Encrypted pin is NULL!");
      return T2Error;
    }

    if (length == 0) {
      LOGI("xxFailxx ---------------- Encrypted pin is blank!");
      free(encryptedPin);
      return T2Error;
    }


    unsigned char *decryptedPIN = decryptUsingKey_malloc1(rIKey_1, encryptedPin, &length);
    T2Assert((decryptedPIN != NULL), "Memory allocation error");

    EVP_CIPHER_CTX_cleanup(&acredential.encryptContext);
    EVP_CIPHER_CTX_cleanup(&acredential.encryptContext);
  
    EVP_CIPHER_CTX_cleanup(&rIKey_1->encryptContext);
    EVP_CIPHER_CTX_cleanup(&rIKey_1->decryptContext);

    logAsHexString(decryptedPIN, length, (char *) "CheckPin: saved pin = ");
    logAsHexString(pin, strlen((char const *) pin), (char *) "CheckPin: presented pin = ");

    //if (strcmp(decryptedPIN, pin) == 0) {
    if (length > 0 && memcmp(decryptedPIN, pin, length) == 0) {
        _initializedPin = pin;
        retVal = T2Success;
    }
    else {
        LOGI("WARNING: PIN does not match");
    }
    free(encryptedPin);
    free(decryptedPIN);
    return retVal;
}

/*!
 * @brief Utility to covet Hex string to binary
 * @param hex String to covert
 * @param stringLength Length of input (Also gets set to length of output)
 * @return  Binary representation of input string
 */
unsigned char * hexStringAsBin_malloc(unsigned char * hex, int *stringLength) {
    
    unsigned char *result;
    unsigned int  i;
    
    if (!stringLength) {
        LOGI("No string length!");
        return NULL;     
    }

    int inStringLength = *stringLength;

    *stringLength = *stringLength / 2;

    result = (unsigned char *) malloc(*stringLength );
    if (result == NULL) {
      LOGE("xxFAILxx   Unable to allocate memory");
      return NULL;
    }

    int resultIndex = 0;
    unsigned char tmp = 0;;
    for (i = 0; i < inStringLength; i++) {
        int digit = hex[i];
        //LOGI("digit = %x", digit);
        if (digit >= 0x30 && digit <= 0x39) {
            tmp = digit - 0x30;
        } else if (digit >= 0x61 && digit <= 0x66) {
            tmp = digit - 0x61 + 10;
        }

        if ((i & 1) == 0) {

            result[resultIndex] = tmp << 4;
            //LOGI("even - tmp = %x, resultIndex = %d, result[resultIndex] = %x", tmp, resultIndex, result[resultIndex]);
        } else {
                 
            result[resultIndex] |= tmp;
            //LOGI("odd - tmp = %x, resultIndex = %d, result[resultIndex] = %x", tmp, resultIndex, result[resultIndex]);
            resultIndex++;
        }
    }
    return result;
}


/*!
 * @brief Utility to convert binary array to a Hex string
 * @discussion 
 * @param binary array to covert
 * @param binsz Length of input (Also gets set to length of output)
 * @return  Hex representation of bytes  
 */
unsigned char * binAsHexString_malloc(unsigned char * bin, unsigned int binsz) {
    
    unsigned char *result;
    char          hex_str[]= "0123456789abcdef";
    unsigned int  i;
    
    result = (unsigned char *) malloc(binsz * 2 + 1);
    if (result == NULL) {
      LOGE("xxFAILxx   Unable to allocate memory");
      return NULL;
    }

    (result)[binsz * 2] = 0;
    
    if (!binsz)
        return NULL;
    
    for (i = 0; i < binsz; i++) {
        (result)[i * 2 + 0] = hex_str[bin[i] >> 4  ];
        (result)[i * 2 + 1] = hex_str[bin[i] & 0x0F];
    }
    
    return result;
    
}


/*!
 * @brief Utility to log a binary array as a string
 * @param binary array to log
 * @param binsz Length of input
 * @param message Message to prepend to log line
 */
void logAsHexString(unsigned char * bin, unsigned int binsz, char * message) {

    char *result;
    char          hex_str[]= "0123456789abcdef";
    unsigned int  i;
    
    result = (char *)malloc(binsz * 2 + 1);
    if (result == NULL) {
      return;
    }
    (result)[binsz * 2] = 0;
    
    if (!binsz)
        return;
    
    for (i = 0; i < binsz; i++) {
        (result)[i * 2 + 0] = hex_str[bin[i] >> 4  ];
        (result)[i * 2 + 1] = hex_str[bin[i] & 0x0F];
    }

    LOGI("   %s = : %s \n", message, result);
    free(result);
   
}

} // End extern "C" {
