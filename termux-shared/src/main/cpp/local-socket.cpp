#include <cstdio>
#include <ctime>
#include <cerrno>
#include <jni.h>
#include <sstream>
#include <string>
#include <unistd.h>

#include <android/log.h>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#define LOG_TAG "local-socket"
#define JNI_EXCEPTION "jni-exception"

using namespace std;


/* Convert a jstring to a std:string. */
string jstring_to_stdstr(JNIEnv *env, jstring jString) {
    jclass stringClass = env->FindClass("java/lang/String");
    jmethodID getBytes = env->GetMethodID(stringClass, "getBytes", "()[B");
    jbyteArray jStringBytesArray = (jbyteArray) env->CallObjectMethod(jString, getBytes);
    jsize length = env->GetArrayLength(jStringBytesArray);
    jbyte* jStringBytes = env->GetByteArrayElements(jStringBytesArray, nullptr);
    std::string stdString((char *)jStringBytes, length);
    env->ReleaseByteArrayElements(jStringBytesArray, jStringBytes, JNI_ABORT);
    return stdString;
}

/* Get characters before first occurrence of the delim in a std:string. */
string get_string_till_first_delim(string str, char delim) {
    if (!str.empty()) {
        stringstream cmdline_args(str);
        string tmp;
        if (getline(cmdline_args, tmp, delim))
            return tmp;
    }
    return "";
}

/* Replace `\0` values with spaces in a std:string. */
string replace_null_with_space(string str) {
    if (str.empty())
        return "";

    stringstream tokens(str);
    string tmp;
    string str_spaced;
    while (getline(tokens, tmp, '\0')){
        str_spaced.append(" " + tmp);
    }

    if (!str_spaced.empty()) {
        if (str_spaced.front() == ' ')
            str_spaced.erase(0, 1);
    }

    return str_spaced;
}

/* Get class name of a jclazz object with a call to `Class.getName()`. */
string get_class_name(JNIEnv *env, jclass clazz) {
    jclass classClass = env->FindClass("java/lang/Class");
    jmethodID getName = env->GetMethodID(classClass, "getName", "()Ljava/lang/String;");
    jstring className = (jstring) env->CallObjectMethod(clazz, getName);
    return jstring_to_stdstr(env, className);
}



/*
 * Get /proc/[pid]/cmdline for a process with pid.
 *
 * https://manpages.debian.org/testing/manpages/proc.5.en.html
 */
string get_process_cmdline(const pid_t pid) {
    string cmdline;
    char buf[BUFSIZ];
    size_t len;
    char procfile[BUFSIZ];
    sprintf(procfile, "/proc/%d/cmdline", pid);
    FILE *fp = fopen(procfile, "rb");
    if (fp) {
        while ((len = fread(buf, 1, sizeof(buf), fp)) > 0) {
            cmdline.append(buf, len);
        }
        fclose(fp);
    }

    return cmdline;
}

/* Extract process name from /proc/[pid]/cmdline value of a process. */
string get_process_name_from_cmdline(string cmdline) {
    return get_string_till_first_delim(cmdline, '\0');
}

/* Replace `\0` values with spaces in /proc/[pid]/cmdline value of a process. */
string get_process_cmdline_spaced(string cmdline) {
    return replace_null_with_space(cmdline);
}


/* Send an ERROR log message to android logcat. */
void log_error(string message) {
    __android_log_write(ANDROID_LOG_ERROR, LOG_TAG, message.c_str());
}

/* Send an WARN log message to android logcat. */
void log_warn(string message) {
    __android_log_write(ANDROID_LOG_WARN, LOG_TAG, message.c_str());
}

/* Get "title: message" formatted string. */
string get_title_and_message(JNIEnv *env, jstring title, string message) {
    if (title)
        message = jstring_to_stdstr(env, title) + ": " + message;
    return message;
}


/* Convert timespec to milliseconds. */
int64_t timespec_to_milliseconds(const struct timespec* const time) {
    return (((int64_t)time->tv_sec) * 1000) + (((int64_t)time->tv_nsec)/1000000);
}

