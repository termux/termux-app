#include <jni.h>
#include <cstdint>
#include <vector>
#include <limits>
#include "vorbislib.h"

namespace {
void raise(JNIEnv* env,const char* text) {
    jclass type=env->FindClass("java/io/IOException");
    if(type)env->ThrowNew(type,text);
}
bool readInput(JNIEnv* env,jbyteArray input,std::vector<uint8_t>& data){
    if(!input){raise(env,"Missing audio bytes");return false;}
    jsize size=env->GetArrayLength(input);
    if(size<=0){raise(env,"Input is empty");return false;}
    data.resize(static_cast<size_t>(size));
    env->GetByteArrayRegion(input,0,size,reinterpret_cast<jbyte*>(data.data()));
    return !env->ExceptionCheck();
}
jbyteArray result(JNIEnv* env,vl_status status,vl_buffer& output){
    if(status!=VL_OK) {
        raise(env,vl_last_error());return nullptr;
    }
    if(!output.data || output.size==0 ||
       output.size>static_cast<size_t>(std::numeric_limits<jsize>::max())){
        raise(env,"Invalid native Vorbis output size");return nullptr;
    }
    auto bytes=env->NewByteArray(static_cast<jsize>(output.size));
    if(bytes)env->SetByteArrayRegion(bytes,0,static_cast<jsize>(output.size),
       reinterpret_cast<const jbyte*>(output.data));
    return bytes;
}
}
extern "C" JNIEXPORT jbyteArray JNICALL
Java_com_termux_app_MLBBNativeVorbis_nativeEncode(JNIEnv* env,jclass,jbyteArray audio,jfloat quality,jint maxFrames){
    if(maxFrames<=0||quality < -0.1f || quality>1.0f){
        raise(env,"Invalid quality or frame count");return nullptr;
    }
    std::vector<uint8_t> source;
    if(!readInput(env,audio,source))return nullptr;
    vl_buffer output{};
    vl_status status=vl_audio_to_wem(source.data(),source.size(),quality,
        static_cast<uint32_t>(maxFrames),&output);
    jbyteArray out=result(env,status,output);
    vl_free(output.data);
    return out;
}
extern "C" JNIEXPORT jbyteArray JNICALL
Java_com_termux_app_MLBBNativeVorbis_nativeToOgg(JNIEnv* env,jclass,jbyteArray wem,jboolean legacy){
    std::vector<uint8_t> source;
    if(!readInput(env,wem,source))return nullptr;
    vl_buffer output{};
    vl_status status=vl_wem_to_ogg(source.data(),source.size(),
        legacy?VL_LEGACY_CODEBOOKS:VL_AOTUV_603,&output);
    jbyteArray out=result(env,status,output);
    vl_free(output.data);
    return out;
}
