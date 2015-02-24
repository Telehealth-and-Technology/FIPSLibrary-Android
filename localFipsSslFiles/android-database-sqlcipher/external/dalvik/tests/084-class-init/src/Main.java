/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

public class Main {
    public static void main(String[] args) {
        checkExceptions();
        checkTiming();
    }

    public static void sleep(int msec) {
        try {
            Thread.sleep(msec);
        } catch (InterruptedException ie) {
            System.err.println("sleep interrupted");
        }
    }

    static void checkExceptions() {
        try {
            System.out.println(PartialInit.FIELD0);
            System.err.println("Construction of PartialInit succeeded unexpectedly");
        } catch (ExceptionInInitializerError eiie) {
            System.out.println("Got expected EIIE for FIELD0");
        }

        try {
            System.out.println(PartialInit.FIELD0);
            System.err.println("Load of FIELD0 succeeded unexpectedly");
        } catch (NoClassDefFoundError ncdfe) {
            System.out.println("Got expected NCDFE for FIELD0");
        }
        try {
            System.out.println(PartialInit.FIELD1);
            System.err.println("Load of FIELD1 succeeded unexpectedly");
        } catch (NoClassDefFoundError ncdfe) {
            System.out.println("Got expected NCDFE for FIELD1");
        }
    }

    static void checkTiming() {
        FieldThread fieldThread = new FieldThread();
        MethodThread methodThread = new MethodThread();

        fieldThread.start();
        methodThread.start();

        /* start class init */
        IntHolder zero = SlowInit.FIELD0;

        /* init complete; allow other threads time to finish printing */
        Main.sleep(500);

        /* print all values */
        System.out.println("Fields (main thread): " +
            SlowInit.FIELD0.getValue() + SlowInit.FIELD1.getValue() +
            SlowInit.FIELD2.getValue() + SlowInit.FIELD3.getValue());
    }

    static class FieldThread extends Thread {
        public void run() {
            /* allow class init to start */
            Main.sleep(200);

            /* print fields; should delay until class init completes */
            System.out.println("Fields (child thread): " +
                SlowInit.FIELD0.getValue() + SlowInit.FIELD1.getValue() +
                SlowInit.FIELD2.getValue() + SlowInit.FIELD3.getValue());
        }
    }

    static class MethodThread extends Thread {
        public void run() {
            /* allow class init to start */
            Main.sleep(400);

            /* use a method that shouldn't be accessible yet */
            SlowInit.printMsg("MethodThread message");
        }
    }
}
