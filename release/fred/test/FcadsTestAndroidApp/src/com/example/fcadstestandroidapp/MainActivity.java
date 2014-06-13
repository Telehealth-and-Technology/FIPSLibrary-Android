package com.example.fcadstestandroidapp;

import java.io.File;




import java.util.ArrayList;
import java.util.List;

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
	
    public native int  FIPSFromJNI();

    
    
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
		int j = FIPSFromJNI();	
		Log.e("TAG", "FIPS_mode returned " + j);
		updateView("FIPS_mode returned " + j);  
		if (j == 1) {
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
        SQLiteDatabase database = SQLiteDatabase.openOrCreateDatabase(databaseFile, "test123", null);
        database.execSQL("create table t1(a, b)");
       // database.execSQL("insert into t1(a, b) values(?, ?)", new Object[]{"one for the money",
       //                                                                 "two for the show"});
        
        
        ContentValues values = new ContentValues();
        values.put("a", "fred");
        values.put("b", "arnie");
        database.insert("t1", null, values);
        
        String resultA= "";
        //Cursor cursor = database.query("t1", new String[] {"a", "b"}, );
        String query = "select a from t1";
        Cursor cursor = database.rawQuery(query,  null);
        if (cursor.moveToFirst()) {
        	resultA = cursor.getString(0);
        	//resultB = cursor.getString(1);
        	Log.i("mytag", "resultA = " + resultA );
        	if (resultA.equalsIgnoreCase("fred")) {
        		Log.i("mytag", "PASSED - successfully writing to and reading from sqlcipher database" );
        	}
        	cursor.close();
        }
        
        
    }
//    static {
//     // System.loadLibrary("wrapper");
//  //    System.loadLibrary("sqlcipher_android");
//		//System.loadLibrary("hello-jni");
////
//    }
}
