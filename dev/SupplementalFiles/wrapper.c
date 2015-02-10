/*****************************************************************
wrapper.c

Copyright (C) 2011-2014 The National Center for Telehealth and 
Technology

Eclipse Public License 1.0 (EPL-1.0)

This library is free software; you can redistribute it and/or
modify it under the terms of the Eclipse Public License as
published by the Free Software Foundation, version 1.0 of the 
License.

The Eclipse Public License is a reciprocal license, under 
Section 3. REQUIREMENTS iv) states that source code for the 
Program is available from such Contributor, and informs licensees 
how to obtain it in a reasonable manner on or through a medium 
customarily used for software exchange.

Post your updates and modifications to our GitHub or email to 
t2@tee2.org.

This library is distributed WITHOUT ANY WARRANTY; without 
the implied warranty of MERCHANTABILITY or FITNESS FOR A 
PARTICULAR PURPOSE.  See the Eclipse Public License 1.0 (EPL-1.0)
for more details.
 
You should have received a copy of the Eclipse Public License
along with this library; if not, 
visit http://www.opensource.org/licenses/EPL-1.0

*****************************************************************/
#include <string.h>
#include <jni.h>
#include <openssl/des.h>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <android/log.h>

// JNI Stuff
//     Assumptions, The project includes:
//         1. A JAVA class FipsWrapper.java in package com.t2.fcads
//         2. A native file called wrapper.c

//     To add a static java method that a native routine can call
//         1. Add method to class com.t2.fcads 
//             take note of it's signature, it will be used indirectly in the native code
//             in the method call GetDtaticMethodId.
//         2. In the native code call like this:
//             jclass cls = (*env)->FindClass(env, "com/t2/fcads/FipsWrapper");
//             jmethodID id = (*env)->GetStaticMethodID(env, cls, "clearAllData", "()V");
//             (*env)->CallStaticVoidMethod(env, cls, id);

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
//                 const char *pin= (*env)->GetStringUTFChars(env, jPin, 0);
//                 (*env)->ReleaseStringUTFChars(env, jPin, pin);
//                 return (*env)->NewStringUTF(env, (char*)formattedKey);
//             }        
//         1a. Note that the method must be named according to the class and package name that will be calling it:
//                 JAVA_{package name}_{class name}_{method name}
//             in the above: 
//                 package = com_t2_fcads
//                 class = FipsWrapper
//                 method = example
//         1b. It is important that you use (*env)->Getxx() and (*env)->Release() before using variables
//             and that you only return environment variables ex:
//                 return (*env)->NewStringUTF(env, (char*)formattedKey);
//         2. In the project class that will be calling the native method add a declaration of the method
//             public native String example(String pin);

//             You can then call the native method from that JAVA class, ex:
//                 String result = example(new String("string to process"));



#define APPNAME "wrapper"




#define boolean int
#define TRUE 1
#define FALSE 0
#define SIGNATURE_SIZE 20

extern const void*          FIPS_text_start(),  *FIPS_text_end();
extern const unsigned char  FIPS_rodata_start[], FIPS_rodata_end[];
extern unsigned char        FIPS_signature[SIGNATURE_SIZE];
extern unsigned int         FIPS_incore_fingerprint (unsigned char *, unsigned int);

static unsigned char        Calculated_signature[SIGNATURE_SIZE];

int const OpenSSLError = 0;
int const OpenSSLSuccess = 1;
unsigned char *_initializedPin;

unsigned char * formattedKey[256];


#define MAX_KEY_LENGTH 32
#define SALT_LENGTH 8

#define EVP_aes_256_cbc_Key_LENGTH 32
#define EVP_aes_256_cbc_Iv_LENGTH 16

#define CANNNED_SALT {0x39, 0x30, 0x00, 0x00, 0x31, 0xD4, 0x00, 0x00}
#define CANNED_RI_KEY_BYTES "password"

#define KEY_MASTER_KEY "MasterKey"
#define KEY_BACKUP_KEY "BackupKey"
#define KEY_DATABASE_PIN "KEY_DATABASE_PIN"
#define KEY_DATABASE_CHECK "KEY_DATABASE_CHECK"

const char KEY_SALT[] = "KEY_SALT";

#define KCHECK_VALUE "check"

int const T2Error = -1;
int const T2Success = 0;

int const T2True = 1;
int const T2False = 0;

int larry = 0;

unsigned char *_salt;
boolean useTestVectors;
boolean verboseLogging;


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


