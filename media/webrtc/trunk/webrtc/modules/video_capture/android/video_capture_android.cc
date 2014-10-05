/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/modules/video_capture/android/video_capture_android.h"

#include <stdio.h>

#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/system_wrappers/interface/ref_count.h"
#include "webrtc/system_wrappers/interface/trace.h"

#include "AndroidJNIWrapper.h"
#include "mozilla/Assertions.h"

namespace webrtc
{
#if defined(WEBRTC_ANDROID) && !defined(WEBRTC_CHROMIUM_BUILD)
// TODO(leozwang) These SetAndroidVM apis will be refactored, thus we only
// keep and reference java vm.
int32_t SetCaptureAndroidVM(void* javaVM, void* javaContext) {
  return videocapturemodule::VideoCaptureAndroid::SetAndroidObjects(
      javaVM,
      javaContext);
}
#endif

namespace videocapturemodule
{

VideoCaptureModule* VideoCaptureImpl::Create(
    const int32_t id,
    const char* deviceUniqueIdUTF8) {

  RefCountImpl<videocapturemodule::VideoCaptureAndroid>* implementation =
      new RefCountImpl<videocapturemodule::VideoCaptureAndroid>(id);

  if (!implementation || implementation->Init(id, deviceUniqueIdUTF8) != 0) {
    delete implementation;
    implementation = NULL;
  }
  return implementation;
}

#ifdef DEBUG
// Android logging, uncomment to print trace to
// logcat instead of trace file/callback
#include <android/log.h>
// #undef WEBRTC_TRACE
// #define WEBRTC_TRACE(a,b,c,...)
// __android_log_print(ANDROID_LOG_DEBUG, "*WEBRTCN*", __VA_ARGS__)
// Some functions are called before before the WebRTC logging can be brought up,
// log those to the Android log.
#define EARLY_WEBRTC_TRACE(a,b,c,...) __android_log_print(ANDROID_LOG_DEBUG, "*WEBRTC-VCA", __VA_ARGS__)
#else
#define EARLY_WEBRTC_TRACE(a,b,c,...)
#endif

JavaVM* VideoCaptureAndroid::g_jvm = NULL;
//VideoCaptureAndroid.java
jclass VideoCaptureAndroid::g_javaCmClass = NULL;
//VideoCaptureDeviceInfoAndroid.java
jclass VideoCaptureAndroid::g_javaCmDevInfoClass = NULL;
//static instance of VideoCaptureDeviceInfoAndroid.java
jobject VideoCaptureAndroid::g_javaCmDevInfoObject = NULL;

/*
 * Register references to Java Capture class.
 */
int32_t VideoCaptureAndroid::SetAndroidObjects(void* javaVM,
                                               void* javaContext) {

  MOZ_ASSERT(javaVM != nullptr || g_javaCmDevInfoClass != nullptr);
  EARLY_WEBRTC_TRACE(webrtc::kTraceStateInfo, webrtc::kTraceVideoCapture, -1,
               "%s: running", __FUNCTION__);

  g_jvm = static_cast<JavaVM*> (javaVM);

  if (javaVM) {
    // Already done? Exit early.
    if (g_javaCmClass != NULL
        && g_javaCmDevInfoClass != NULL
        && g_javaCmDevInfoObject != NULL) {
        EARLY_WEBRTC_TRACE(webrtc::kTraceStateInfo, webrtc::kTraceVideoCapture, -1,
                     "%s: early exit", __FUNCTION__);
        return 0;
    }

    JNIEnv* env = NULL;
    if (g_jvm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
      EARLY_WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, -1,
                   "%s: could not get Java environment", __FUNCTION__);
      return -1;
    }
    // get java capture class type (note path to class packet)
    g_javaCmClass = jsjni_GetGlobalClassRef(AndroidJavaCaptureClass);
    if (!g_javaCmClass) {
      EARLY_WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, -1,
                   "%s: could not find java class", __FUNCTION__);
      return -1;
    }
    JNINativeMethod nativeFunctions =
        { "ProvideCameraFrame", "([BIIJ)V",
          (void*) &VideoCaptureAndroid::ProvideCameraFrame };
    if (env->RegisterNatives(g_javaCmClass, &nativeFunctions, 1) == 0) {
      EARLY_WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCapture, -1,
                   "%s: Registered native functions", __FUNCTION__);
    }
    else {
      EARLY_WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, -1,
                   "%s: Failed to register native functions",
                   __FUNCTION__);
      return -1;
    }

    // get java capture class type (note path to class packet)
    g_javaCmDevInfoClass = jsjni_GetGlobalClassRef(
                 AndroidJavaCaptureDeviceInfoClass);
    if (!g_javaCmDevInfoClass) {
      EARLY_WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, -1,
                   "%s: could not find java class", __FUNCTION__);
      return -1;
    }

    EARLY_WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCapture, -1,
                 "VideoCaptureDeviceInfoAndroid get method id");

    // get the method ID for the Android Java CaptureClass static
    //CreateVideoCaptureAndroid factory method.
    jmethodID cid = env->GetStaticMethodID(
        g_javaCmDevInfoClass,
        "CreateVideoCaptureDeviceInfoAndroid",
        "(ILandroid/content/Context;)"
        "Lorg/webrtc/videoengine/VideoCaptureDeviceInfoAndroid;");
    if (cid == NULL) {
      EARLY_WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, -1,
                   "%s: could not get java"
                   "VideoCaptureDeviceInfoAndroid constructor ID",
                   __FUNCTION__);
      return -1;
    }

    EARLY_WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCapture, -1,
                 "%s: construct static java device object", __FUNCTION__);

    // construct the object by calling the static constructor object
    jobject javaCameraDeviceInfoObjLocal =
        env->CallStaticObjectMethod(g_javaCmDevInfoClass,
                                    cid, (int) -1,
                                    javaContext);
    bool exceptionThrown = env->ExceptionCheck();
    if (!javaCameraDeviceInfoObjLocal || exceptionThrown) {
      if (exceptionThrown) {
        env->ExceptionDescribe();
        env->ExceptionClear();
      }
      EARLY_WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceVideoCapture, -1,
                   "%s: could not create Java Capture Device info object",
                   __FUNCTION__);
      return -1;
    }
    // create a reference to the object (to tell JNI that
    // we are referencing it after this function has returned)
    g_javaCmDevInfoObject = env->NewGlobalRef(javaCameraDeviceInfoObjLocal);
    if (!g_javaCmDevInfoObject) {
      EARLY_WEBRTC_TRACE(webrtc::kTraceError,
                   webrtc::kTraceAudioDevice,
                   -1,
                   "%s: could not create Java"
                   "cameradevinceinfo object reference",
                   __FUNCTION__);
      return -1;
    }
    // Delete local object ref, we only use the global ref
    env->DeleteLocalRef(javaCameraDeviceInfoObjLocal);

    EARLY_WEBRTC_TRACE(webrtc::kTraceStateInfo, webrtc::kTraceVideoCapture, -1,
                 "%s: success", __FUNCTION__);
    return 0;
  }
  else {
    EARLY_WEBRTC_TRACE(webrtc::kTraceStateInfo, webrtc::kTraceVideoCapture, -1,
                 "%s: JVM is NULL, assuming deinit", __FUNCTION__);
    if (!g_jvm) {
      EARLY_WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, -1,
                   "%s: SetAndroidObjects not called with a valid JVM.",
                   __FUNCTION__);
      return -1;
    }
    JNIEnv* env = NULL;
    bool attached = false;
    if (g_jvm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
      // try to attach the thread and get the env
      // Attach this thread to JVM
      jint res = g_jvm->AttachCurrentThread(&env, NULL);
      if ((res < 0) || !env) {
        EARLY_WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture,
                     -1, "%s: Could not attach thread to JVM (%d, %p)",
                     __FUNCTION__, res, env);
        return -1;
      }
      attached = true;
    }
    env->DeleteGlobalRef(g_javaCmDevInfoObject);
    env->DeleteGlobalRef(g_javaCmDevInfoClass);
    env->DeleteGlobalRef(g_javaCmClass);
    if (attached && g_jvm->DetachCurrentThread() < 0) {
      EARLY_WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceVideoCapture, -1,
                   "%s: Could not detach thread from JVM", __FUNCTION__);
      return -1;
    }
    return 0;
    env = (JNIEnv *) NULL;
  }
  return 0;
}

