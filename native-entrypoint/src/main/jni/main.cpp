#include <stdint.h>
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
 *
 */

//BEGIN_INCLUDE(all)
#include <dlfcn.h>
#include <stdio.h>

#include <android/native_activity.h>
#include <android/log.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "native-activity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "native-activity", __VA_ARGS__))

jstring ANativeActivity_getIntent(ANativeActivity* activity, const char* intentName) {
    JNIEnv* env = activity->env;
    jobject me = activity->clazz;

    jclass acl = env->GetObjectClass(me); //class pointer of NativeActivity
    jmethodID giid = env->GetMethodID(acl, "getIntent", "()Landroid/content/Intent;");
    jobject intent = env->CallObjectMethod(me, giid); //Got our intent

    jclass icl = env->GetObjectClass(intent); //class pointer of Intent
    jmethodID gseid = env->GetMethodID(icl, "getStringExtra", "(Ljava/lang/String;)Ljava/lang/String;");

    return (jstring)env->CallObjectMethod(intent, gseid, env->NewStringUTF(intentName));
}

void ANativeActivity_onCreate(ANativeActivity* activity, void* savedState, size_t savedStateSize) {
    JNIEnv* env = activity->env;
    jstring dylib_path = ANativeActivity_getIntent(activity, "DYLIB_PATH");

    const char *path = env->GetStringUTFChars(dylib_path, 0);
    void* handle = dlopen(path, RTLD_LAZY | RTLD_GLOBAL);

    env->ReleaseStringUTFChars(dylib_path, path);

    {
        if (handle == NULL) {
            char* errorText = dlerror();
            LOGW("ERROR!!! %s\n", errorText);
            printf("%s\n", errorText);
            ANativeActivity_finish(activity);
            return;
        }

        void* entryPoint = dlsym(handle, "ANativeActivity_onCreate");

        if (entryPoint) {
            LOGI("%s found.\n", "ANativeActivity_onCreate");
        } else {
            entryPoint = dlsym(handle, __func__);

            if (entryPoint) {
                LOGI("%s found.\n", __func__);
            }
        }

        if (entryPoint) {
            LOGI("calling main...\n");
            (*(void(*)(ANativeActivity*, void*, size_t))entryPoint)(activity, savedState, savedStateSize);
            LOGI("exited!...\n");
        } else {
            char* errorText = dlerror();
            LOGW("ERROR!!! %s\n", errorText);
            printf("%s\n", errorText);
        }

        dlclose(handle);
    }
}