// Function prototypes
void logAsHexString(unsigned char * bin, unsigned int binsz, char * message);
void generateMasterOrRemoteKeyAndSave(JNIEnv* env, T2Key * RIKey, T2Key * LockingKey, const char * keyType);
unsigned char * encryptUsingKey_malloc(T2Key * credentials, unsigned char * pUencryptedText, int *outLength);
unsigned char * decryptUsingKey_malloc(T2Key * credentials, unsigned char * encryptedText);
int key_init(unsigned char * key_data, int key_data_len, unsigned char * salt, T2Key * aCredentials);
unsigned char * aes_decrypt_malloc(EVP_CIPHER_CTX * decryptContext, unsigned char * ciphertext, int *len);
unsigned char * aes_encrypt_malloc(EVP_CIPHER_CTX * encryptContext , unsigned char * plaintext, int * len);
unsigned char * binAsHexString_malloc(unsigned char * bin, unsigned int binsz);
int getRIKeyUsing(JNIEnv* env, T2Key * RiKey, char * answersOrPin, char * keyType);
int checkPin_I(JNIEnv* env, unsigned char *pin);
int checkAnswers_I(JNIEnv* env, unsigned char *answers);
jint Java_com_t2_fcads_FipsWrapper_checkPin( JNIEnv* env,jobject thiz, jstring jPin);
jint Java_com_t2_fcads_FipsWrapper_checkAnswers( JNIEnv* env,jobject thiz, jstring jAnswers);

// Macro to throw java exction from "c" code
#define THROW_T2_EXCEPTION(_LABEL_) \
    jclass cls = (*env)->FindClass(env, "java/lang/RuntimeException");  \
    if (cls == 0) {                                                     \
      (*env)->ThrowNew(env, cls, "Cannot Find RuntimeException class"); \
    } else {                                                            \
      (*env)->ThrowNew(env, cls, _LABEL_);                              \
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
  jclass cls = (*env)->FindClass(env, "com/t2/fcads/FipsWrapper");
  if (cls == 0) {
      T2Assert((cls != 0), "Can't find class FipsWrapper");
      return;
  }

  jmethodID id = (*env)->GetStaticMethodID(env, cls, "putString", "(Ljava/lang/String;Ljava/lang/String;)V");
  if (id == 0) {
    T2Assert((cls != 0), "Can't find method putString");
    return;
  }

  jstring key = (*env)->NewStringUTF(env, pKey);
  jstring value = (*env)->NewStringUTF(env, pValue);

  (*env)->CallStaticVoidMethod(env, cls, id, key, value);
} 

/*!
 * @brief Retrieves a string from Android sharedPReferences via Java call;
 * @discussion Corresponding java method is static String com.t2.fcads.getString(String key) 
 * @param env Jni environment
 * @param pKey Key Key of variable to retrieve
 * @return  String value associated with key
 */
const char * getString(JNIEnv* env, const char *pKey) {
  jstring key = (*env)->NewStringUTF(env, pKey);

  jclass cls = (*env)->FindClass(env, "com/t2/fcads/FipsWrapper");
  if (cls == 0) {
      T2Assert((cls != 0), "Can't find class FipsWrapper");
      return;
  }

  jmethodID id = (*env)->GetStaticMethodID(env, cls, "getString", "(Ljava/lang/String;)Ljava/lang/String;");
  if (id == 0) {
    T2Assert((cls != 0), "Can't find method getString");
    return;
  }

  jstring returnedSalt = (*env)->CallStaticObjectMethod(env, cls, id, key);

  const char *utfChars = (*env)->GetStringUTFChars(env, returnedSalt, 0);

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
  jclass cls = (*env)->FindClass(env, "com/t2/fcads/FipsWrapper");
  if (cls == 0) {
      T2Assert((cls != 0), "Can't find class FipsWrapper");
      return;
  }

  jmethodID id = (*env)->GetStaticMethodID(env, cls, "putData", "(Ljava/lang/String;[B)V");
  if (id == 0) {
    T2Assert((cls != 0), "Can't find method putData");
    return;
  }

  jstring key = (*env)->NewStringUTF(env, pKey);

  jbyteArray retArray;
  if(!retArray) {
    retArray = (*env)->NewByteArray(env, data_size);    
  }

  if((*env)->GetArrayLength(env, retArray) != data_size) {
      (*env)->DeleteLocalRef(env, retArray);
      retArray = (*env)->NewByteArray(env, data_size);
  }

  void *temp = (*env)->GetPrimitiveArrayCritical(env, (jarray)retArray, 0);
  memcpy(temp, pValue, data_size);
  (*env)->ReleasePrimitiveArrayCritical(env, retArray, temp, 0);

  (*env)->CallStaticVoidMethod(env, cls, id, key, retArray);
} 