/*
 * JNI callback from Java class. Called
 * when the camera has a new frame to deliver
 * Class:     org_webrtc_capturemodule_VideoCaptureAndroid
 * Method:    ProvideCameraFrame
 * Signature: ([BIJ)V
 */
void JNICALL VideoCaptureAndroid::ProvideCameraFrame(JNIEnv * env,
                                                     jobject,
                                                     jbyteArray javaCameraFrame,
                                                     jint length,
                                                     jint rotation,
                                                     jlong context) {
  VideoCaptureAndroid* captureModule =
      reinterpret_cast<VideoCaptureAndroid*>(context);
  WEBRTC_TRACE(webrtc::kTraceInfo, webrtc::kTraceVideoCapture,
               -1, "%s: IncomingFrame %d", __FUNCTION__,length);

  switch (rotation) {
    case 90:
      captureModule->SetCaptureRotation(kCameraRotate90);
      break;
    case 180:
      captureModule->SetCaptureRotation(kCameraRotate180);
      break;
    case 270:
      captureModule->SetCaptureRotation(kCameraRotate270);
      break;
    case 0:
    default:
      captureModule->SetCaptureRotation(kCameraRotate0);
      break;
  }

  jbyte* cameraFrame= env->GetByteArrayElements(javaCameraFrame,NULL);
  captureModule->IncomingFrame((uint8_t*) cameraFrame,
                               length,captureModule->_frameInfo,0);
  env->ReleaseByteArrayElements(javaCameraFrame,cameraFrame,JNI_ABORT);
}