/* Convert milliseconds to timeval. */
timeval milliseconds_to_timeval(int milliseconds) {
    struct timeval tv = {};
    tv.tv_sec = milliseconds / 1000;
    tv.tv_usec = (milliseconds % 1000) * 1000;
    return tv;
}


// Note: Exceptions thrown from JNI must be caught with Throwable class instead of Exception,
// otherwise exception will be sent to UncaughtExceptionHandler of the thread.
// Android studio complains that getJniResult functions always return nullptr since linter is broken
// for jboolean and jobject if comparisons.
bool checkJniException(JNIEnv *env) {
    if (env->ExceptionCheck()) {
        jthrowable throwable = env->ExceptionOccurred();
        if (throwable != NULL) {
            env->ExceptionClear();
            env->Throw(throwable);
            return true;
        }
    }

    return false;
}

string getJniResultString(const int retvalParam, const int errnoParam,
                          string errmsgParam, const int intDataParam) {
    return "retval=" + to_string(retvalParam) + ", errno=" + to_string(errnoParam) +
           ", errmsg=\"" + errmsgParam + "\"" + ", intData=" + to_string(intDataParam);
}

/* Get "com/termux/shared/jni/models/JniResult" object that can be returned as result for a JNI call. */
jobject getJniResult(JNIEnv *env, jstring title, const int retvalParam, const int errnoParam,
                     string errmsgParam, const int intDataParam) {
    jclass clazz = env->FindClass("com/termux/shared/jni/models/JniResult");
    if (checkJniException(env)) return NULL;
    if (!clazz) {
        log_error(get_title_and_message(env, title,
                                        "Failed to find JniResult class to create object for " +
                                        getJniResultString(retvalParam, errnoParam, errmsgParam, intDataParam)));
        return NULL;
    }

    jmethodID constructor = env->GetMethodID(clazz, "<init>", "(IILjava/lang/String;I)V");
    if (checkJniException(env)) return NULL;
    if (!constructor) {
        log_error(get_title_and_message(env, title,
                                        "Failed to get constructor for JniResult class to create object for " +
                                        getJniResultString(retvalParam, errnoParam, errmsgParam, intDataParam)));
        return NULL;
    }

    if (!errmsgParam.empty())
        errmsgParam = get_title_and_message(env, title, string(errmsgParam));

    jobject obj = env->NewObject(clazz, constructor, retvalParam, errnoParam, env->NewStringUTF(errmsgParam.c_str()), intDataParam);
    if (checkJniException(env)) return NULL;
    if (obj == NULL) {
        log_error(get_title_and_message(env, title,
                                        "Failed to get JniResult object for " +
                                        getJniResultString(retvalParam, errnoParam, errmsgParam, intDataParam)));
        return NULL;
    }

    return obj;
}


jobject getJniResult(JNIEnv *env, jstring title, const int retvalParam, const int errnoParam) {
    return getJniResult(env, title, retvalParam, errnoParam, strerror(errnoParam), 0);
}

jobject getJniResult(JNIEnv *env, jstring title, const int retvalParam, string errmsgPrefixParam) {
    return getJniResult(env, title, retvalParam, 0, errmsgPrefixParam, 0);
}

jobject getJniResult(JNIEnv *env, jstring title, const int retvalParam, const int errnoParam, string errmsgPrefixParam) {
    return getJniResult(env, title, retvalParam, errnoParam, errmsgPrefixParam + ": " + string(strerror(errnoParam)), 0);
}

jobject getJniResult(JNIEnv *env, jstring title, const int intDataParam) {
    return getJniResult(env, title, 0, 0, "", intDataParam);
}

jobject getJniResult(JNIEnv *env, jstring title) {
    return getJniResult(env, title, 0, 0, "", 0);
}


