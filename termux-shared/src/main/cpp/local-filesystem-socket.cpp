#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ioctl.h>

#include <ctime>
#include <cstdio>
#include <unistd.h>
#include <cerrno>

#include <jni.h>

#include <android/log.h>





int64_t millisecondtime(const struct timespec* const time) {
    return (((int64_t)time->tv_sec)*1000)+(((int64_t)time->tv_nsec)/1000000);
}



/// Sets the timeout in microseconds
void settimeout_micro(int fd, int timeout) {
    struct timeval t = {};
    t.tv_sec = 0;
    t.tv_usec = timeout;
    socklen_t len = sizeof(t);
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &t, len);
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &t, len);
}

/// Sets the timeout in milliseconds
void settimeout(int fd, int timeout) {
    settimeout_micro(fd, timeout*1000);
}


extern "C"
JNIEXPORT jint JNICALL
Java_com_termux_shared_shell_LocalFilesystemSocket_createserversocket(JNIEnv *env, jclass clazz, jbyteArray path, jint backlog) {
    if (backlog < 1) {
        return -1;
    }
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1) {
        return -1;
    }
    jbyte* p = env->GetByteArrayElements(path, nullptr);
    if (p == nullptr) {
        close(fd);
        return -1;
    }
    int chars = env->GetArrayLength(path);
    if (chars >= sizeof(struct sockaddr_un)-sizeof(sa_family_t)) {
        env->ReleaseByteArrayElements(path, p, JNI_ABORT);
        close(fd);
        return -1;
    }
    struct sockaddr_un adr = {.sun_family = AF_UNIX};
    memcpy(&adr.sun_path, p, chars);
    if (bind(fd, reinterpret_cast<struct sockaddr*>(&adr), sizeof(adr)) == -1) {
        env->ReleaseByteArrayElements(path, p, JNI_ABORT);
        close(fd);
        return -1;
    }
    if (listen(fd, backlog) == -1) {
        env->ReleaseByteArrayElements(path, p, JNI_ABORT);
        close(fd);
        return -1;
    }
    return fd;
}


extern "C"
JNIEXPORT void JNICALL
Java_com_termux_shared_shell_LocalFilesystemSocket_closefd(JNIEnv *env, jclass clazz, jint fd) {
    close(fd);
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_termux_shared_shell_LocalFilesystemSocket_accept(JNIEnv *env, jclass clazz, jint fd) {
    int c = accept(fd, nullptr, nullptr);
    if (c == -1) {
        return -1;
    }
    return c;
}




extern "C"
JNIEXPORT jboolean JNICALL
Java_com_termux_shared_shell_LocalFilesystemSocket_send(JNIEnv *env, jclass clazz, jbyteArray data, jint fd, jlong deadline) {
    if (fd == -1) {
        return false;
    }
    jbyte* d = env->GetByteArrayElements(data, nullptr);
    if (d == nullptr) {
        return false;
    }
    struct timespec time = {};
    jbyte* current = d;
    int bytes = env->GetArrayLength(data);
    while (bytes > 0) {
        if (clock_gettime(CLOCK_REALTIME, &time) != -1) {
            if (millisecondtime(&time) > deadline) {
                env->ReleaseByteArrayElements(data, d, JNI_ABORT);
                return false;
            }
        } else {
            __android_log_write(ANDROID_LOG_WARN, "LocalFilesystemSocket.send", "Could not get the current time, deadline will not work");
        }
        int ret = send(fd, current, bytes, MSG_NOSIGNAL);
        if (ret == -1) {
            __android_log_print(ANDROID_LOG_DEBUG, "LocalFilesystemSocket.send", "%s", strerror(errno));
            env->ReleaseByteArrayElements(data, d, JNI_ABORT);
            return false;
        }
        bytes -= ret;
        current += ret;
    }
    env->ReleaseByteArrayElements(data, d, JNI_ABORT);
    return true;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_termux_shared_shell_LocalFilesystemSocket_recv(JNIEnv *env, jclass clazz, jbyteArray data, jint fd, jlong deadline) {
    if (fd == -1) {
        return -1;
    }
    jbyte* d = env->GetByteArrayElements(data, nullptr);
    if (d == nullptr) {
        return -1;
    }
    struct timespec time = {};
    jbyte* current = d;
    int bytes = env->GetArrayLength(data);
    int transferred = 0;
    while (transferred < bytes) {
        if (clock_gettime(CLOCK_REALTIME, &time) != -1) {
            if (millisecondtime(&time) > deadline) {
                env->ReleaseByteArrayElements(data, d, 0);
                return -1;
            }
        } else {
            __android_log_write(ANDROID_LOG_WARN, "LocalFilesystemSocket.recv", "Could not get the current time, deadline will not work");
        }
        int ret = read(fd, current, bytes);
        if (ret == -1) {
            env->ReleaseByteArrayElements(data, d, 0);
            return -1;
        }
        // EOF, peer closed writing end
        if (ret == 0) {
            break;
        }
        transferred += ret;
        current += ret;
    }
    env->ReleaseByteArrayElements(data, d, 0);
    return transferred;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_termux_shared_shell_LocalFilesystemSocket_settimeout(JNIEnv *env, jclass clazz, jint fd, jint timeout) {
    settimeout(fd, timeout);
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_termux_shared_shell_LocalFilesystemSocket_available(JNIEnv *env, jclass clazz, jint fd) {
    int size = 0;
    if (ioctl(fd, SIOCINQ, &size) == -1) {
        return 0;
    }
    return size;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_termux_shared_shell_LocalFilesystemSocket_getpeeruid(JNIEnv *env, jclass clazz, jint fd) {
    struct ucred cred = {};
    cred.uid = 1; // initialize uid to 1 here because I'm paranoid and a failed getsockopt that somehow doesn't report as failed would report the uid of root
    socklen_t len = sizeof(cred);
    if (getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &cred, &len) == -1) {
        return -1;
    }
    return cred.uid;
}
