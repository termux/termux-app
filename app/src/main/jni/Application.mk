APP_ABI := arm64-v8a armeabi-v7a x86
APP_PLATFORM := android-21
NDK_TOOLCHAIN_VERSION := 4.9
APP_CFLAGS := -std=c11 -Wall -Wextra -Os -fno-stack-protector
APP_LDFLAGS = -nostdlib -Wl,--gc-sections
