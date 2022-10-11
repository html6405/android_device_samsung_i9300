#
# Copyright (C) 2012 The CyanogenMod Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

LOCAL_PATH := device/samsung/i9300
COMMON_PATH := device/samsung/smdk4412-common

# Overlay
DEVICE_PACKAGE_OVERLAYS += $(LOCAL_PATH)/overlay

# Screen density
PRODUCT_AAPT_CONFIG := normal
PRODUCT_AAPT_PREF_CONFIG := xhdpi

# Init files
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/rootdir/fstab.smdk4x12:$(TARGET_COPY_OUT_VENDOR)/etc/fstab.smdk4x12 \
    $(LOCAL_PATH)/rootdir/fstab.smdk4x12:$(TARGET_COPY_OUT_RAMDISK)/fstab.smdk4x12 \
    $(LOCAL_PATH)/rootdir/init.target.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.target.rc \
    $(LOCAL_PATH)/rootdir/init.target.usb.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.target.usb.rc

# HIDL
DEVICE_MANIFEST_FILE := $(LOCAL_PATH)/manifest.xml

# Audio
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/configs/tiny_hw.xml:vendor/etc/sound/i9300

# Sensors
PRODUCT_PACKAGES += \
    sensors.smdk4x12

# Gps
PRODUCT_PACKAGES += \
    gps.smdk4x12 \
    libgps_symbols

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/configs/gps.xml:vendor/etc/gps.xml

# Keylayout
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/keylayout/sec_touchkey.kl:vendor/usr/keylayout/sec_touchkey.kl

# Product specific Packages
PRODUCT_PACKAGES += \
    libsecril-client \
    libsecril-client-sap \
    tinyplay

# RIL
PRODUCT_PACKAGES += \
	libsecril-shim

# Proprietary blobs dependency on libstlport
PRODUCT_PACKAGES +=  libstlport

PRODUCT_PACKAGES += \
    camera.smdk4x12

# f2fs
PRODUCT_PACKAGES += \
	fsck.f2fs

# NFC
# PRODUCT_PACKAGES += \
	nfc.exynos4 \
    libnfc \
    libnfc_jni \
    Nfc \
    Tag

# RIL
PRODUCT_PROPERTY_OVERRIDES += \
    ro.telephony.ril_class=SamsungExynos4RIL \
    ro.telephony.call_ring.multiple=false \
    ro.telephony.call_ring.delay=3000

# These are the hardware-specific features
PRODUCT_COPY_FILES += \
	$(LOCAL_PATH)/configs/handheld_core_hardware.xml:vendor/etc/permissions/handheld_core_hardware.xml \
    frameworks/native/data/etc/android.hardware.telephony.gsm.xml:vendor/etc/permissions/android.hardware.telephony.gsm.xml
 #   frameworks/native/data/etc/android.hardware.nfc.xml:vendor/etc/permissions/android.hardware.nfc.xml

# NFCEE access control
ifeq ($(TARGET_BUILD_VARIANT),user)
    NFCEE_ACCESS_PATH := $(LOCAL_PATH)/configs/nfcee_access.xml
else
    NFCEE_ACCESS_PATH := $(LOCAL_PATH)/configs/nfcee_access_debug.xml
endif

# PRODUCT_COPY_FILES += \
    $(NFCEE_ACCESS_PATH):vendor/etc/nfcee_access.xml

# PRODUCT_PACKAGES += \
    com.android.nfc_extras

# UMS
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/configs/ums_init.sh:vendor/bin/ums_init.sh

# Apply Dalvik config for 512MB phone
$(call inherit-product, $(LOCAL_PATH)/phone-hdpi-512-dalvik-heap.mk)

$(call inherit-product-if-exists, vendor/samsung/i9300/i9300-vendor.mk)

# Vendor properties
-include $(LOCAL_PATH)/vendor_prop.mk