/*!
 * @brief Retrieves bytes from Android sharedPReferences via Java call;
 * @discussion Corresponding java method is static String com.t2.fcads.getBytes(String key) 
 * @param env Jni environment
 * @param pKey Key Key of bytes to retrieve
 * @return  Bytes associated with key
 */
char * getData(JNIEnv* env, const char *pKey, int * length) {
  jclass cls = (*env)->FindClass(env, "com/t2/fcads/FipsWrapper");
  if (cls == 0) {
      T2Assert((cls != 0), "Can't find class FipsWrapper");
      return;
  }

  jmethodID id = (*env)->GetStaticMethodID(env, cls, "getData", "(Ljava/lang/String;)[B");
  if (id == 0) {
    T2Assert((cls != 0), "Can't find method getData");
    return;
  }

  jstring key = (*env)->NewStringUTF(env, pKey);


  jbyteArray ar = (*env)->CallStaticObjectMethod(env, cls, id, key);
  jboolean isCopy;
  char *returnedValue = (*env)->GetByteArrayElements(env, ar, &isCopy);
  *length = (*env)->GetArrayLength(env, ar);


  return returnedValue;
}

void clearAllData(JNIEnv* env) {
  jclass cls = (*env)->FindClass(env, "com/t2/fcads/FipsWrapper");
  if (cls == 0) {
      T2Assert((cls != 0), "Can't find class FipsWrapper");
      return;
  }

  jmethodID id = (*env)->GetStaticMethodID(env, cls, "clearAllData", "()V");
  if (id == 0) {
    T2Assert((cls != 0), "Can't find method clearAllData");
    return;
  }

  (*env)->CallStaticVoidMethod(env, cls, id);
} 



// void testDataMEthods(JNIEnv* env) {
//       // Test putString, getString, putData, and getData
//     const char testStringKey[] = "StringKey";
//     const char testStringValue[] = "Test 1";

//     const char testDataKey[] = "DataKey";
//     const char testDataValue[] = {'s', 'c', 'o', 't', 0};
//     int testDataValueSize = sizeof(testDataValue);

//     LOGI("sending to putString  %s", testStringValue);
//     putString(env, testStringKey, testStringValue );
//     const char *pReturnString = getString(env, testStringKey);
//     LOGI("getString returned  %s", pReturnString);

//     LOGI("sending to putData  %s", testDataValue);
//     putData(env, testDataKey, testDataValue, testDataValueSize );
//     char *pReturnBytes = getData(env, testDataKey);
//     LOGI("getData returned  %s", pReturnBytes);

// }

// --------------------------------------------------------------
// ------------ t2crypto methods
// --------------------------------------------------------------

void Java_com_t2_fcads_FipsWrapper_setVerboseLogging(JNIEnv* env,jobject thiz, jboolean jvl) {
  int vl = (boolean) jvl;
  verboseLogging = vl;
}



void Java_com_t2_fcads_FipsWrapper_init( ) {
  verboseLogging = T2False;
  LOGI("INFO: Initializing");
  _salt = malloc(sizeof(unsigned char) * SALT_LENGTH);
}


/*!
 * @brief Prepares keys - MUST BE CALLED BEFORE ANYTHING ELSE!
 * @param env Jni environment
 * @param textVectors - Boolean telling whtehter to use test vectors or random bytes for key generation
 * @discussion This may also be called to change use of test vectors or not
  */
void Java_com_t2_fcads_FipsWrapper_prepare( JNIEnv* env,jobject thiz, jboolean testVectors) {
   // int useTestVectors = (jboolean)(jTestVectors != JNI_FALSE);
    useTestVectors = (boolean) testVectors;

    if (useTestVectors) {
        LOGI("Java_com_t2_fcads_FipsWrapper_prepare, useTestVectors  = TRUE"); 
    }
    else {
       LOGI("Java_com_t2_fcads_FipsWrapper_prepare, useTestVectors  = FALSE"); 
    }
}