/* Set int fieldName field for clazz to value. */
string setIntField(JNIEnv *env, jobject obj, jclass clazz, const string fieldName, const int value) {
    jfieldID field = env->GetFieldID(clazz, fieldName.c_str(), "I");
    if (checkJniException(env)) return JNI_EXCEPTION;
    if (!field) {
        return "Failed to get int \"" + string(fieldName) + "\" field of \"" +
               get_class_name(env, clazz) + "\" class to set value \"" + to_string(value) + "\"";
    }

    env->SetIntField(obj, field, value);
    if (checkJniException(env)) return JNI_EXCEPTION;

    return "";
}

/* Set String fieldName field for clazz to value. */
string setStringField(JNIEnv *env, jobject obj, jclass clazz, const string fieldName, const string value) {
    jfieldID field = env->GetFieldID(clazz, fieldName.c_str(), "Ljava/lang/String;");
    if (checkJniException(env)) return JNI_EXCEPTION;
    if (!field) {
        return "Failed to get String \"" + string(fieldName) + "\" field of \"" +
               get_class_name(env, clazz) + "\" class to set value \"" + value + "\"";
    }

    env->SetObjectField(obj, field, env->NewStringUTF(value.c_str()));
    if (checkJniException(env)) return JNI_EXCEPTION;

    return "";
}



extern "C"
JNIEXPORT jobject JNICALL
Java_com_termux_shared_net_socket_local_LocalSocketManager_createServerSocketNative(JNIEnv *env, jclass clazz,
                                                                                    jstring logTitle,
                                                                                    jbyteArray pathArray,
                                                                                    jint backlog) {
    if (backlog < 1 || backlog > 500) {
        return getJniResult(env, logTitle, -1, "createServerSocketNative(): Backlog \"" +
                                               to_string(backlog) + "\" is not between 1-500");
    }

    // Create server socket
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1) {
        return getJniResult(env, logTitle, -1, errno, "createServerSocketNative(): Create local socket failed");
    }

    jbyte* path = env->GetByteArrayElements(pathArray, nullptr);
    if (checkJniException(env)) return NULL;
    if (path == nullptr) {
        close(fd);
        return getJniResult(env, logTitle, -1, "createServerSocketNative(): Path passed is null");
    }

    // On Linux, sun_path is 108 bytes (UNIX_PATH_MAX) in size
    int chars = env->GetArrayLength(pathArray);
    if (checkJniException(env)) return NULL;
    if (chars >= 108 || chars >= sizeof(struct sockaddr_un) - sizeof(sa_family_t)) {
        env->ReleaseByteArrayElements(pathArray, path, JNI_ABORT);
        if (checkJniException(env)) return NULL;
        close(fd);
        return getJniResult(env, logTitle, -1, "createServerSocketNative(): Path passed is too long");
    }

    struct sockaddr_un adr = {.sun_family = AF_UNIX};
    memcpy(&adr.sun_path, path, chars);

    // Bind path to server socket
    if (::bind(fd, reinterpret_cast<struct sockaddr*>(&adr), sizeof(adr)) == -1) {
        int errnoBackup = errno;
        env->ReleaseByteArrayElements(pathArray, path, JNI_ABORT);
        if (checkJniException(env)) return NULL;
        close(fd);
        return getJniResult(env, logTitle, -1, errnoBackup,
                            "createServerSocketNative(): Bind to local socket at path \"" + string(adr.sun_path) + "\" with fd " + to_string(fd) + " failed");
    }

    // Start listening for client sockets on server socket
    if (listen(fd, backlog) == -1) {
        int errnoBackup = errno;
        env->ReleaseByteArrayElements(pathArray, path, JNI_ABORT);
        if (checkJniException(env)) return NULL;
        close(fd);
        return getJniResult(env, logTitle, -1, errnoBackup,
                            "createServerSocketNative(): Listen on local socket at path \"" + string(adr.sun_path) + "\" with fd " + to_string(fd) + " failed");
    }

    env->ReleaseByteArrayElements(pathArray, path, JNI_ABORT);
    if (checkJniException(env)) return NULL;

    // Return success and server socket fd in JniResult.intData field
    return getJniResult(env, logTitle, fd);
}

