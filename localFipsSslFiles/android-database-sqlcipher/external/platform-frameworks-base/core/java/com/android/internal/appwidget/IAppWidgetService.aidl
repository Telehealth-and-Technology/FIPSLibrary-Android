/*
 * Copyright (C) 2011 The Android Open Source Project
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

package com.android.internal.appwidget;

import android.content.ComponentName;
import android.content.Intent;
import android.appwidget.AppWidgetProviderInfo;
import com.android.internal.appwidget.IAppWidgetHost;
import android.os.IBinder;
import android.widget.RemoteViews;

/** {@hide} */
interface IAppWidgetService {
    
    //
    // for AppWidgetHost
    //
    int[] startListening(IAppWidgetHost host, String packageName, int hostId,
            out List<RemoteViews> updatedViews);
    void stopListening(int hostId);
    int allocateAppWidgetId(String packageName, int hostId);
    void deleteAppWidgetId(int appWidgetId);
    void deleteHost(int hostId);
    void deleteAllHosts();
    RemoteViews getAppWidgetViews(int appWidgetId);

    //
    // for AppWidgetManager
    //
    void updateAppWidgetIds(in int[] appWidgetIds, in RemoteViews views);
    void partiallyUpdateAppWidgetIds(in int[] appWidgetIds, in RemoteViews views);
    void updateAppWidgetProvider(in ComponentName provider, in RemoteViews views);
    void notifyAppWidgetViewDataChanged(in int[] appWidgetIds, int viewId);
    List<AppWidgetProviderInfo> getInstalledProviders();
    AppWidgetProviderInfo getAppWidgetInfo(int appWidgetId);
    void bindAppWidgetId(int appWidgetId, in ComponentName provider);
    void bindRemoteViewsService(int appWidgetId, in Intent intent, in IBinder connection);
    void unbindRemoteViewsService(int appWidgetId, in Intent intent);
    int[] getAppWidgetIds(in ComponentName provider);

}