void Java_com_t2_fcads_FipsWrapper_deInitializeLogin( JNIEnv* env,jobject thiz ) {

  LOGI("Java_com_t2_fcads_FipsWrapper_deInitializeLogin");
  clearAllData(env);

}



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
    const char *oldPin= (*env)->GetStringUTFChars(env, jOldPin, 0);
    const char *newPin = (*env)->GetStringUTFChars(env, jNewPin, 0);

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

    // Encrypt the PIN and save to NVM
    // NSMutableData *encryptedPIN = [self encryptUsingKey:rIKey_1 :newPin];
    // NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    // [defaults setObject:encryptedPIN forKey:KEY_DATABASE_PIN];
    // [defaults synchronize];

    int outLength;
    unsigned char *encryptedPin = encryptUsingKey_malloc(rIKey_1, (unsigned char *) newPin, &outLength);
    LOGI("outLength = %d", outLength);
     logAsHexString(encryptedPin, outLength, "    Encrypted pin ");

    putData(env, KEY_DATABASE_PIN, encryptedPin, outLength );
    free(encryptedPin);


    
    EVP_CIPHER_CTX_cleanup(&rIKey_1->encryptContext);
    EVP_CIPHER_CTX_cleanup(&rIKey_1->decryptContext);
    EVP_CIPHER_CTX_cleanup(&LockingKey.encryptContext);
    EVP_CIPHER_CTX_cleanup(&LockingKey.decryptContext);
    
    // Clean up jni variables
    (*env)->ReleaseStringUTFChars(env, jOldPin, oldPin);
    (*env)->ReleaseStringUTFChars(env, jNewPin, newPin);

    return T2Success;

}

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
    const char *answers= (*env)->GetStringUTFChars(env, jAnswers, 0);
    const char *newPin = (*env)->GetStringUTFChars(env, jNewPin, 0);

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
    unsigned char *encryptedPin = encryptUsingKey_malloc(rIKey_1, (unsigned char *) newPin, &outLength);
    LOGI("outLength = %d", outLength);
     logAsHexString(encryptedPin, outLength, "    Encrypted pin ");

    putData(env, KEY_DATABASE_PIN, encryptedPin, outLength );
    free(encryptedPin);
    
    // Clean up jni variables
    (*env)->ReleaseStringUTFChars(env, jAnswers, answers);
    (*env)->ReleaseStringUTFChars(env, jNewPin, newPin);

    EVP_CIPHER_CTX_cleanup(&rIKey_1->encryptContext);
    EVP_CIPHER_CTX_cleanup(&rIKey_1->decryptContext);
    EVP_CIPHER_CTX_cleanup(&LockingKey.encryptContext);
    EVP_CIPHER_CTX_cleanup(&LockingKey.decryptContext);
    
    return T2Success;

}

/*!
 * @brief performs initialization and login using pin and answers
 * @param env Jni environment
 * @discussion This will set up the encrypted Master Key and Backup Key
 *             and save them  to NVM
 * @param jpin User supplied pin
 * @param jAnswers User supplied answers
 * @answers Concatenated answers
 * @result Master Key is saved to NVM
 * @result Backup Key is saved to NVM
 * @return T2Success if successful
 */
