/*****************************************************************
MainActivity

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
	int iterations = 0;
	
    
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
        
       // MY_FIPS_mode();
        updateView("Checking FIPS signature");     
        updateView("Calling FIPS_mode");      
        
        FipsWrapper fipsWrapper = FipsWrapper.getInstance();
        
        FipsWrapper.setContext(this);
        
		Log.d(TAG, "---------------------- Testsing FIPS functionality ---------------------");       
        int result = fipsWrapper.doFIPSmode();	
        String t2FipsVersion = fipsWrapper.doT2FIPSVersion();
        Log.d(TAG, "T2 FIPS Version = " + t2FipsVersion);
        updateView("T2 FIPS Version = " + t2FipsVersion);
		Log.e(TAG, "FIPS_mode returned " + result);
		updateView("FIPS_mode returned " + result);  
		assertT2Test(result == 1, "enable FIPS!");
		if (result == 1) {
			updateView("PASSED - successfully enabled FIPS!");
		}
		else {
			updateView("FAILED - was NOT able to enable FIPS!");
		}

		// ---------------------------------------------------------------------------------------
		final int NUM_ITERATIONS = 1000;
		fipsWrapper.doPrepare(false);		// Tell it to use test vectors for now
		fipsWrapper.doSetVerboseLogging(false);
		// ---------------------------------------------------------------------------------------
		
		// Test t2Crypto
		String pin = "One";
		String newPin1 = "tWo";
		String newPin2 = "thrEe";
		String newPin3 = "Password is  is four";
		String answers = "TwoThreeFour";
		String badAnswers = "TwoThreeFour1";
		String badPin = "TOne";



		
		
		while(iterations++ < NUM_ITERATIONS) {
		
		Log.d(TAG, "---------------------- Testsing T2Crypto functionality - Iteration " + iterations + "  ---------------------");
	
			int ret;
			String testDescription;
			
			try {
				
				testDescription = startTest("Test 1 - doIsInitialized");
				fipsWrapper.doDeInitializeLogin();
				ret = fipsWrapper.doIsInitialized();
				assertT2Test(ret == T2False, testDescription);
				
				testDescription = startTest("Test 2 - doInitializeLogin");
				ret = fipsWrapper.doInitializeLogin(pin, answers);
				assertT2Test(ret == T2Success, testDescription );
	
				testDescription = startTest("Test 3 - doIsInitialized");
				ret = fipsWrapper.doIsInitialized();
				assertT2Test(ret == T2True, testDescription );
	
				
				String key = fipsWrapper.doGetDatabaseKeyUsingPin(pin);
				Log.d(TAG,"key = " + key);
	
				testDescription = startTest("Test 4 - check answers (bad answers)");
				ret = fipsWrapper.doCheckAnswers(badAnswers);
				assertT2Test(ret == T2Error, testDescription );
				
				testDescription = startTest("Test 5 - check answers (good answers)");
				ret = fipsWrapper.doCheckAnswers(answers);
				assertT2Test(ret == T2Success, testDescription );
				
				testDescription = startTest("Test 6 - check pin (bad pin)");
				ret = fipsWrapper.doCheckPin(badPin);
				assertT2Test(ret == T2Error, testDescription );		
				
				testDescription = startTest("Test 7 - check pin (good pin)");
				ret = fipsWrapper.doCheckPin(pin);
				assertT2Test(ret == T2Success, testDescription);		
				
				testDescription = startTest("Test 8 - change pin using pin");
				ret = fipsWrapper.changePinUsingPin(pin, newPin1);
				assertT2Test(ret == T2Success, testDescription );
				
				testDescription = startTest("Test 9 - check pin (good pin)");
				ret = fipsWrapper.doCheckPin(newPin1);
				assertT2Test(ret == T2Success, testDescription );	
				
				testDescription = startTest("Test 10 - check pin (bad pin)");
				ret = fipsWrapper.doCheckPin(pin);
				assertT2Test(ret == T2Error, testDescription );				
				
				testDescription = startTest("Test 11 - changePinUsingAnswers");
				ret  = fipsWrapper.changePinUsingAnswers(newPin3, answers);
				assertT2Test(ret == T2Success, testDescription );	
				
				testDescription = startTest("Test 12 - check pin (bad pin)");
				ret = fipsWrapper.doCheckPin(newPin1);
				assertT2Test(ret == T2Error, testDescription );	
				
				testDescription = startTest("Test 13 - check pin (good pin)");
				ret = fipsWrapper.doCheckPin(newPin3);
				assertT2Test(ret == T2Success, testDescription) ;			
				
				
			} catch (Exception e) {
				Log.e(TAG, "Exception " + e.toString());
				e.printStackTrace();
			}
	
			Log.i(TAG, "-------------------------------------------------------------");
			Log.i(TAG, "  Iteration " + iterations + " - Unit Test Summary");
			Log.i(TAG, "-------------------------------------------------------------");
			Log.i(TAG, "   Iteration " + iterations + " -   Iterations Performed: " + iterations);
			Log.i(TAG, "   Iteration " + iterations + " -   Tests passed: " + testPassCount);
			Log.i(TAG, "   Iteration " + iterations + " -   Tests failed: " + testFailCount);
			if (testPassCount == testTotalCount) {
				Log.i(TAG, "   ++ ALL TESTS PASSED ++ ");
			}
			else {
				Log.e(TAG, "   ** ONE OF MORE TESTS FAILED! ** ");
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
		Log.d(TAG," ------ Starting test " + testDescription);
    	return testDescription;
    }
    
    private void assertT2Test(boolean result, String testDescription) {
    	testTotalCount++;
		if (result) {
			Log.d(TAG,"PASSED - Iteration " + iterations + ", " + testDescription);
			testPassCount++;
		}
		else {
			Log.e(TAG,"xxFAILxx - Iteration " + iterations + ", " + testDescription);
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
			File databaseFile = this.getApplicationContext().getDatabasePath(DATABASE_NAME);
			database = SQLiteDatabase.openOrCreateDatabase(databaseFile, PASSWORD, null);
			

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
			
		} catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
    }

    
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