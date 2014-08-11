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
import android.database.Cursor;
import android.os.Bundle;
import android.util.Log;
import android.widget.TextView;

public class MainActivity extends Activity
{
	TextView textViewMain;
	List<String> textLines;
	private static final String PASSWORD = "test123";
	

    
    
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

        
       // MY_FIPS_mode();
        updateView("Checking FIPS signature");     
        updateView("Calling FIPS_mode");      
        
        FipsWrapper fipsWrapper = FipsWrapper.getInstance();
        int result = fipsWrapper.doFIPSmode();	
        String t2FipsVersion = fipsWrapper.doT2FIPSVersion();
        Log.d("TAG", "T2 FIPS Version = " + t2FipsVersion);
        updateView("T2 FIPS Version = " + t2FipsVersion);
		Log.e("TAG", "FIPS_mode returned " + result);
		updateView("FIPS_mode returned " + result);  
		if (result == 1) {
			Log.i("mytag", "PASSED - successfully enabled FIPS!" );
			updateView("PASSED - successfully enabled FIPS!");
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
        File databaseFile = getDatabasePath("demo.db");
        databaseFile.mkdirs();
        databaseFile.delete();
        SQLiteDatabase database = SQLiteDatabase.openOrCreateDatabase(databaseFile, PASSWORD, null);
        database.execSQL("create table t1(a, b)");

        
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
        
        
    }

}