jint Java_com_t2_fcads_FipsWrapper_initializeLogin( JNIEnv* env,jobject thiz, 
          jstring jPin,
          jstring jAnswers
           ) {
  T2Key RIKey;
  T2Key LockingKey;
  T2Key SecondaryLockingKey;    
 
  // Retrieve jni variables
  const char *pin= (*env)->GetStringUTFChars(env, jPin, 0);
  const char *answers = (*env)->GetStringUTFChars(env, jAnswers, 0);

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
      RIKeyBytes = malloc(sizeof(char) * strlen(CANNED_RI_KEY_BYTES));
      T2Assert((RIKeyBytes != NULL), "Memory allocation error for RIKey!");
      
      T2Assert(_salt != NULL, "Memory allocation error - Memory not allocated")
      unsigned char tmp[] = CANNNED_SALT;
      memcpy(_salt,tmp,SALT_LENGTH);
      
      char tmp1[] = CANNED_RI_KEY_BYTES;
      strcpy(RIKeyBytes, tmp1);
      RIKeyBytesLen = strlen(RIKeyBytes);
    }
    else {
        LOGI(" -- Using random vectors for key ---");
        RAND_poll();
        // Note _salt is malloc'ed in init but key may be different length so we need to do that here
        int result = RAND_bytes(_salt, SALT_LENGTH);
        T2Assert(result == OpenSSLSuccess, "ERROR: - Can't calculate random salt");
        
        RIKeyBytes = malloc(sizeof(char) * MAX_KEY_LENGTH);
        result = RAND_bytes((unsigned char*)RIKeyBytes, MAX_KEY_LENGTH);
        T2Assert(result == OpenSSLSuccess, "ERROR: - Can't calculate RIKeyBytes");
        RIKeyBytesLen = MAX_KEY_LENGTH;
    }

    // Now save the salt
    // TOTO: need to  encrypt it??
    putData(env, KEY_SALT, _salt, SALT_LENGTH );

    // Generate RIKey
    // RIKey is the main key used to encrypt and decrypt everything. Note, it is never stored unencrypted.
    // Only the master key and backup key are stored, from which the RIKey can be derived using
    // the pin and answers respectively.
    // -----------------
    LOGI("INFO: *** Generating RIKey ***");
    /* gen key and iv. init the cipher ctx object */
    if ((key_init((unsigned char*) RIKeyBytes, RIKeyBytesLen, (unsigned char *) _salt, &RIKey))) {
        free(RIKeyBytes);
        T2Assert(0, "ERROR: initializing key");
        // Clean up jni variables
        (*env)->ReleaseStringUTFChars(env, jPin, pin);
        (*env)->ReleaseStringUTFChars(env, jAnswers, answers);            
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
            (*env)->ReleaseStringUTFChars(env, jPin, pin);
            (*env)->ReleaseStringUTFChars(env, jAnswers, answers);            
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
            (*env)->ReleaseStringUTFChars(env, jPin, pin);
            (*env)->ReleaseStringUTFChars(env, jAnswers, answers);            
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
    unsigned char *encryptedPin = encryptUsingKey_malloc(&RIKey, (unsigned char *) pin, &outLength);
    LOGI("outLength = %d", outLength);
     logAsHexString(encryptedPin, outLength, "    Encrypted pin ");

    putData(env, KEY_DATABASE_PIN, encryptedPin, outLength );
    free(encryptedPin);

    unsigned char *encryptedCheck = encryptUsingKey_malloc(&RIKey, (unsigned char *) KCHECK_VALUE, &outLength);
    putData(env, KEY_DATABASE_CHECK, encryptedCheck, outLength );
    free(encryptedCheck);
    
    EVP_CIPHER_CTX_cleanup(&RIKey.encryptContext);
    EVP_CIPHER_CTX_cleanup(&RIKey.decryptContext);
    EVP_CIPHER_CTX_cleanup(&LockingKey.encryptContext);
    EVP_CIPHER_CTX_cleanup(&LockingKey.decryptContext);
    EVP_CIPHER_CTX_cleanup(&SecondaryLockingKey.encryptContext);
    EVP_CIPHER_CTX_cleanup(&SecondaryLockingKey.decryptContext);
    
    // Clean up jni variables
    (*env)->ReleaseStringUTFChars(env, jPin, pin);
    (*env)->ReleaseStringUTFChars(env, jAnswers, answers);
    return T2Success;
}

jint Java_com_t2_fcads_FipsWrapper_isInitialized( JNIEnv* env,jobject thiz ) {
    int length;
    unsigned char *masterKey = getData(env, KEY_MASTER_KEY, &length);
    logAsHexString(masterKey, length, "Masterkey =  ");
    if (strcmp(masterKey, "") == 0)  {
      return T2False;
    } else {
      return T2True;
    }
}

jstring Java_com_t2_fcads_FipsWrapper_keyInit( JNIEnv* env, jobject thiz, jint jKeyLen, jstring jSalt, jstring jCredentials ) {

    int keyLen = (int) jKeyLen;
    const char *salt = (*env)->GetStringUTFChars(env, jSalt, 0);
    const char *credentials = (*env)->GetStringUTFChars(env, jCredentials, 0);

    LOGI("Java_com_t2_fcads_FipsWrapper_keyInit, keyLen = %d, salt = %s, credentials = %s", keyLen, salt, credentials);

    (*env)->ReleaseStringUTFChars(env, jSalt, salt);
    (*env)->ReleaseStringUTFChars(env, jCredentials, credentials);
    return (*env)->NewStringUTF(env, "testReturn_keyInit");
}

jstring Java_com_t2_fcads_FipsWrapper_getDatabaseKeyUsingPin(JNIEnv* env, jobject thiz, jstring jPin) {

    T2Key aRIKey;
    T2Key *RIKey = &aRIKey;    formattedKey[0] = 0;   // Clear out formatted key in case we fail


    // Retrieve jni variables
    const char *pin= (*env)->GetStringUTFChars(env, jPin, 0);
 
    // Make sure the pin is correct
    int result = checkPin_I(env, (unsigned char*) pin);
   
    if (result == T2Success) {
      // Pin is correct, now get the RIKey based in the given pin
      result = getRIKeyUsing(env, RIKey, (char*) pin, (char *) KEY_MASTER_KEY);

      if (result == T2Success) {
          
          // RIKey is now set up
          // Now create the database key by appending the key and iv

          unsigned char *RIKeyAndIv = malloc((RIKey->keyLength + RIKey->ivLength) + 1);
          int i;
          for (i = 0; i < RIKey->keyLength; i++) {
              RIKeyAndIv[i] = RIKey->key[i];
          }
          for (i = 0; i < RIKey->ivLength; i++) {
              RIKeyAndIv[i + RIKey->keyLength] = RIKey->iv[i];
          }
          RIKeyAndIv[RIKey->keyLength + RIKey->ivLength] = 0; // Terminate it with a zero
        
          unsigned char *result = binAsHexString_malloc(RIKeyAndIv, RIKey->keyLength + RIKey->ivLength);
        
          // Note that the format of this string is specific
          // IT tells SQLCipher to use this key directly instead of
          // using it as a password to derive a key from 
           sprintf((char*) formattedKey, "x'%s'", result);

          free(result);
          free(RIKeyAndIv);
          EVP_CIPHER_CTX_cleanup(&RIKey->encryptContext);
          EVP_CIPHER_CTX_cleanup(&RIKey->decryptContext);

      } 

    }
    
    // Clean up jni variables
    (*env)->ReleaseStringUTFChars(env, jPin, pin);
    return (*env)->NewStringUTF(env, (char*)formattedKey);

}

