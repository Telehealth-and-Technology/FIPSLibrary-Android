/*****************************************************************
MainActivity

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

package com.example.fcadstestandroidapp;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

import com.t2.fcads.FipsWrapper;

import net.sqlcipher.database.SQLiteDatabase;
import android.app.Activity;
import android.content.ContentValues;
import android.content.Context;
import android.content.pm.PackageManager;
import android.content.pm.Signature;
import android.database.Cursor;
import net.sqlcipher.database.SQLiteException;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.widget.TextView;

public class MainActivity extends Activity
{
	private String TAG = MainActivity.class.getSimpleName();	

	
	TextView textViewMain;
	List<String> textLines;
	private static final String PASSWORD = "test123";
	private static final String DATABASE_NAME = "demo.db";
	private static final int T2Success = 0;
	private static final int T2Error = -1;
	private static final int T2True = 1;
	private static final int T2False = 0;

	private int testPassCount = 0;
	private int testFailCount = 0;
	private int testTotalCount = 0;
	private int iterations = 0;
	private String testDescription;
	private boolean databaseTestsPassed = false;
    
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);
        
        textViewMain = (TextView) findViewById(R.id.textViewMain);   
        textLines = new ArrayList<String>();

        updateView("Initializing sqlcipher");
        InitializeSQLCipher();
        //        reopenDatabase();
        updateView("Checking FIPS signature");     
        updateView("Calling FIPS_mode");      
        
        FipsWrapper fipsWrapper = FipsWrapper.getInstance(this);
        
        FipsWrapper.setContext(this);
        
		Log.d(TAG, "---------------------- Testsing FIPS functionality ---------------------");       
		testDescription = startTest("Test 1.1");
        int result = fipsWrapper.doFIPSmode();	
        String t2FipsVersion = fipsWrapper.doT2FIPSVersion();
		assertT2Test(result == 1, testDescription);
        

        Log.d(TAG, "T2 FIPS Version = " + t2FipsVersion);
        updateView("T2 FIPS Version = " + t2FipsVersion);
		
		Log.d(TAG, "FIPS_mode returned " + result);
		updateView("FIPS_mode returned " + result);  
		if (result == 1) {
			updateView("PASSED - enabled FIPS!");
		}
		else {
			updateView("FAILED - was NOT able to enable FIPS!");
		}
		
		if (databaseTestsPassed == true) {
			updateView("PASSED - sqlCipher database test");		
		}
		else {
			updateView("Failed - sqlCipher database test");
		}

		
		final boolean useTestVectorsFoRiKey = true;
		final boolean useRandomVectorsFoRiKey = false;
		
		// ---------------------------------------------------------------------------------------
		final int NUM_ITERATIONS = 1;
		//fipsWrapper.doPrepare(useRandomVectorsFoRiKey);	
		fipsWrapper.doPrepare(useTestVectorsFoRiKey);
		
		
		
		fipsWrapper.doSetVerboseLogging(true);
		// ---------------------------------------------------------------------------------------
		
		// Test t2Crypto
		String pin = "One";
		String newPin1 = "tWo";
		String newPin2 = "thrEe";
		String newPin3 = "Password is  is four";
		String answers = "TwoThreeFour";
		String badAnswers = "TwoThreeFour1";
		String badPin = "TOne";
		String blankPin = "";
		String crazyPin = "9q34un 9vq34ute noaerpt9uertv[!%!#$^&^*^%&*))%2032498132064684;asdjjf;fsjfd;fkjkflja;fdjfeie11";
		String stringToEncrypt = "This is a string to encrypt";
		String blankStringToEncrypt = "";
		String longStringToEncrypt = "This is a string to encrypt and this is extra stuff to put in the line - ;aklsdfj;akdjs ;falsdjfljafp8934cp5u4pq34np5v89u34n[pa96unvwp5unpvtun53p89v6u[3490[v5[m09805)***H*H&P*)#*Y#Y&T#&YYP#*N#&B$&T$T$&NT$&$T$&N$$N$&$$N)$N)$&$$$)&$)$&$$N$N*$N*$NPU$*NP*$N hi there";

		// Unit test template:		
		//		testDescription = startTest("Test x");
		//		assertT2Test(x = x, testDescription);
		
		
		while(iterations++ < NUM_ITERATIONS) {
		
		Log.d(TAG, "---------------------- Testsing T2Crypto functionality - Iteration " + iterations + "  ---------------------");
	
			int ret;

			
			try {
				
				testDescription = startTest("Test 2.1 - doIsInitialized");
				fipsWrapper.doDeInitializeLogin();
				ret = fipsWrapper.doIsInitialized();
				assertT2Test(ret == T2False, testDescription);
			
				testDescription = startTest("Test 2.2 - doInitializeLogin");
				ret = fipsWrapper.doInitializeLogin(pin, answers);
				assertT2Test(ret == T2Success, testDescription );
				
				// 
				// Purposefully insert the encrypt/decrypt raw tests here to make sure they work before t2Crypto is initialized!
				testDescription = startTest("Test 2.2a - check encryptRaw/decryptRaw");
				String encryptedText = fipsWrapper.doEncryptRaw(crazyPin, stringToEncrypt);
				String decryptedText = fipsWrapper.doDecryptRaw(crazyPin, encryptedText);
				//Log.d(TAG,"java decryptedText = " + decryptedText);
				assertT2Test(stringToEncrypt.equalsIgnoreCase(decryptedText), testDescription) ;					
				
				testDescription = startTest("Test 2.2b - check encrypt/decrypt - blank string");
				encryptedText = fipsWrapper.doEncryptRaw(pin, blankStringToEncrypt);
				decryptedText = fipsWrapper.doDecryptRaw(pin, encryptedText);
				//Log.d(TAG,"java decryptedText = " + decryptedText);
				assertT2Test(blankStringToEncrypt.equalsIgnoreCase(decryptedText), testDescription) ;					


				testDescription = startTest("Test 2.c - check encrypt/decrypt - long string");
				encryptedText = fipsWrapper.doEncryptRaw(blankPin, longStringToEncrypt);
				decryptedText = fipsWrapper.doDecryptRaw(blankPin, encryptedText);
				assertT2Test(longStringToEncrypt.equalsIgnoreCase(decryptedText), testDescription) ;					

				
				
				
				// !!!!This will fail until Bug 3003 gets fixed
				testDescription = startTest("Test 2.d - check encrypt/decrypt - bad pin");
				encryptedText = fipsWrapper.doEncryptRaw(blankPin, longStringToEncrypt);
				try {
					decryptedText = fipsWrapper.doDecryptRaw(crazyPin, encryptedText);
					//decryptedText = fipsWrapper.doDecryptRaw(blankPin, encryptedText);
				} catch (Exception e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
				assertT2Test(!longStringToEncrypt.equalsIgnoreCase(decryptedText), testDescription) ;		
				
				
				testDescription = startTest("Test 2.3 - doIsInitialized");
				ret = fipsWrapper.doIsInitialized();
				assertT2Test(ret == T2True, testDescription );
	
				
				String key = fipsWrapper.doGetDatabaseKeyUsingPin(pin);
				Log.d(TAG,"key = " + key);
	
				testDescription = startTest("Test 2.4 - check answers (bad answers)");
				ret = fipsWrapper.doCheckAnswers(badAnswers);
				assertT2Test(ret == T2Error, testDescription );
				
				testDescription = startTest("Test 2.5 - check answers (good answers)");
				ret = fipsWrapper.doCheckAnswers(answers);
				assertT2Test(ret == T2Success, testDescription );
				
				testDescription = startTest("Test 2.6 - check pin (bad pin)");
				ret = fipsWrapper.doCheckPin(badPin);
				assertT2Test(ret == T2Error, testDescription );		
				
				testDescription = startTest("Test 2.7 - check pin (good pin)");
				ret = fipsWrapper.doCheckPin(pin);
				assertT2Test(ret == T2Success, testDescription);		
				
				testDescription = startTest("Test 2.8 - change pin using pin");
				ret = fipsWrapper.changePinUsingPin(pin, newPin1);
				assertT2Test(ret == T2Success, testDescription );
				
				testDescription = startTest("Test 2.9 - check pin (good pin)");
				ret = fipsWrapper.doCheckPin(newPin1);
				assertT2Test(ret == T2Success, testDescription );	
				
				testDescription = startTest("Test 2.10 - check pin (bad pin)");
				ret = fipsWrapper.doCheckPin(pin);
				assertT2Test(ret == T2Error, testDescription );				
				
				testDescription = startTest("Test 2.11 - changePinUsingAnswers");
				ret  = fipsWrapper.changePinUsingAnswers(newPin3, answers);
				assertT2Test(ret == T2Success, testDescription );	
				
				testDescription = startTest("Test 2.12 - check pin (bad pin)");
				ret = fipsWrapper.doCheckPin(newPin1);
				assertT2Test(ret == T2Error, testDescription );	
				
				testDescription = startTest("Test 2.13 - check pin (good pin)");
				ret = fipsWrapper.doCheckPin(newPin3);
				assertT2Test(ret == T2Success, testDescription) ;			
				
				// Check blank pin
				testDescription = startTest("Test 2.14 - changePinUsingAnswers");
				ret  = fipsWrapper.changePinUsingAnswers(blankPin, answers);
				assertT2Test(ret == T2Success, testDescription );	
				
				testDescription = startTest("Test 2.15 - check pin (bad pin)");
				ret = fipsWrapper.doCheckPin(newPin1);
				assertT2Test(ret == T2Error, testDescription );	
				
				testDescription = startTest("Test 2.16 - check pin (good pin)");
				ret = fipsWrapper.doCheckPin(blankPin);
				assertT2Test(ret == T2Success, testDescription) ;					
				
				// Check crazy pin
				testDescription = startTest("Test 2.17 - changePinUsingAnswers");
				ret  = fipsWrapper.changePinUsingAnswers(crazyPin, answers);
				assertT2Test(ret == T2Success, testDescription );	
				
				testDescription = startTest("Test 2.18 - check pin (bad pin)");
				ret = fipsWrapper.doCheckPin(newPin1);
				assertT2Test(ret == T2Error, testDescription );	
				
				testDescription = startTest("Test 2.19 - check pin (good pin)");
				ret = fipsWrapper.doCheckPin(crazyPin);
				assertT2Test(ret == T2Success, testDescription) ;					
						
				
				testDescription = startTest("Test 2.20 - check encrypt/decrypt");
				encryptedText = fipsWrapper.doEncryptUsingT2Crypto(crazyPin, stringToEncrypt);
				decryptedText = fipsWrapper.doDecryptUsingT2Crypto(crazyPin, encryptedText);
				//Log.d(TAG,"java decryptedText = " + decryptedText);
				assertT2Test(stringToEncrypt.equalsIgnoreCase(decryptedText), testDescription) ;					
								
				testDescription = startTest("Test 2.21 - check encrypt/decrypt - blank string");
				encryptedText = fipsWrapper.doEncryptUsingT2Crypto(crazyPin, blankStringToEncrypt);
				decryptedText = fipsWrapper.doDecryptUsingT2Crypto(crazyPin, encryptedText);
				//Log.d(TAG,"java decryptedText = " + decryptedText);
				assertT2Test(blankStringToEncrypt.equalsIgnoreCase(decryptedText), testDescription) ;					

				testDescription = startTest("Test 2.22 - check encrypt/decrypt - long string");
				encryptedText = fipsWrapper.doEncryptUsingT2Crypto(crazyPin, longStringToEncrypt);
				decryptedText = fipsWrapper.doDecryptUsingT2Crypto(crazyPin, encryptedText);
				assertT2Test(longStringToEncrypt.equalsIgnoreCase(decryptedText), testDescription) ;	
				
				testDescription = startTest("Test 2.23 - check encrypt/decrypt - BAD PIIN long string");
				encryptedText = fipsWrapper.doEncryptUsingT2Crypto(crazyPin, longStringToEncrypt);
				decryptedText = fipsWrapper.doDecryptUsingT2Crypto(newPin3, encryptedText);
				assertT2Test(!longStringToEncrypt.equalsIgnoreCase(decryptedText), testDescription) ;	
				
				
				// No test here, just do it and make sure that the app doesn't crash
				testDescription = startTest("Test 2.24 - check encrypt/decrypt - Good pin, decrypted non-encrypted string");
				//encryptedText = fipsWrapper.doEncryptUsingT2Crypto(newPin3, longStringToEncrypt);
				try {
					decryptedText = fipsWrapper.doDecryptUsingT2Crypto(newPin3, "1");
				} catch (Exception e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
				try {
					decryptedText = fipsWrapper.doDecryptUsingT2Crypto(newPin3, "4");
				} catch (Exception e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
				//assertT2Test(!longStringToEncrypt.equalsIgnoreCase(decryptedText), testDescription) ;	
				

				
				
			} catch (Exception e) {
				Log.e(TAG, "Exception " + e.toString());
				e.printStackTrace();
			}
	
			Log.d(TAG, "-------------------------------------------------------------");
			Log.d(TAG, "  Iteration " + iterations + " - Unit Test Summary");
			Log.d(TAG, "-------------------------------------------------------------");
			Log.d(TAG, "   Iteration " + iterations + " -   Iterations Performed: " + iterations);
			Log.d(TAG, "   Iteration " + iterations + " -   Tests passed: " + testPassCount);
			Log.d(TAG, "   Iteration " + iterations + " -   Tests failed: " + testFailCount);
			if (testPassCount == testTotalCount) {
				Log.d(TAG, "   ++ ALL TESTS PASSED ++ ");
				updateView("PASSED - All t2Crypto tests");
			}
			else {
				Log.e(TAG, "   ** ONE OF MORE t2Crypto FAILED! ** ");
				updateView("** ONE OF MORE t2Crypto FAILED! **");
			}
		}
		
//		fipsWrapper.doPrepare(false);		
//		try {
//			int ret = fipsWrapper.doInitializeLogin(pin, answers);
//		} catch (Exception e) {
//			// TODO Auto-generated catch block
//			e.printStackTrace();
//		}		

		
    }
 
    private String startTest(String testDescription) {
		Log.d(TAG,testDescription + " ------ Starting test " );
    	return testDescription;
    }
    
    private void assertT2Test(boolean result, String testDescription) {
    	testTotalCount++;
		if (result) {
			Log.d(TAG,"    PASSED - Iteration " + iterations + ", " + testDescription);
			testPassCount++;
		}
		else {
			Log.e(TAG,"    xxFAILxx - Iteration " + iterations + ", " + testDescription);
			testFailCount++;
		}	
    }
    
    private void updateView(String newLine) {
        textLines.add(newLine);   
    	textViewMain.setText("");
    	for (String line : textLines) {
    		textViewMain.append(line);
    		textViewMain.append("\n");
    	}
    	
    }
    
    private void InitializeSQLCipher() {
        SQLiteDatabase.loadLibs(this);


        SQLiteDatabase database;
		try {
			

			
			// Now create the database file and the database	
			String dbDirPath = getDatabasePath(DATABASE_NAME).getParent();
			File dbDirFile = this.getApplicationContext().getDatabasePath(dbDirPath);
			boolean result = dbDirFile.mkdirs();	// Note: we MUST create the directoryt path for this version of SQLCiphert
			File databaseFile = this.getApplicationContext().getDatabasePath(DATABASE_NAME);
			database = SQLiteDatabase.openOrCreateDatabase(databaseFile, PASSWORD, null);			
			
			


			
	        // Create a database table
	        database.execSQL("create table if not exists t1(a, b)");
	        
	        // Do a quick check to make sure we can write to and read from database
	        ContentValues values = new ContentValues();
	        values.put("a", "fred");
	        values.put("b", "arnie");
	        database.insert("t1", null, values);
	        
	        
	        Log.d(TAG, "---------------------- Testsing sqlCipher database functionality ---------------------");    
	        testDescription = startTest("Test 3.1 - writing to and reading from sqlcipher database");

			
	        String resultA= "";
	        String query = "select a from t1";
	        Cursor cursor = database.rawQuery(query,  null);
	        if (cursor.moveToFirst()) {
	        	resultA = cursor.getString(0);

				assertT2Test(resultA.equalsIgnoreCase("fred"), testDescription);
	        	Log.i(TAG, "resultA = " + resultA );
	        	
	        	if (resultA.equalsIgnoreCase("fred")) {
	        		databaseTestsPassed = true;
	        	} else {
	        		databaseTestsPassed = false;
	        	}
	        	
	        	
	        	
	        	cursor.close();
	        }	
			
			
	        database.close();
			
		} catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
			Log.e(TAG,"    xxFAILxx - Iteration " + iterations + ", " + testDescription);
			testFailCount++;
		}
    }

    // This can be used to test for database path issues - currently not used
    private void reopenDatabase() {
        SQLiteDatabase.loadLibs(this);

        String dbDirPath = getDatabasePath(DATABASE_NAME).getParent();
        File dbDirFile = this.getApplicationContext().getDatabasePath(dbDirPath);
        dbDirFile.mkdirs();
        
        SQLiteDatabase database;
		try {
			
			// Now create the database file and the database
			File databaseFile = this.getApplicationContext().getDatabasePath(DATABASE_NAME);
			database = SQLiteDatabase.openOrCreateDatabase(databaseFile, "Wrong password", null);
			
			
	        // Create a database table
	        database.execSQL("create table if not exists t1(a, b)");
	        
	        // Do a quick check to make sure we can write to and read from database
	        ContentValues values = new ContentValues();
	        values.put("a", "fred");
	        values.put("b", "arnie");
	        database.insert("t1", null, values);
	        
	        String resultA= "";
	        String query = "select a from t1";
	        Cursor cursor = database.rawQuery(query,  null);
	        if (cursor.moveToFirst()) {
	        	resultA = cursor.getString(0);

	        	Log.i("mytag", "resultA = " + resultA );
	        	if (resultA.equalsIgnoreCase("fred")) {
	        		Log.i("mytag", "PASSED - successfully writing to and reading from sqlcipher database" );
	        	}
	        	cursor.close();
	        }	
			
			
	        database.close();
			
		} catch (SQLiteException e) {
			Log.e(TAG, "SQLite Exception: " + e.toString());
			e.printStackTrace();
		} catch (Exception e) {
			e.printStackTrace();
		}
    }
    
}
;