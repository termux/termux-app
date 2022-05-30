/*
 * Copyright (C) 2015 The Android Open Source Project
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
 * limitations under the License
 */
package com.termux.view.support;

import android.util.Log;
import android.widget.PopupWindow;

import java.lang.reflect.Method;

/**
 * Implementation of PopupWindow compatibility that can call Gingerbread APIs.
 * https://chromium.googlesource.com/android_tools/+/HEAD/sdk/extras/android/support/v4/src/gingerbread/android/support/v4/widget/PopupWindowCompatGingerbread.java
 */
public class PopupWindowCompatGingerbread {

    private static Method sSetWindowLayoutTypeMethod;
    private static boolean sSetWindowLayoutTypeMethodAttempted;
    private static Method sGetWindowLayoutTypeMethod;
    private static boolean sGetWindowLayoutTypeMethodAttempted;

    public static void setWindowLayoutType(PopupWindow popupWindow, int layoutType) {
        if (!sSetWindowLayoutTypeMethodAttempted) {
            try {
                sSetWindowLayoutTypeMethod = PopupWindow.class.getDeclaredMethod(
                    "setWindowLayoutType", int.class);
                sSetWindowLayoutTypeMethod.setAccessible(true);
            } catch (Exception e) {
                // Reflection method fetch failed. Oh well.
            }
            sSetWindowLayoutTypeMethodAttempted = true;
        }
        if (sSetWindowLayoutTypeMethod != null) {
            try {
                sSetWindowLayoutTypeMethod.invoke(popupWindow, layoutType);
            } catch (Exception e) {
                // Reflection call failed. Oh well.
            }
        }
    }

    public static int getWindowLayoutType(PopupWindow popupWindow) {
        if (!sGetWindowLayoutTypeMethodAttempted) {
            try {
                sGetWindowLayoutTypeMethod = PopupWindow.class.getDeclaredMethod(
                    "getWindowLayoutType");
                sGetWindowLayoutTypeMethod.setAccessible(true);
            } catch (Exception e) {
                // Reflection method fetch failed. Oh well.
            }
            sGetWindowLayoutTypeMethodAttempted = true;
        }
        if (sGetWindowLayoutTypeMethod != null) {
            try {
                return (Integer) sGetWindowLayoutTypeMethod.invoke(popupWindow);
            } catch (Exception e) {
                // Reflection call failed. Oh well.
            }
        }
        return 0;
    }

}