extern "C"
JNIEXPORT jobject JNICALL
Java_com_termux_shared_net_socket_local_LocalSocketManager_closeSocketNative(JNIEnv *env, jclass clazz,
                                                                             jstring logTitle, jint fd) {
    if (fd < 0) {
        return getJniResult(env, logTitle, -1, "closeSocketNative(): Invalid fd \"" + to_string(fd) + "\" passed");
    }

    if (close(fd) == -1) {
        return getJniResult(env, logTitle, -1, errno, "closeSocketNative(): Failed to close socket fd " + to_string(fd));
    }

    // Return success
    return getJniResult(env, logTitle);
}

extern "C"
JNIEXPORT jobject JNICALL
Java_com_termux_shared_net_socket_local_LocalSocketManager_acceptNative(JNIEnv *env, jclass clazz,
                                                                        jstring logTitle, jint fd) {
    if (fd < 0) {
        return getJniResult(env, logTitle, -1, "acceptNative(): Invalid fd \"" + to_string(fd) + "\" passed");
    }

    // Accept client socket
    int clientFd = accept(fd, nullptr, nullptr);
    if (clientFd == -1) {
        return getJniResult(env, logTitle, -1, errno, "acceptNative(): Failed to accept client on fd " + to_string(fd));
    }

    // Return success and client socket fd in JniResult.intData field
    return getJniResult(env, logTitle, clientFd);
}

extern "C"
JNIEXPORT jobject JNICALL
Java_com_termux_shared_net_socket_local_LocalSocketManager_readNative(JNIEnv *env, jclass clazz,
                                                                      jstring logTitle,
                                                                      jint fd, jbyteArray dataArray,
                                                                      jlong deadline) {
    if (fd < 0) {
        return getJniResult(env, logTitle, -1, "readNative(): Invalid fd \"" + to_string(fd) + "\" passed");
    }

    jbyte* data = env->GetByteArrayElements(dataArray, nullptr);
    if (checkJniException(env)) return NULL;
    if (data == nullptr) {
        return getJniResult(env, logTitle, -1, "readNative(): data passed is null");
    }

    struct timespec time = {};
    jbyte* current = data;
    int bytes = env->GetArrayLength(dataArray);
    if (checkJniException(env)) return NULL;
    int bytesRead = 0;
    while (bytesRead < bytes) {
        if (deadline > 0) {
            if (clock_gettime(CLOCK_REALTIME, &time) != -1) {
                // If current time is greater than the time defined in deadline
                if (timespec_to_milliseconds(&time) > deadline) {
                    env->ReleaseByteArrayElements(dataArray, data, 0);
                    if (checkJniException(env)) return NULL;
                    return getJniResult(env, logTitle, -1,
                                        "readNative(): Deadline \"" + to_string(deadline) + "\" timeout");
                }
            } else {
                log_warn(get_title_and_message(env, logTitle,
                                               "readNative(): Deadline \"" + to_string(deadline) +
                                               "\" timeout will not work since failed to get current time"));
            }
        }

        // Read data from socket
        int ret = read(fd, current, bytes);
        if (ret == -1) {
            int errnoBackup = errno;
            env->ReleaseByteArrayElements(dataArray, data, 0);
            if (checkJniException(env)) return NULL;
            return getJniResult(env, logTitle, -1, errnoBackup, "readNative(): Failed to read on fd "  + to_string(fd));
        }
        // EOF, peer closed writing end
        if (ret == 0) {
            break;
        }

        bytesRead += ret;
        current += ret;
    }

    env->ReleaseByteArrayElements(dataArray, data, 0);
    if (checkJniException(env)) return NULL;

    // Return success and bytes read in JniResult.intData field
    return getJniResult(env, logTitle, bytesRead);
}