VideoCaptureAndroid::VideoCaptureAndroid(const int32_t id)
    : VideoCaptureImpl(id), _capInfo(id), _javaCaptureObj(NULL),
      _captureStarted(false) {
  WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCapture, -1,
               "%s: context %x", __FUNCTION__, (int) this);
}

// ----------------------------------------------------------------------------
//  Init
//
//  Initializes needed Java resources like the JNI interface to
//  VideoCaptureAndroid.java
// ----------------------------------------------------------------------------
int32_t VideoCaptureAndroid::Init(const int32_t id,
                                        const char* deviceUniqueIdUTF8) {
  const int nameLength = strlen(deviceUniqueIdUTF8);
  if (nameLength >= kVideoCaptureUniqueNameLength) {
    return -1;
  }

  // Store the device name
  _deviceUniqueId = new char[nameLength + 1];
  memcpy(_deviceUniqueId, deviceUniqueIdUTF8, nameLength + 1);

  if (_capInfo.Init() != 0) {
    WEBRTC_TRACE(webrtc::kTraceError,
                 webrtc::kTraceVideoCapture,
                 _id,
                 "%s: Failed to initialize CaptureDeviceInfo",
                 __FUNCTION__);
    return -1;
  }

  WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCapture, -1, "%s:",
               __FUNCTION__);
  // use the jvm that has been set
  if (!g_jvm) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, _id,
                 "%s: Not a valid Java VM pointer", __FUNCTION__);
    return -1;
  }

  AutoLocalJNIFrame jniFrame;
  JNIEnv* env = jniFrame.GetEnv();
  if (!env)
      return -1;

  jclass javaCmDevInfoClass = jniFrame.GetCmDevInfoClass();
  jobject javaCmDevInfoObject = jniFrame.GetCmDevInfoObject();

  WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCapture, _id,
               "get method id");
  // get the method ID for the Android Java
  // CaptureDeviceInfoClass AllocateCamera factory method.
  char signature[256];
  sprintf(signature, "(IJLjava/lang/String;)L%s;", AndroidJavaCaptureClass);

  jmethodID cid = env->GetMethodID(javaCmDevInfoClass, "AllocateCamera",
                                   signature);
  if (cid == NULL) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, _id,
                 "%s: could not get constructor ID", __FUNCTION__);
    return -1; /* exception thrown */
  }

  jstring capureIdString = env->NewStringUTF((char*) deviceUniqueIdUTF8);
  // construct the object by calling the static constructor object
  jobject javaCameraObjLocal = env->CallObjectMethod(javaCmDevInfoObject,
                                                     cid, (jint) id,
                                                     (jlong) this,
                                                     capureIdString);
  if (!javaCameraObjLocal || jniFrame.CheckForException()) {
    WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceVideoCapture, _id,
                 "%s: could not create Java Capture object", __FUNCTION__);
    return -1;
  }

  // create a reference to the object (to tell JNI that we are referencing it
  // after this function has returned)
  _javaCaptureObj = env->NewGlobalRef(javaCameraObjLocal);
  if (!_javaCaptureObj) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioDevice, _id,
                 "%s: could not create Java camera object reference",
                 __FUNCTION__);
    return -1;
  }

  return 0;
}

VideoCaptureAndroid::~VideoCaptureAndroid() {
  WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCapture, -1, "%s:",
               __FUNCTION__);
  if (_javaCaptureObj == NULL || g_jvm == NULL) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, -1,
                 "%s: Nothing to clean", __FUNCTION__);
  }
  else {
    AutoLocalJNIFrame jniFrame;
    JNIEnv* env = jniFrame.GetEnv();
    if (!env)
        return;

    // get the method ID for the Android Java CaptureClass static
    // DeleteVideoCaptureAndroid  method. Call this to release the camera so
    // another application can use it.
    jmethodID cid = env->GetStaticMethodID(g_javaCmClass,
                                           "DeleteVideoCaptureAndroid",
                                           "(Lorg/webrtc/videoengine/VideoCaptureAndroid;)V");
    if (cid != NULL) {
      WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCapture, -1,
                   "%s: Call DeleteVideoCaptureAndroid", __FUNCTION__);
      // Close the camera by calling the static destruct function.
      env->CallStaticVoidMethod(g_javaCmClass, cid, _javaCaptureObj);
      jniFrame.CheckForException();

      // Delete global object ref to the camera.
      env->DeleteGlobalRef(_javaCaptureObj);
      _javaCaptureObj = NULL;
    } else {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, -1,
                     "%s: Failed to find DeleteVideoCaptureAndroid id",
                     __FUNCTION__);
    }
  }
}

