/*****************************************************************
FipsWrapper

Copyright (C) 2011-2015 The National Center for Telehealth and 
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

import java.nio.ByteBuffer;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.pm.Signature;
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
 *         This is the official JAVA wrapper for the wrapper.c native function
 *         FIPSmode().
 * 
 *         Besides this, This wrapper is the gateway for all t2Crypto (Lost
 *         password implementation) functionality
 * 
 */
public class FipsWrapper {
	private String TAG = FipsWrapper.class.getSimpleName();
	private static FipsWrapper instance = null;

	// Native mathods defined in wrapper.c
	// wrapper.c is complied as part of the main FCADS build process
	 public native int FIPSmode();
	 public native String T2FIPSVersion();
	 public native int initializeLogin(String pin, String answers);
	 public native void prepare(boolean useTestVectors);
	 public native void init();
	 public native void cleanup();
	 public native String getDatabaseKeyUsingPin(String pin);
	 public native int checkAnswers(String answers);
	 public native int checkPin(String pin);
	 public native int isInitialized();
	 public native void deInitializeLogin();
	 public native int changePinUsingPin(String oldPin, String newPin);
	 public native int changePinUsingAnswers(String newPin, String answers);
	 public native int setVerboseLogging(boolean verboseLogging);
	 public native String encrypt(String pin, String plainText);
	 public native String decrypt(String pin, String cipherText);