extern "C"
JNIEXPORT jobject JNICALL
Java_com_termux_shared_net_socket_local_LocalSocketManager_sendNative(JNIEnv *env, jclass clazz,
                                                                      jstring logTitle,
                                                                      jint fd, jbyteArray dataArray,
                                                                      jlong deadline) {
    if (fd < 0) {
        return getJniResult(env, logTitle, -1, "sendNative(): Invalid fd \"" + to_string(fd) + "\" passed");
    }

    jbyte* data = env->GetByteArrayElements(dataArray, nullptr);
    if (checkJniException(env)) return NULL;
    if (data == nullptr) {
        return getJniResult(env, logTitle, -1, "sendNative(): data passed is null");
    }

    struct timespec time = {};
    jbyte* current = data;
    int bytes = env->GetArrayLength(dataArray);
    if (checkJniException(env)) return NULL;
    while (bytes > 0) {
        if (deadline > 0) {
            if (clock_gettime(CLOCK_REALTIME, &time) != -1) {
                // If current time is greater than the time defined in deadline
                if (timespec_to_milliseconds(&time) > deadline) {
                    env->ReleaseByteArrayElements(dataArray, data, JNI_ABORT);
                    if (checkJniException(env)) return NULL;
                    return getJniResult(env, logTitle, -1,
                                        "sendNative(): Deadline \"" + to_string(deadline) + "\" timeout");
                }
            } else {
                log_warn(get_title_and_message(env, logTitle,
                                               "sendNative(): Deadline \"" + to_string(deadline) +
                                               "\" timeout will not work since failed to get current time"));
            }
        }

        // Send data to socket
        int ret = send(fd, current, bytes, MSG_NOSIGNAL);
        if (ret == -1) {
            int errnoBackup = errno;
            env->ReleaseByteArrayElements(dataArray, data, JNI_ABORT);
            if (checkJniException(env)) return NULL;
            return getJniResult(env, logTitle, -1, errnoBackup, "sendNative(): Failed to send on fd " + to_string(fd));
        }

        bytes -= ret;
        current += ret;
    }

    env->ReleaseByteArrayElements(dataArray, data, JNI_ABORT);
    if (checkJniException(env)) return NULL;

    // Return success
    return getJniResult(env, logTitle);
}

extern "C"
JNIEXPORT jobject JNICALL
Java_com_termux_shared_net_socket_local_LocalSocketManager_availableNative(JNIEnv *env, jclass clazz,
                                                                           jstring logTitle, jint fd) {
    if (fd < 0) {
        return getJniResult(env, logTitle, -1, "availableNative(): Invalid fd \"" + to_string(fd) + "\" passed");
    }

    int available = 0;
    if (ioctl(fd, SIOCINQ, &available) == -1) {
        return getJniResult(env, logTitle, -1, errno,
                            "availableNative(): Failed to get number of unread bytes in the receive buffer of fd " + to_string(fd));
    }

    // Return success and bytes available in JniResult.intData field
    return getJniResult(env, logTitle, available);
}

/* Sets socket option timeout in milliseconds. */
int set_socket_timeout(int fd, int option, int timeout) {
    struct timeval tv = milliseconds_to_timeval(timeout);
    socklen_t len = sizeof(tv);
    return setsockopt(fd, SOL_SOCKET, option, &tv, len);
}

extern "C"
JNIEXPORT jobject JNICALL
Java_com_termux_shared_net_socket_local_LocalSocketManager_setSocketReadTimeoutNative(JNIEnv *env, jclass clazz,
                                                                                      jstring logTitle,
                                                                                      jint fd, jint timeout) {
    if (fd < 0) {
        return getJniResult(env, logTitle, -1, "setSocketReadTimeoutNative(): Invalid fd \"" + to_string(fd) + "\" passed");
    }

    if (set_socket_timeout(fd, SO_RCVTIMEO, timeout) == -1) {
        return getJniResult(env, logTitle, -1, errno,
                            "setSocketReadTimeoutNative(): Failed to set socket receiving (SO_RCVTIMEO) timeout for fd " + to_string(fd));
    }

    // Return success
    return getJniResult(env, logTitle);
}