int32_t VideoCaptureAndroid::StartCapture(
    const VideoCaptureCapability& capability) {
  CriticalSectionScoped cs(&_apiCs);
  WEBRTC_TRACE(webrtc::kTraceStateInfo, webrtc::kTraceVideoCapture, -1,
               "%s: ", __FUNCTION__);

  int32_t result = 0;

  AutoLocalJNIFrame jniFrame;
  JNIEnv* env = jniFrame.GetEnv();
  if (!env)
      return -1;

  if (_capInfo.GetBestMatchedCapability(_deviceUniqueId, capability,
                                        _frameInfo) < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, -1,
                 "%s: GetBestMatchedCapability failed. Req cap w%d h%d",
                 __FUNCTION__, capability.width, capability.height);
    return -1;
  }

  // Store the new expected capture delay
  _captureDelay = _frameInfo.expectedCaptureDelay;

  WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCapture, -1,
               "%s: _frameInfo w%d h%d", __FUNCTION__, _frameInfo.width,
               _frameInfo.height);

  // get the method ID for the Android Java
  // CaptureClass static StartCapture  method.
  jmethodID cid = env->GetMethodID(g_javaCmClass, "StartCapture", "(III)I");
  if (cid != NULL) {
    WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCapture, -1,
                 "%s: Call StartCapture", __FUNCTION__);
    // Close the camera by calling the static destruct function.
    result = env->CallIntMethod(_javaCaptureObj, cid, _frameInfo.width,
                                _frameInfo.height, _frameInfo.maxFPS);
  }
  else {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, -1,
                 "%s: Failed to find StartCapture id", __FUNCTION__);
  }

  if (result == 0) {
    _requestedCapability = capability;
    _captureStarted = true;
  }
  WEBRTC_TRACE(webrtc::kTraceStateInfo, webrtc::kTraceVideoCapture, -1,
               "%s: result %d", __FUNCTION__, result);
  return result;
}

int32_t VideoCaptureAndroid::StopCapture() {
  CriticalSectionScoped cs(&_apiCs);
  WEBRTC_TRACE(webrtc::kTraceStateInfo, webrtc::kTraceVideoCapture, -1,
               "%s: ", __FUNCTION__);

  int32_t result = 0;

  AutoLocalJNIFrame jniFrame;
  JNIEnv* env = jniFrame.GetEnv();
  if (!env)
      return -1;

  memset(&_requestedCapability, 0, sizeof(_requestedCapability));
  memset(&_frameInfo, 0, sizeof(_frameInfo));

  // get the method ID for the Android Java CaptureClass StopCapture  method.
  jmethodID cid = env->GetMethodID(g_javaCmClass, "StopCapture", "()I");
  if (cid != NULL) {
    WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCapture, -1,
                 "%s: Call StopCapture", __FUNCTION__);
    // Close the camera by calling the static destruct function.
    result = env->CallIntMethod(_javaCaptureObj, cid);
  }
  else {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, -1,
                 "%s: Failed to find StopCapture id", __FUNCTION__);
  }

  _captureStarted = false;

  WEBRTC_TRACE(webrtc::kTraceStateInfo, webrtc::kTraceVideoCapture, -1,
               "%s: result %d", __FUNCTION__, result);
  return result;
}

bool VideoCaptureAndroid::CaptureStarted() {
  CriticalSectionScoped cs(&_apiCs);
  WEBRTC_TRACE(webrtc::kTraceStateInfo, webrtc::kTraceVideoCapture, -1,
               "%s: ", __FUNCTION__);
  return _captureStarted;
}

int32_t VideoCaptureAndroid::CaptureSettings(
    VideoCaptureCapability& settings) {
  CriticalSectionScoped cs(&_apiCs);
  WEBRTC_TRACE(webrtc::kTraceStateInfo, webrtc::kTraceVideoCapture, -1,
               "%s: ", __FUNCTION__);
  settings = _requestedCapability;
  return 0;
}

int32_t VideoCaptureAndroid::SetCaptureRotation(
    VideoCaptureRotation rotation) {
  CriticalSectionScoped cs(&_apiCs);
  return VideoCaptureImpl::SetCaptureRotation(rotation);
}

}  // namespace videocapturemodule
}  // namespace webrtc
