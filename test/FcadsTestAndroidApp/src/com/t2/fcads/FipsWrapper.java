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
	
	public static FipsWrapper getInstance() {
		if (instance == null) {
			instance = new FipsWrapper();
		}
		return instance;
	}
	
    /**
     * calls the native function FIPSmode
     * 
     * @return return value from jni FIPSmode
     */
    public int doFIPSmode() {
    	return FIPSmode();
    }

    
    public String doT2FIPSVersion() {
    	return T2FIPSVersion();
    }
}