// ------------------------------------
// FIPS  Interface Routines
// ------------------------------------
jint Java_com_t2_fcads_FipsWrapper_FIPSmode( JNIEnv* env, jobject thiz ) {
    return MY_FIPS_mode();
}

jstring Java_com_t2_fcads_FipsWrapper_T2FIPSVersion( JNIEnv* env, jobject thiz ) {
    return (*env)->NewStringUTF(env, "1.3.0");
}

// ------------------------------------
// Internal Routines
// ------------------------------------

int MY_FIPS_mode() {
    int i;
    int len = 0;
    int mode = 0;
    int ret = 0;
  const unsigned int p1 = (unsigned int)FIPS_rodata_start;
  const unsigned int p2 = (unsigned int)FIPS_rodata_end;

  LOGI(".rodata start: 0x%08x\n", p1);
  LOGI(".rodata end: 0x%08x\n", p2);
 
  logAsHexString(FIPS_signature, SIGNATURE_SIZE, "embedded fingerprint  ");
 
  len = FIPS_incore_fingerprint(Calculated_signature, sizeof(Calculated_signature));
  
  if (len < 0) {
    LOGI("Failed to calculate expected signature");
    return -1;
  }

  logAsHexString(Calculated_signature, len, "calculated fingerprint");


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
      ERR_load_crypto_strings();
      ERR_print_errors_fp(stderr);
    }
  }
   return mode;
}


unsigned char * aes_encrypt_malloc(EVP_CIPHER_CTX * encryptContext , unsigned char * plaintext, int * len) {
    /* max ciphertext len for a n bytes of plaintext is n + AES_BLOCK_SIZE -1 bytes */
    int c_len = *len + AES_BLOCK_SIZE, f_len = 0;
    unsigned char *pCiphertext = malloc(c_len);
    
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

unsigned char * aes_decrypt_malloc(EVP_CIPHER_CTX * decryptContext, unsigned char * ciphertext, int *len) {
    /* plaintext will always be equal to or lesser than length of ciphertext*/
    int p_len = *len, f_len = 0;
    unsigned char *plaintext = malloc(p_len);
    
    EVP_DecryptInit_ex(decryptContext, NULL, NULL, NULL, NULL);
    EVP_DecryptUpdate(decryptContext, plaintext, &p_len, ciphertext, *len);
    EVP_DecryptFinal_ex(decryptContext, plaintext+p_len, &f_len);
    
    *len = p_len + f_len;
    return plaintext;
}

unsigned char * encryptUsingKey_malloc(T2Key * credentials, unsigned char * pUencryptedText, int * outLength) {
   
    int len1 = strlen(pUencryptedText) + 1; // Make sure we encrypt the terminating 0 also!
    unsigned char* szEncryptedText =  aes_encrypt_malloc(&credentials->encryptContext, pUencryptedText, &len1);
    *outLength = len1;
    return szEncryptedText;
}

unsigned char * decryptUsingKey_malloc(T2Key * credentials, unsigned char * encryptedText) {
    int len1 = strlen(encryptedText) + 1; // Make sure we encrypt the terminating 0 also!
    unsigned char* decryptedText =  aes_decrypt_malloc(&credentials->decryptContext, encryptedText, &len1);
    return decryptedText;
}

void generateMasterOrRemoteKeyAndSave(JNIEnv* env, T2Key * RIKey, T2Key * LockingKey, const char * keyType) {

    // This input to encrypt wil be the RIKey (key and iv concatenated)
    int i; 
    unsigned char *RIKeyAndIv = malloc((RIKey->ivLength + RIKey->keyLength) + 1);
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
 
    free (RIKeyAndIv);

    logAsHexString((unsigned char *) rawMasterKey, RIKey->ivLength + RIKey->keyLength,  (char *)keyType);

    // Save Master Key to User Preferences
    // ------------------------------
    LOGI("INFO:   *** Save %s Key to User Preferences ***", keyType);
    LOGI("INFO:   length = %d", RIKey->ivLength + RIKey->keyLength);

    putData(env, keyType, rawMasterKey, RIKey->ivLength + RIKey->keyLength);
    
    free(rawMasterKey);
}

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
    
    logAsHexString(( unsigned char *)aCredentials->key, aCredentials->keyLength, "    key");
    logAsHexString((unsigned char *)aCredentials->iv, aCredentials->ivLength, "     iv");
    
    // Setup encryption context
    EVP_CIPHER_CTX_init(&aCredentials->encryptContext);                                     // Initialize ciipher context
    EVP_EncryptInit_ex(&aCredentials->encryptContext, EVP_aes_256_cbc(), NULL, aCredentials->key, aCredentials->iv);    // Set up context to use specific cyper type
    
    EVP_CIPHER_CTX_init(&aCredentials->decryptContext);                                     // Initialize ciipher context
    EVP_DecryptInit_ex(&aCredentials->decryptContext, EVP_aes_256_cbc(), NULL, aCredentials->key, aCredentials->iv);    // Set up context to use specific cyper type
    
    return T2Success;
}

