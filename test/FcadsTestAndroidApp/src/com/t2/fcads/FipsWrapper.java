/*****************************************************************
FipsWrapper

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
package com.t2.fcads;

import android.content.Context;
import android.content.SharedPreferences;
import android.util.Base64;
import android.util.Log;

//JNI Stuff
//Assumptions, The project includes:
//    1. A JAVA class FipsWrapper.java in package com.t2.fcads
//    2. A native file called wrapper.c
//
//To add a static java method that a native routine can call
//    1. Add method to class com.t2.fcads 
//        take note of it's signature, it will be used indirectly in the native code
//        in the method call GetDtaticMethodId.
//    2. In the native code call like this:
//        jclass cls = (*env)->FindClass(env, "com/t2/fcads/FipsWrapper");
//        jmethodID id = (*env)->GetStaticMethodID(env, cls, "clearAllData", "()V");
//        (*env)->CallStaticVoidMethod(env, cls, id);
//
//    2a. Note in this case that GetStaticMethodID is looking for the method called
//       clearAllData that returns void (The V after parentheses)
//       and is send no parameters (empty parentheses).
//
//       Here are a couple of examples of how signatures translate to the codes used in GetStaticMethodID.
//        "(Ljava/lang/String;Ljava/lang/String;)V"   ->> void putString(String key, String value)
//        "(Ljava/lang/String;)Ljava/lang/String;"    ->> String getString(String key)
//        "(Ljava/lang/String;[B)V"                   ->> void putData(String key, byte[] values)
//        "(Ljava/lang/String;)[B"                    ->> byte[] getData(String key)
//
//To add a native method callable from JAVA
//    1. Add method to native file, ex:
//        jstring Java_com_t2_fcads_FipsWrapper_example(JNIEnv* env, jobject thiz, jstring jPin) {
//            const char *pin= (*env)->GetStringUTFChars(env, jPin, 0);
//            (*env)->ReleaseStringUTFChars(env, jPin, pin);
//            return (*env)->NewStringUTF(env, (char*)formattedKey);
//        }        
//    1a. Note that the method must be named according to the class and package name that will be calling it:
//            JAVA_{package name}_{class name}_{method name}
//        in the above: 
//            package = com_t2_fcads
//            class = FipsWrapper
//            method = example
//    1b. It is important that you use (*env)->Getxx() and (*env)->Release() before using variables
//        and that you only return environment variables ex:
//            return (*env)->NewStringUTF(env, (char*)formattedKey);
//    2. In the project class that will be calling the native method add a declaration of the method
//        public native String example(String pin);
//
//        You can then call the native method from that JAVA class, ex:
//            String result = example(new String("string to process"));


/**
 * @author scoleman
 * 
 * This is the official JAVA wrapper for the wrapper.c
 * native function FIPSmode()
 *
 */
public class FipsWrapper {
	private static FipsWrapper instance = null;
	public native int  FIPSmode();
	public native String  T2FIPSVersion();
	public native int  initializeLogin(String pin, String answers);
	public native void  prepare(boolean useTestVectors);
	public native void  init();
	public native void  cleanup();
	public native String getDatabaseKeyUsingPin(String pin);
	public native int checkAnswers(String answers);
	public native int checkPin(String pin);
	public native int isInitialized();
	public native void deInitializeLogin();
	public native int changePinUsingPin(String oldPin, String newPin);
	public native int changePinUsingAnswers(String newPin, String answers);
	public native int setVerboseLogging(boolean verboseLogging);

	public FipsWrapper() {
		init();		
	}
	
	
	@Override
	protected void finalize() throws Throwable {
		cleanup();
		super.finalize();
	}


	static Context context;
	
	public static void setContext(Context cxt) {
		context = cxt;
	}
	
	
	public static FipsWrapper getInstance() {
		if (instance == null) {
			instance = new FipsWrapper();
		}
		return instance;
	}
	
// Routines for t2cryptyo

	
	public int doSetVerboseLogging(boolean vb) {
		return setVerboseLogging(vb);
	}
	
	public int doIsInitialized() {
		return isInitialized();
	}
	
	public void doPrepare(boolean useTestVectors) {
		prepare(useTestVectors);
	}
	
	public int doInitializeLogin(String pin, String answers)  {
		return initializeLogin(pin, answers);

	}
	public String  doGetDatabaseKeyUsingPin(String pin) {
		// Corresponds to Java_com_t2_fcads_ggetDatabseKeyUsingPin(JNIEnv* env, jobject thiz, jstring jPin)
		return getDatabaseKeyUsingPin(pin);
	}
	
	public int doCheckAnswers(String answers) {
		return checkAnswers(answers);
	}
	
	public int doCheckPin(String pin) {
		return checkPin(pin);
	}
		
	public void doDeInitializeLogin() {
		deInitializeLogin();
	}
	
	public int doChangePinUsingPin(String oldPin, String newPin) {
		return changePinUsingPin(oldPin,  newPin);
	}
	
	public int doChangePinUsingAnswers(String newPin, String answers) {
		return changePinUsingAnswers(newPin, answers);
	}
	
    /**
     * calls the native function FIPSmode
     * 
     * @return return value from jni FIPSmode
     *         1 = success
     */
    public int doFIPSmode() {
    	return FIPSmode();
    }

    
    /**
     * calls the native function T2FIPSVersion
     * 
     * @return return value from jni T2FIPSVersion - Version number of T2 FIPS build
     * 
     */    
    public String doT2FIPSVersion() {
    	return T2FIPSVersion();
    }
     
     
 // Routines for native code to call for preferences
    
	static void putString(String key, String value) {
		SharedPreferences sp = context.getSharedPreferences("FIPS_VARS", Context.MODE_PRIVATE);
		sp.edit().putString(key, value).commit();
	}
	
	static String getString(String key) {
		SharedPreferences sp = context.getSharedPreferences("FIPS_VARS", Context.MODE_PRIVATE);
		return sp.getString(key, "");
	}
     
	static void putData(String key, byte[] values) {
		String str = Base64.encodeToString(values,  Base64.NO_WRAP);
		SharedPreferences sp = context.getSharedPreferences("FIPS_VARS", Context.MODE_PRIVATE);
		sp.edit().putString(key, str).commit();
	}
	
	static byte[] getData(String key) {
		SharedPreferences sp = context.getSharedPreferences("FIPS_VARS", Context.MODE_PRIVATE);
		String str = sp.getString(key, "");
		byte[] tmp = Base64.decode(str, Base64.NO_WRAP);
		return Base64.decode(str, Base64.NO_WRAP);
	}
     
	static void clearAllData() {
		SharedPreferences sp = context.getSharedPreferences("FIPS_VARS", Context.MODE_PRIVATE);
		sp.edit().clear().commit();
	}
	
     
     
     
     
     
     
}
