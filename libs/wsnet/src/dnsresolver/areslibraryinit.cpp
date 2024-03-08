#include "areslibraryinit.h"
#include <spdlog/spdlog.h>
#include <ares.h>

#if defined(ANDROID) || defined(__ANDROID__)
    extern JavaVM *g_javaVM;
#endif

namespace wsnet {

AresLibraryInit::AresLibraryInit() : bInitialized_(false), bFailedInitialize_(false)
{
    spdlog::info("c-ares version: {}", ARES_VERSION_STR);
}

AresLibraryInit::~AresLibraryInit()
{
    if (bInitialized_)
        ares_library_cleanup();
}

bool AresLibraryInit::init()
{
    if (!bInitialized_) {
        if (!bFailedInitialize_) {

#if defined(__ANDROID__)
            ares_library_init_jvm(g_javaVM);
#endif
            int status = ares_library_init(ARES_LIB_INIT_ALL);
            if (status != ARES_SUCCESS) {
                spdlog::critical("ares_library_init failed: {}", ares_strerror(status));
                bFailedInitialize_ = true;
                return false;
            } else  {
                if (!initAndroid()) {
                    bFailedInitialize_ = true;
                    return false;
                }
                bInitialized_ = true;
                return true;
            }
        }
    }
    return true;
}

// See: https://c-ares.org/ares_library_init_android.html
bool AresLibraryInit::initAndroid()
{
#if defined(__ANDROID__)
    JNIEnv *env = NULL;
    jint res = g_javaVM->GetEnv((void **)&env, JNI_VERSION_1_6);
    assert(res == JNI_OK);

    jclass activityThreadCls = env->FindClass("android/app/ActivityThread");
    jmethodID currentActivityThread = env->GetStaticMethodID(activityThreadCls, "currentActivityThread", "()Landroid/app/ActivityThread;");
    jobject activityThreadObj = env->CallStaticObjectMethod(activityThreadCls, currentActivityThread);
    jmethodID getApplication = env->GetMethodID(activityThreadCls, "getApplication", "()Landroid/app/Application;");
    jobject context = env->CallObjectMethod(activityThreadObj, getApplication);

    jclass obj_cls = env->FindClass("android/content/Context");
    jmethodID obj_mid = env->GetMethodID(obj_cls, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
    jfieldID fid = env->GetStaticFieldID(obj_cls, "CONNECTIVITY_SERVICE", "Ljava/lang/String;");
    jstring str = (jstring)env->GetStaticObjectField(obj_cls, fid);
    jobject connectivity_manager = env->CallObjectMethod(context, obj_mid, str);
    assert(connectivity_manager);
    int status = ares_library_init_android(connectivity_manager);
    if (status != ARES_SUCCESS) {
        spdlog::critical("ares_library_init_android failed: {}", ares_strerror(status));
        return false;
    }
#endif
    return true;
}

} // namespace wsnet