int keyCredentialsFromBytes(unsigned char * key_data, int key_data_len, int iv_data_len, T2Key * aCredentials) {

    int i;
    for (i = 0; i < key_data_len; i++) {
        aCredentials->key[i] = key_data[i];
    }
    
    for (i = 0; i < iv_data_len; i++) {
        aCredentials->iv[i] = key_data[i + key_data_len];
    }
    
    logAsHexString((unsigned char *)aCredentials->key, key_data_len, "    key");
    logAsHexString((unsigned char *)aCredentials->iv, iv_data_len, "   iv");
    
    // Setup encryption context
    EVP_CIPHER_CTX_init(&aCredentials->encryptContext);                                     // Initialize ciipher context
    EVP_EncryptInit_ex(&aCredentials->encryptContext, EVP_aes_256_cbc(), NULL, aCredentials->key, aCredentials->iv);    // Set up context to use specific cyper type
    
    EVP_CIPHER_CTX_init(&aCredentials->decryptContext);                                     // Initialize ciipher context
    EVP_DecryptInit_ex(&aCredentials->decryptContext, EVP_aes_256_cbc(), NULL, aCredentials->key, aCredentials->iv);    // Set up context to use specific cyper type
    
    return T2Success;
}


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
    unsigned char *masterKey = getData(env, keyType, &length);
    logAsHexString(masterKey, length, "INFO: Stored value = ");
    
    // Double check the master key length
    if (length != EVP_aes_256_cbc_Key_LENGTH + EVP_aes_256_cbc_Iv_LENGTH) {
      LOGI(" ERROR master key length incorrect, is: %d, should be: %d", length, EVP_aes_256_cbc_Key_LENGTH + EVP_aes_256_cbc_Iv_LENGTH);
      return T2Error;
    }

    int len = length + 1;
    
    char *RIKeyAndIv = malloc(len);
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
;
    free(RIKeyAndIv);
    
    EVP_CIPHER_CTX_cleanup(&LockingKey.encryptContext);
    EVP_CIPHER_CTX_cleanup(&LockingKey.decryptContext);
    
    return retVal;
}

jint Java_com_t2_fcads_FipsWrapper_checkAnswers( JNIEnv* env,jobject thiz, jstring jAnswers) {
  int retVal = T2Error;

  
  if (!Java_com_t2_fcads_FipsWrapper_isInitialized(env, thiz)) {
    return T2Error;
  }

  const char *answers = (*env)->GetStringUTFChars(env, jAnswers, 0);

  retVal = checkAnswers_I(env, (unsigned char *)answers);

  (*env)->ReleaseStringUTFChars(env, jAnswers, answers);
  return retVal;
}

