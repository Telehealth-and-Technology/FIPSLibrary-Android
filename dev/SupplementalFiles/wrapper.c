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
   return mode;
}

// This is the official interface for this module
jint
Java_com_t2_fcads_FipsWrapper_FIPSmode( JNIEnv* env,
                                              jobject thiz )
{
        return MY_FIPS_mode();
}

jstring
Java_com_t2_fcads_FipsWrapper_T2FIPSVersion( JNIEnv* env,
                                              jobject thiz )
{
    return (*env)->NewStringUTF(env, "1.3.0");
}



// The rest of these interfaces are used for testing and development 
jint 
Java_com_demo_sqlcipher_HelloSQLCipherActivity_Scott( JNIEnv* env,
                                              jobject thiz )
{
    return MY_FIPS_mode();
}




jint 
Java_com_demo_sqlcipher_HelloSQLCipherActivity_FIPSFromJNI( JNIEnv* env,
                                              jobject thiz )
{
    return MY_FIPS_mode();
}

jint
Java_com_demo_sqlcipher_HelloSQLCipherActivity_FIPSFromJNIfred( JNIEnv* env,
                                                           jobject thiz )
{
    return MY_FIPS_mode();
}

jint
Java_com_example_fcadstestandroidapp_MainActivity_FIPSFromJNI( JNIEnv* env,
                                              jobject thiz )
{
    return MY_FIPS_mode();
}

jint
Java_com_example_fcadstestandroidapp_MainActivity_FIPSmode( JNIEnv* env,
                                              jobject thiz )
{
    return MY_FIPS_mode();
}



jint
Java_com_example_t2testjniwrapper_MainActivity_FIPSFromJNI( JNIEnv* env,
                                              jobject thiz )
{
    return MY_FIPS_mode();
}



jstring
Java_com_example_t2testjniwrapper_MainActivity_testFromJNI( JNIEnv* env,
                                               jobject thiz )
{ 
    return (*env)->NewStringUTF(env, "This is a test string for The weekend1");
}