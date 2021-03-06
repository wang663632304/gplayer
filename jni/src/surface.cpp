#include <stdlib.h>
#define FROM_SURFACE_CODE 1
#include "surface.h"
#ifdef ANDROID4
#include <gui/Surface.h>
#else
#include <surfaceflinger/Surface.h>
#endif
#include <utils/Log.h>
#include <SkBitmap.h>
#include <SkCanvas.h>
#define TAG "SurfaceWrapper"
#include "trace.h"
#define SDK_VERSION_FROYO 8

namespace android { 
	extern "C" {
		static Surface*	sSurface;
		static SkBitmap	sBitmapClient;
		static SkBitmap	sBitmapSurface;

		static Surface* getNativeSurface(JNIEnv* env, jobject jsurface,int sdkVersion) {
			jclass clazz = env->FindClass("android/view/Surface");
			jfieldID field_surface = env->GetFieldID(clazz, 
					sdkVersion > SDK_VERSION_FROYO ? "mNativeSurface" : "mSurface", "I");//mSurface if froyo
			if(field_surface == NULL) {
				return NULL;
			}
			return (Surface *) env->GetIntField(jsurface, field_surface);
		}

		static int initBitmap(SkBitmap *bitmap, int format, int width, int height, bool allocPixels) {
			switch (format) {
				case PIXEL_FORMAT_RGBA_8888:
					bitmap->setConfig(SkBitmap::kARGB_8888_Config, width, height);
					break;

				case PIXEL_FORMAT_RGBA_4444:
					bitmap->setConfig(SkBitmap::kARGB_4444_Config, width, height);
					break;

				case PIXEL_FORMAT_RGB_565:
					bitmap->setConfig(SkBitmap::kRGB_565_Config, width, height);
					break;

				case PIXEL_FORMAT_A_8:
					bitmap->setConfig(SkBitmap::kA8_Config, width, height);
					break;

				default:
					bitmap->setConfig(SkBitmap::kNo_Config, width, height);
					break;
			}

			if(allocPixels) {
				bitmap->setIsOpaque(true);
				//-- alloc array of pixels
				if(!bitmap->allocPixels()) {
					return -1;
				}
			}
			return 0;
		}

		int AndroidSurface_register(JNIEnv* env, jobject jsurface, int sdkVersion) {
			__android_log_print(ANDROID_LOG_INFO, TAG, "registering video surface");

			sSurface = getNativeSurface(env, jsurface, sdkVersion);
			if(sSurface == NULL) {
				return ANDROID_SURFACE_RESULT_JNI_EXCEPTION;
			}

			__android_log_print(ANDROID_LOG_INFO, TAG, "registered");

			return ANDROID_SURFACE_RESULT_SUCCESS;
		}

		int AndroidSurface_getPixels(int width, int height, void** pixels) {
			__android_log_print(ANDROID_LOG_INFO, TAG, "getting surface's pixels %ix%i", width, height);
			if(sSurface == NULL) {
				return ANDROID_SURFACE_RESULT_JNI_EXCEPTION;
			}
			if(initBitmap(&sBitmapClient, PIXEL_FORMAT_RGB_565, width, height, true) < 0) {
				return ANDROID_SURFACE_RESULT_COULDNT_INIT_BITMAP_CLIENT;
			}
			*pixels = sBitmapClient.getPixels();
			__android_log_print(ANDROID_LOG_INFO, TAG, "getted");
			return ANDROID_SURFACE_RESULT_SUCCESS;
		}

		static void doUpdateSurface() {
			SkCanvas	canvas(sBitmapSurface);
			SkRect	surface_sBitmapClient;
			SkRect	surface_sBitmapSurface;
			SkMatrix	matrix;

			surface_sBitmapSurface.set(0, 0, sBitmapSurface.width(), sBitmapSurface.height());
			surface_sBitmapClient.set(0, 0, sBitmapClient.width(), sBitmapClient.height());
			matrix.setRectToRect(surface_sBitmapClient, surface_sBitmapSurface, SkMatrix::kFill_ScaleToFit);

			canvas.drawBitmapMatrix(sBitmapClient, matrix);
		}

		static int prepareSurfaceBitmap(Surface::SurfaceInfo* info) {
			if(initBitmap(&sBitmapSurface, info->format, info->w, info->h, false) < 0) {
				return -1;
			}
			sBitmapSurface.setPixels(info->bits);
			return 0;
		}

		int AndroidSurface_updateSurface() {
			static Surface::SurfaceInfo surfaceInfo;
			if(sSurface == NULL) {
				return ANDROID_SURFACE_RESULT_JNI_EXCEPTION;
			}
			if (!sSurface->isValid()) {
				return ANDROID_SURFACE_RESULT_NOT_VALID;
			}
			if (sSurface->lock(&surfaceInfo) < 0) {
				return ANDROID_SURFACE_RESULT_COULDNT_LOCK;
			}

			if(prepareSurfaceBitmap(&surfaceInfo) < 0) {
				return ANDROID_SURFACE_RESULT_COULDNT_INIT_BITMAP_SURFACE;
			}

			doUpdateSurface();

			if (sSurface->unlockAndPost() < 0) {
				return ANDROID_SURFACE_RESULT_COULDNT_UNLOCK_AND_POST;
			}
			return ANDROID_SURFACE_RESULT_SUCCESS;
		}

		int AndroidSurface_unregister() {
			__android_log_print(ANDROID_LOG_INFO, TAG, "unregistering video surface");
			__android_log_print(ANDROID_LOG_INFO, TAG, "unregistered");
			return ANDROID_SURFACE_RESULT_SUCCESS;
		}

	}

};
