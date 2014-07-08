#include <string.h>
#include <jni.h>
#include <openssl/evp.h>
#include <android/log.h>



#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "native-activity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "native-activity", __VA_ARGS__))

extern const void*          FIPS_text_start(),  *FIPS_text_end();
extern const unsigned char  FIPS_rodata_start[], FIPS_rodata_end[];

extern unsigned char        FIPS_signature[20];
extern unsigned int         FIPS_incore_fingerprint (unsigned char *, unsigned int);

static unsigned char        Calculated_signature[20];


int MY_FIPS_mode()
    {
	int i;
	int len = 0;
	int mode = 0;
	int ret = 0;
  const unsigned int p1 = (unsigned int)FIPS_rodata_start;
  const unsigned int p2 = (unsigned int)FIPS_rodata_end;

  LOGI(".rodata start: 0x%08x\n", p1);
  LOGI(".rodata end: 0x%08x\n", p2);
  
    LOGI("Embedded: ");
  for(i = 0; i < 20; ++i) {
    LOGI("%02x", FIPS_signature[i]);
  }
  LOGI("\n");
  
  
  len = FIPS_incore_fingerprint(Calculated_signature, sizeof(Calculated_signature));
  LOGI("len = %d", len);
  LOGI("sizeof(Calculated_signature) = %d", sizeof(Calculated_signature));
  
  
  if (len < 0) {
    LOGI("Failed to calculate expected signature");
    return -1;
  }

  LOGI("Calculated: ");
  for(i = 0; i < len && i < sizeof(Calculated_signature); ++i) {
    LOGI("%02x", Calculated_signature[i]);
  }
  LOGI("\n");

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



      //  LOGI("accelerometer: x=%d ", 27);
 
       // int ret = 22233;

	//int mode = mode = FIPS_mode();

   return mode;
}

jint 
Java_com_demo_sqlcipher_HelloSQLCipherActivity_Scott( JNIEnv* env,
                                              jobject thiz )
{
  int testValue = 101;
    
    return testValue;
}




jint 
Java_com_demo_sqlcipher_HelloSQLCipherActivity_FIPSFromJNI( JNIEnv* env,
                                              jobject thiz )
{
  int testValue = 101;
    
    return MY_FIPS_mode();
}

jint
Java_com_demo_sqlcipher_HelloSQLCipherActivity_FIPSFromJNIfred( JNIEnv* env,
                                                           jobject thiz )
{
    int testValue = 101;
    
    return MY_FIPS_mode();
}

jint
Java_com_example_fcadstestandroidapp_MainActivity_FIPSFromJNI( JNIEnv* env,
                                              jobject thiz )
{
  int testValue = 101;
    
    return MY_FIPS_mode();
}



jint
Java_com_example_t2testjniwrapper_MainActivity_FIPSFromJNI( JNIEnv* env,
                                              jobject thiz )
{
	int testValue = 101;
    
    return MY_FIPS_mode();
}



jstring
Java_com_example_t2testjniwrapper_MainActivity_testFromJNI( JNIEnv* env,
                                               jobject thiz )
{
    
    
    return (*env)->NewStringUTF(env, "This is a test string for The weekend1");
}