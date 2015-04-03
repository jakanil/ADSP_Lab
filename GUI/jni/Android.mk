LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := libSpeechProcessing
LOCAL_SRC_FILES := FIR.c VAD.c MFCC.c Transforms.c Timer.c GMM.c Classifier.c LogMMSE.c Synthesis.c SpeechProcessing.c
LOCAL_CFLAGS := -O3

#LOCAL_CFLAGS += -DLOGCAT_DEBUG=1

LOCAL_LDLIBS := -llog -landroid

include $(BUILD_SHARED_LIBRARY)