	/**
	 * Constructor
	 */
	public FipsWrapper() {
		init();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see java.lang.Object#finalize()
	 */
	@Override
	protected void finalize() throws Throwable {
		cleanup();
		super.finalize();
	}

	static Context context;

	public static void setContext(Context cxt) {
		context = cxt;
	}

	/**
	 * @return Instance of FipsWrapper
	 */
	public static FipsWrapper getInstance() {
		if (instance == null) {
			instance = new FipsWrapper();
		}
		return instance;
	}

	// Routines for t2cryptyo

	/**
	 * Sets verbose logging level. Logs all generated and saved key data and
	 * encryption/decryption info
	 * 
	 * @param vb
	 *            true for verboase logging
	 */
	public int doSetVerboseLogging(boolean vb) {
		return setVerboseLogging(vb);
	}

	/**
	 * Checks to see if t2Cryto has been previously initialized
	 * 
	 * @return 1 if initialized, 0 if not
	 */
	public int doIsInitialized() {
		return isInitialized();
	}

	/**
	 * Tells t2Cyrpto whether to use test vectors or random bytes for key/salt
	 * generation if
	 * 
	 * @param useTestVectors
	 *            True to use test vectors, false to use randomy generated
	 *            key/salt
	 */
	public void doPrepare(boolean useTestVectors) {
		prepare(useTestVectors);
	}

	/**
	 * Performs initialization and login using pin and answers This will set up
	 * the encrypted Master Key and Backup Key and save them to NVM
	 * 
	 * @param pin
	 *            User supplied pin
	 * @param answers
	 *            user supplied recovery answers (concatenated into one string)
	 * @return T2Success or T2Error
	 */
	public int doInitializeLogin(String pin, String answers) {
		return initializeLogin(pin, answers);
	}

	/**
	 * Retrieves database key (to use in SqlCipher) using PIN to authenticate
	 * 
	 * @param pin
	 *            Pin to authenticate with
	 * @return Database key
	 */
	public String doGetDatabaseKeyUsingPin(String pin) {
		// Corresponds to Java_com_t2_fcads_ggetDatabseKeyUsingPin(JNIEnv* env,
		// jobject thiz, jstring jPin)
		return getDatabaseKeyUsingPin(pin);
	}

	/**
	 * Checks to see if the answers give and the same answers used to initialize
	 * t2Cypto
	 * 
	 * @param answers
	 *            Answers to check
	 * @return T2Success if answers are correct or T2Error if not
	 */
	public int doCheckAnswers(String answers) {
		return checkAnswers(answers);
	}

	/**
	 * Checks pin or password to see if it matches the latest pin or password
	 * set
	 * 
	 * @param pin
	 *            Pin to check
	 * @return T2Success if pin is correct or T2Error if not
	 */
	public int doCheckPin(String pin) {
		return checkPin(pin);
	}

	/**
	 * Clears all login data
	 */
	public void doDeInitializeLogin() {
		deInitializeLogin();
	}

	/**
	 * 
	 * Changes to a new PIN using the previous PIN to authenticate Generates and
	 * saves new masterKey
	 * 
	 * @param oldPin
	 *            Previous (current) pin
	 * @param newPin
	 *            New pin
	 * @return T2Success or T2Error
	 */
	public int doChangePinUsingPin(String oldPin, String newPin) {
		return changePinUsingPin(oldPin, newPin);
	}

	/**
	 * 
	 * Changes to a new PIN using ANSWERS to authenticate Generates and saves
	 * new backup key
	 * 
	 * @param newPin
	 *            New pin
	 * @param answers
	 *            Answers used to initialize t2Ctrypto
	 * @return T2Success or T2Error
	 */
	public int doChangePinUsingAnswers(String newPin, String answers) {
		return changePinUsingAnswers(newPin, answers);
	}

	/**
	 * calls the native function FIPSmode
	 * 
	 * @return return value from jni FIPSmode 1 = success
	 */
	public int doFIPSmode() {
		return FIPSmode();
	}

	/**
	 * calls the native function T2FIPSVersion
	 * 
	 * @return return value from jni T2FIPSVersion - Version number of T2 FIPS
	 *         build
	 * 
	 */
	public String doT2FIPSVersion() {
		return T2FIPSVersion();
	}

	/**
	 * Encrypts a string using a pin/password
	 * 
	 * @param pin
	 *            Pin to use
	 * @param plainText
	 *            Text to encrypt
	 * 
	 * @return return encrypted version of plainText
	 */
	public String doEncrypt(String pin, String plainText) {
		return encrypt(pin, plainText);
	}

	/**
	 * decrypts a string using a pin/password
	 * 
	 * @param pin
	 *            Pin to use
	 * @param cipherText
	 *            Text to decrypt
	 * 
	 * @return return encrypted version of cipherText
	 */
	public String doDecrypt(String pin, String cipherText) {
		return decrypt(pin, cipherText);
	}

	// Routines for native code to call for preferences
	// NOTE: the native code MUST do encryption/decryption on all keys and
	// values before calling these routines!

	/**
	 * Saves value to NVM using key
	 *   This routine MUST be here for the native code to call for NVM storage
	 *   NOTE: the native code MUST do encryption/decryption on all keys and
	 *         values before calling this routine.
	 *         
	 * @param key Key to use for value
	 * @param value Value to save to NVM
	 */
	static void putString(String key, String value) {
		SharedPreferences sp = context.getSharedPreferences("FIPS_VARS",
				Context.MODE_PRIVATE);
		sp.edit().putString(key, value).commit();
	}

	/**
	 * Retrieves value associated with Key from NVM
	 * 
	 *   This routine MUST be here for the native code to call for NVM storage
	 *   NOTE: the native code MUST do encryption/decryption on all keys and
	 *         values before calling this routine.
	 * @param key Key to use for value
	 * @return Retrieved value
	 */
	static String getString(String key) {
		SharedPreferences sp = context.getSharedPreferences("FIPS_VARS",
				Context.MODE_PRIVATE);
		return sp.getString(key, "");
	}

	/**
	 * Saves value to NVM using key
	 *   This routine MUST be here for the native code to call for NVM storage
	 *   NOTE: the native code MUST do encryption/decryption on all keys and
	 *         values before calling this routine.
	 * @param key Key to use for value
	 * @param values Value to save to NVM
	 */
	static void putData(String key, byte[] values) {
		String str = Base64.encodeToString(values, Base64.NO_WRAP);
		SharedPreferences sp = context.getSharedPreferences("FIPS_VARS",
				Context.MODE_PRIVATE);
		sp.edit().putString(key, str).commit();
	}

	/**
	 * Retrieves value associated with Key from NVM
	 *   This routine MUST be here for the native code to call for NVM storage
	 *   NOTE: the native code MUST do encryption/decryption on all keys and
	 *         values before calling this routine.
	 * @param key Key to use for value
	 * @return Retrieved value
	 */
	static byte[] getData(String key) {
		SharedPreferences sp = context.getSharedPreferences("FIPS_VARS",
				Context.MODE_PRIVATE);
		String str = sp.getString(key, "");
		byte[] tmp = Base64.decode(str, Base64.NO_WRAP);
		return Base64.decode(str, Base64.NO_WRAP);
	}

	/**
	 * Clears all data from NVM (cancels any logins)
	 */
	static void clearAllData() {
		SharedPreferences sp = context.getSharedPreferences("FIPS_VARS",
				Context.MODE_PRIVATE);
		sp.edit().clear().commit();
	}

	/**
	 * Returns a 8 byte salt for native t2Crypto to use with encryption
	 * 
	 * @return salt bytes
	 */
	public static byte[] getPackageBasedSalt() {
		byte[] defaultSalt = new byte[] { (byte) 0x39, (byte) 0x30,
				(byte) 0x00, (byte) 0x00, (byte) 0x31, (byte) 0xD4,
				(byte) 0x00, (byte) 0x00 };

		long hash = 0;
		try {
			// Calculate a 8 byte hash of the signature
			Signature[] sigs = context.getPackageManager().getPackageInfo(
					context.getPackageName(), PackageManager.GET_SIGNATURES).signatures;
			char[] t = sigs[0].toChars();
			int length = t.length;

			hash = 5381;

			for (int i = 0; i < length; i++) {
				hash = ((hash << 5) + hash) + t[i];
			}
			Log.i("FipsWrapper", String.format("salt = %x", hash));

			ByteBuffer buffer = ByteBuffer.allocate(8);
			buffer.putLong(hash);
			return buffer.array();

		} catch (NameNotFoundException e) {
			e.printStackTrace();
			return defaultSalt;
		}
	}

}
