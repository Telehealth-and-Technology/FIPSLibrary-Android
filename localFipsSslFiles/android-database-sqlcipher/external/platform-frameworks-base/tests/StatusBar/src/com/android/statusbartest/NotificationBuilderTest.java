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

package com.android.statusbartest;

import java.util.GregorianCalendar;

import android.app.Activity;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.ContentResolver;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.os.Vibrator;
import android.os.Handler;
import android.text.TextUtils;
import android.util.Log;
import android.net.Uri;
import android.os.SystemClock;
import android.view.View;
import android.widget.CompoundButton;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.RemoteViews;
import android.os.PowerManager;

public class NotificationBuilderTest extends Activity
{
    private final static String TAG = "NotificationTestList";

    NotificationManager mNM;

    @Override
    public void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        mNM = (NotificationManager)getSystemService(NOTIFICATION_SERVICE);
        setContentView(R.layout.notification_builder_test);
        if (icicle == null) {
            setDefaults();
        }
        for (int id: new int[] {
                    R.id.clear_1,
                    R.id.clear_2,
                    R.id.clear_3,
                    R.id.clear_4,
                    R.id.clear_5,
                    R.id.clear_6,
                    R.id.clear_7,
                    R.id.clear_8,
                    R.id.clear_9,
                    R.id.clear_10,
                    R.id.notify_1,
                    R.id.notify_2,
                    R.id.notify_3,
                    R.id.notify_4,
                    R.id.notify_5,
                    R.id.notify_6,
                    R.id.notify_7,
                    R.id.notify_8,
                    R.id.notify_9,
                    R.id.notify_10,
                    R.id.ten,
                    R.id.clear_all,
                }) {
            findViewById(id).setOnClickListener(mClickListener);
        }
    }

    private void setDefaults() {
        setChecked(R.id.when_now);
        setChecked(R.id.icon_surprise);
        setChecked(R.id.title_medium);
        setChecked(R.id.text_medium);
        setChecked(R.id.info_none);
        setChecked(R.id.number_0);
        setChecked(R.id.intent_alert);
        setChecked(R.id.delete_none);
        setChecked(R.id.full_screen_none);
        setChecked(R.id.ticker_none);
        setChecked(R.id.large_icon_none);
        setChecked(R.id.sound_none);
        setChecked(R.id.vibrate_none);
        setChecked(R.id.lights_red);
        setChecked(R.id.lights_off);
    }

    private View.OnClickListener mClickListener = new View.OnClickListener() {
        public void onClick(View v) {
            switch (v.getId()) {
                case R.id.clear_1:
                    mNM.cancel(1);
                    break;
                case R.id.clear_2:
                    mNM.cancel(2);
                    break;
                case R.id.clear_3:
                    mNM.cancel(3);
                    break;
                case R.id.clear_4:
                    mNM.cancel(4);
                    break;
                case R.id.clear_5:
                    mNM.cancel(5);
                    break;
                case R.id.clear_6:
                    mNM.cancel(6);
                    break;
                case R.id.clear_7:
                    mNM.cancel(7);
                    break;
                case R.id.clear_8:
                    mNM.cancel(8);
                    break;
                case R.id.clear_9:
                    mNM.cancel(9);
                    break;
                case R.id.clear_10:
                    mNM.cancel(10);
                    break;
                case R.id.notify_1:
                    sendNotification(1);
                    break;
                case R.id.notify_2:
                    sendNotification(2);
                    break;
                case R.id.notify_3:
                    sendNotification(3);
                    break;
                case R.id.notify_4:
                    sendNotification(4);
                    break;
                case R.id.notify_5:
                    sendNotification(5);
                    break;
                case R.id.notify_6:
                    sendNotification(6);
                    break;
                case R.id.notify_7:
                    sendNotification(7);
                    break;
                case R.id.notify_8:
                    sendNotification(8);
                    break;
                case R.id.notify_9:
                    sendNotification(9);
                    break;
                case R.id.notify_10:
                    sendNotification(10);
                    break;
                case R.id.ten: {
                    for (int id=1; id<=10; id++) {
                        sendNotification(id);
                    }
                    break;
                }
                case R.id.clear_all: {
                    for (int id=1; id<=10; id++) {
                        mNM.cancel(id);
                    }
                    break;
                }
            }
        }
    };

    private void sendNotification(int id) {
        final Notification n = buildNotification(id);
        mNM.notify(id, n);
    }

    private Notification buildNotification(int id) {
        Notification.Builder b = new Notification.Builder(this);

        // when
        switch (getRadioChecked(R.id.group_when)) {
            case R.id.when_midnight: {
                GregorianCalendar c = new GregorianCalendar();
                c.set(GregorianCalendar.HOUR_OF_DAY, 0);
                c.set(GregorianCalendar.MINUTE, 0);
                c.set(GregorianCalendar.SECOND, 0);
                b.setWhen(c.getTimeInMillis());
                break;
            }
            case R.id.when_now:
                b.setWhen(System.currentTimeMillis());
                break;
            case R.id.when_now_plus_1h:
                break;
            case R.id.when_tomorrow:
                break;
        }

        // icon
        switch (getRadioChecked(R.id.group_icon)) {
            case R.id.icon_im:
                b.setSmallIcon(R.drawable.icon1);
                break;
            case R.id.icon_alert:
                b.setSmallIcon(R.drawable.icon2);
                break;
            case R.id.icon_surprise:
                b.setSmallIcon(R.drawable.emo_im_kissing);
                break;
        }

        // title
        final String title = getRadioTag(R.id.group_title);
        if (!TextUtils.isEmpty(title)) {
            b.setContentTitle(title);
        }

        // text
        final String text = getRadioTag(R.id.group_text);
        if (!TextUtils.isEmpty(text)) {
            b.setContentText(text);
        }

        // info
        final String info = getRadioTag(R.id.group_info);
        if (!TextUtils.isEmpty(info)) {
            b.setContentInfo(info);
        }

        // number
        b.setNumber(getRadioInt(R.id.group_number, 0));

        // contentIntent
        switch (getRadioChecked(R.id.group_intent)) {
            case R.id.intent_none:
                break;
            case R.id.intent_alert:
                b.setContentIntent(makeContentIntent(id));
                break;
        }

        // deleteIntent
        switch (getRadioChecked(R.id.group_delete)) {
            case R.id.delete_none:
                break;
            case R.id.delete_alert:
                b.setDeleteIntent(makeDeleteIntent(id));
                break;
        }

        // fullScreenIntent TODO

        // ticker
        switch (getRadioChecked(R.id.group_ticker)) {
            case R.id.ticker_none:
                break;
            case R.id.ticker_short:
            case R.id.ticker_wrap:
            case R.id.ticker_haiku:
                b.setTicker(getRadioTag(R.id.group_ticker));
                break;
            case R.id.ticker_custom:
                // TODO
                break;
        }

        // largeIcon
        switch (getRadioChecked(R.id.group_large_icon)) {
            case R.id.large_icon_none:
                break;
            case R.id.large_icon_pineapple:
                b.setLargeIcon(loadBitmap(R.drawable.pineapple));
                break;
            case R.id.large_icon_pineapple2:
                b.setLargeIcon(loadBitmap(R.drawable.pineapple2));
                break;
            case R.id.large_icon_small:
                b.setLargeIcon(loadBitmap(R.drawable.icon2));
                break;
        }

        // sound TODO

        // vibrate
        switch (getRadioChecked(R.id.group_vibrate)) {
            case R.id.vibrate_none:
                break;
            case R.id.vibrate_short:
                b.setVibrate(new long[] { 0, 200 });
                break;
            case R.id.vibrate_medium:
                b.setVibrate(new long[] { 0, 500 });
                break;
            case R.id.vibrate_long:
                b.setVibrate(new long[] { 0, 1000 });
                break;
            case R.id.vibrate_pattern:
                b.setVibrate(new long[] { 0, 250, 250, 250, 250, 250, 250, 250 });
                break;
        }

        // lights
        final int color = getRadioInt(R.id.group_lights_color, 0xff0000);
        int onMs;
        int offMs;
        switch (getRadioChecked(R.id.group_lights_blink)) {
            case R.id.lights_slow:
                onMs = 1300;
                offMs = 1300;
                break;
            case R.id.lights_fast:
                onMs = 300;
                offMs = 300;
                break;
            case R.id.lights_on:
                onMs = 1;
                offMs = 0;
                break;
            case R.id.lights_off:
            default:
                onMs = 0;
                offMs = 0;
                break;
        }
        if (onMs != 0 && offMs != 0) {
            b.setLights(color, onMs, offMs);
        }

        // flags
        b.setOngoing(getChecked(R.id.flag_ongoing));
        b.setOnlyAlertOnce(getChecked(R.id.flag_once));
        b.setAutoCancel(getChecked(R.id.flag_auto_cancel));

        // defaults
        int defaults = 0;
        if (getChecked(R.id.default_sound)) {
            defaults |= Notification.DEFAULT_SOUND;
        }
        if (getChecked(R.id.default_vibrate)) {
            defaults |= Notification.DEFAULT_VIBRATE;
        }
        if (getChecked(R.id.default_lights)) {
            defaults |= Notification.DEFAULT_LIGHTS;
        }
        b.setDefaults(defaults);

        return b.getNotification();
    }

    private void setChecked(int id) {
        final CompoundButton b = (CompoundButton)findViewById(id);
        b.setChecked(true);
    }

    private int getRadioChecked(int id) {
        final RadioGroup g = (RadioGroup)findViewById(id);
        return g.getCheckedRadioButtonId();
    }

    private String getRadioTag(int id) {
        final RadioGroup g = (RadioGroup)findViewById(id);
        final View v = findViewById(g.getCheckedRadioButtonId());
        return (String)v.getTag();
    }

    private int getRadioInt(int id, int def) {
        String str = getRadioTag(id);
        if (TextUtils.isEmpty(str)) {
            return def;
        } else {
            try {
                return Integer.parseInt(str);
            } catch (NumberFormatException ex) {
                return def;
            }
        }
    }

    private boolean getChecked(int id) {
        final CompoundButton b = (CompoundButton)findViewById(id);
        return b.isChecked();
    }
    
    private Bitmap loadBitmap(int id) {
        final BitmapDrawable bd = (BitmapDrawable)getResources().getDrawable(id);
        return Bitmap.createBitmap(bd.getBitmap());
    }

    private PendingIntent makeDeleteIntent(int id) {
        Intent intent = new Intent(this, ConfirmationActivity.class);
        intent.setData(Uri.fromParts("content", "//status_bar_test/delete/" + id, null));
        intent.putExtra(ConfirmationActivity.EXTRA_TITLE, "Delete intent");
        intent.putExtra(ConfirmationActivity.EXTRA_TEXT, "id: " + id);
        return PendingIntent.getActivity(this, 0, intent, 0);
    }

    private PendingIntent makeContentIntent(int id) {
        Intent intent = new Intent(this, ConfirmationActivity.class);
        intent.setData(Uri.fromParts("content", "//status_bar_test/content/" + id, null));
        intent.putExtra(ConfirmationActivity.EXTRA_TITLE, "Content intent");
        intent.putExtra(ConfirmationActivity.EXTRA_TEXT, "id: " + id);
        return PendingIntent.getActivity(this, 0, intent, 0);
    }
}