int checkAnswers_I(JNIEnv* env, unsigned char *answers) {
  int retVal = T2Error;
  T2Key acredential;
  T2Key *rIKey_1 = &acredential;
  
  // Re-initialize salt from nvm
   int length;
  _salt = getData(env, KEY_SALT, &length);
 
  LOGI("INFO: === checkAnswers ===");
  LOGI("INFO: *** Generating Secondary LockingKey  kdf(%s) ***", answers);
  
  // Generate the RIKey based on Pin and Master Key
  int result = getRIKeyUsing(env, rIKey_1, (unsigned char*)answers, KEY_BACKUP_KEY);
  if (result != T2Success) {
    return result;
  }


  EVP_CIPHER_CTX_cleanup(&acredential.encryptContext);
  EVP_CIPHER_CTX_cleanup(&acredential.encryptContext);
  
  // Read the CHECK from NVM and make sure we can decode it properly
  // if not fail login
  unsigned char *encryptedCheck = getData(env, KEY_DATABASE_CHECK, &length);
  if (encryptedCheck == NULL || strcmp(encryptedCheck, "") == 0) {
    return T2Error;
  }
  unsigned char *decryptedCheck = decryptUsingKey_malloc(rIKey_1, encryptedCheck);

  EVP_CIPHER_CTX_cleanup(&rIKey_1->encryptContext);
  EVP_CIPHER_CTX_cleanup(&rIKey_1->decryptContext);
  if (strcmp(decryptedCheck, KCHECK_VALUE) == 0) {
      retVal = T2Success;
  }
  else {
      LOGI("WARNING: answers does not match");
  }

  free(decryptedCheck);


  return retVal;
}


jint Java_com_t2_fcads_FipsWrapper_checkPin( JNIEnv* env,jobject thiz, jstring jPin) {
  if (!Java_com_t2_fcads_FipsWrapper_isInitialized(env, thiz)) {
    return T2Error;
  }

  const char *pin = (*env)->GetStringUTFChars(env, jPin, 0);

  int retVal = checkPin_I(env, (unsigned char *)pin);

  (*env)->ReleaseStringUTFChars(env, jPin, pin);
  return retVal;
}

int checkPin_I(JNIEnv* env, unsigned char *pin) {
    int retVal = T2Error;
    T2Key acredential;
    T2Key *rIKey_1 = &acredential;

    int length;
    // Re-initialize salt from nvm
    _salt = getData(env, KEY_SALT, &length);

    
    LOGI("INFO: === checkPin ===");
    LOGI("INFO: *** Generating LockingKey  kdf(%s) ***", pin);
    
    // Generate the RIKey based on Pin and Master Key
    int result = getRIKeyUsing(env, rIKey_1, pin, KEY_MASTER_KEY);
    if (result != T2Success) {
      return result;
    }

    // Read the PIN from NVM and make sure we can decode it properly
    // if not fail login
    unsigned char *encryptedPin = getData(env, KEY_DATABASE_PIN, &length);
    if (encryptedPin == NULL || strcmp(encryptedPin, "") == 0) {
      return T2Error;
    }
    unsigned char *decryptedPIN = decryptUsingKey_malloc(rIKey_1, encryptedPin);

    EVP_CIPHER_CTX_cleanup(&acredential.encryptContext);
    EVP_CIPHER_CTX_cleanup(&acredential.encryptContext);
  
    EVP_CIPHER_CTX_cleanup(&rIKey_1->encryptContext);
    EVP_CIPHER_CTX_cleanup(&rIKey_1->decryptContext);

    if (strcmp(decryptedPIN, pin) == 0) {
        _initializedPin = pin;
        retVal = T2Success;
    }
    else {
        LOGI("WARNING: PIN does not match");
    }

    free(decryptedPIN);
    return retVal;
}


unsigned char * binAsHexString_malloc(unsigned char * bin, unsigned int binsz) {
    
    unsigned char *result;
    char          hex_str[]= "0123456789abcdef";
    unsigned int  i;
    
    result = malloc(binsz * 2 + 1);
    (result)[binsz * 2] = 0;
    
    if (!binsz)
        return NULL;
    
    for (i = 0; i < binsz; i++) {
        (result)[i * 2 + 0] = hex_str[bin[i] >> 4  ];
        (result)[i * 2 + 1] = hex_str[bin[i] & 0x0F];
    }
    
    return result;
    
}

void logAsHexString(unsigned char * bin, unsigned int binsz, char * message) {

    char *result;
    char          hex_str[]= "0123456789abcdef";
    unsigned int  i;
    
    result = (char *)malloc(binsz * 2 + 1);
    (result)[binsz * 2] = 0;
    
    if (!binsz)
        return;
    
    for (i = 0; i < binsz; i++) {
        (result)[i * 2 + 0] = hex_str[bin[i] >> 4  ];
        (result)[i * 2 + 1] = hex_str[bin[i] & 0x0F];
    }
    
    printf("   %s = : %s \n", message, result);
    LOGI("   %s = : %s \n", message, result);


    free(result);
   
}

