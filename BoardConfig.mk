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
include device/samsung/smdk4412-common/BoardCommonConfig.mk

# Wifi
WIFI_DRIVER_MODULE_PATH :=

# Bionic
MALLOC_SVELTE := true
TARGET_NEEDS_PLATFORM_TEXT_RELOCATIONS := true
LIBART_IMG_BASE := 0x30000000

TARGET_LD_SHIM_LIBS := \
    /vendor/bin/glgps|libgps_symbols.so

# Graphics
TARGET_REQUIRES_SYNCHRONOUS_SETSURFACE := true

# Bluetooth
BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR := device/samsung/i9300/bluetooth

# Kernel
TARGET_KERNEL_SOURCE := kernel/samsung/smdk4412
TARGET_KERNEL_CONFIG := lineageos_i9300_defconfig

NEED_KERNEL_MODULE_SYSTEM := true

TARGET_SPECIFIC_HEADER_PATH += device/samsung/i9300/include

# Cache
BOARD_CACHEIMAGE_PARTITION_SIZE :=104857600
BOARD_CACHEIMAGE_FILE_SYSTEM_TYPE := ext4

# Vendor
BOARD_VENDORIMAGE_PARTITION_SIZE := 587202560

# Recovery
TARGET_RECOVERY_FSTAB := device/samsung/i9300/rootdir/fstab.smdk4x12
TARGET_RECOVERY_DENSITY := mdpi
TARGET_USERIMAGES_USE_F2FS := true
RECOVERY_FSTAB_VERSION := 2

# PowerHAL
TARGET_POWERHAL_VARIANT := pegasusq

# assert
TARGET_OTA_ASSERT_DEVICE := m0,i9300,GT-I9300

# Init
ifneq ($(WITH_TWRP), true)
TARGET_INIT_VENDOR_LIB := libinit_i9300
endif

# SELinux
BOARD_SEPOLICY_DIRS += device/samsung/i9300/selinux/vendor
SYSTEM_EXT_PUBLIC_SEPOLICY_DIRS += device/samsung/i9300/selinux/public
SYSTEM_EXT_PRIVATE_SEPOLICY_DIRS += device/samsung/i9300/selinux/private