extern "C"
JNIEXPORT jobject JNICALL
Java_com_termux_shared_net_socket_local_LocalSocketManager_setSocketSendTimeoutNative(JNIEnv *env, jclass clazz,
                                                                                      jstring logTitle,
                                                                                      jint fd, jint timeout) {
    if (fd < 0) {
        return getJniResult(env, logTitle, -1, "setSocketSendTimeoutNative(): Invalid fd \"" +
                                               to_string(fd) + "\" passed");
    }

    if (set_socket_timeout(fd, SO_SNDTIMEO, timeout) == -1) {
        return getJniResult(env, logTitle, -1, errno,
                            "setSocketSendTimeoutNative(): Failed to set socket sending (SO_SNDTIMEO) timeout for fd " + to_string(fd));
    }

    // Return success
    return getJniResult(env, logTitle);
}

extern "C"
JNIEXPORT jobject JNICALL
Java_com_termux_shared_net_socket_local_LocalSocketManager_getPeerCredNative(JNIEnv *env, jclass clazz,
                                                                             jstring logTitle,
                                                                             jint fd, jobject peerCred) {
    if (fd < 0) {
        return getJniResult(env, logTitle, -1, "getPeerCredNative(): Invalid fd \"" + to_string(fd) + "\" passed");
    }

    if (peerCred == nullptr) {
        return getJniResult(env, logTitle, -1, "getPeerCredNative(): peerCred passed is null");
    }

    // Initialize to -1 instead of 0 in case a failed getsockopt() call somehow doesn't report failure and returns the uid of root
    struct ucred cred = {};
    cred.pid = -1; cred.uid = -1; cred.gid = -1;

    socklen_t len = sizeof(cred);

    if (getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &cred, &len) == -1) {
        return getJniResult(env, logTitle, -1, errno, "getPeerCredNative(): Failed to get peer credentials for fd " + to_string(fd));
    }

    // Fill "com.termux.shared.net.socket.local.PeerCred" object.
    // The pid, uid and gid will always be set based on ucred.
    // The pname and cmdline will only be set if current process has access to "/proc/[pid]/cmdline"
    // of peer process. Processes of other users/apps are not normally accessible.
    jclass peerCredClazz = env->GetObjectClass(peerCred);
    if (checkJniException(env)) return NULL;
    if (!peerCredClazz) {
        return getJniResult(env, logTitle, -1, errno, "getPeerCredNative(): Failed to get PeerCred class");
    }

    string error;

    error = setIntField(env, peerCred, peerCredClazz, "pid", cred.pid);
    if (!error.empty()) {
        if (error == JNI_EXCEPTION) return NULL;
        return getJniResult(env, logTitle, -1, "getPeerCredNative(): " + error);
    }

    error = setIntField(env, peerCred, peerCredClazz, "uid", cred.uid);
    if (!error.empty()) {
        if (error == JNI_EXCEPTION) return NULL;
        return getJniResult(env, logTitle, -1, "getPeerCredNative(): " + error);
    }

    error = setIntField(env, peerCred, peerCredClazz, "gid", cred.gid);
    if (!error.empty()) {
        if (error == JNI_EXCEPTION) return NULL;
        return getJniResult(env, logTitle, -1, "getPeerCredNative(): " + error);
    }

    string cmdline = get_process_cmdline(cred.pid);
    if (!cmdline.empty()) {
        error = setStringField(env, peerCred, peerCredClazz, "pname", get_process_name_from_cmdline(cmdline));
        if (!error.empty()) {
            if (error == JNI_EXCEPTION) return NULL;
            return getJniResult(env, logTitle, -1, "getPeerCredNative(): " + error);
        }

        error = setStringField(env, peerCred, peerCredClazz, "cmdline", get_process_cmdline_spaced(cmdline));
        if (!error.empty()) {
            if (error == JNI_EXCEPTION) return NULL;
            return getJniResult(env, logTitle, -1, "getPeerCredNative(): " + error);
        }
    }

    // Return success since PeerCred was filled successfully
    return getJniResult(env, logTitle);
}
