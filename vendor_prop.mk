#
# Copyright (C) 2020 The LineageOS Project
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

#
# system.prop for n8000
#
PRODUCT_PROPERTY_OVERRIDES += \
	config.disable_atlas=true \
	dalvik.vm.dexopt-data-only=1 \
	dalvik.vm.dex2oat-Xmx=256m \
	rild.libpath=/vendor/lib/libsecril-shim.so \
	ro.ril.telephony.mqanelements=5 \
	ro.sf.lcd_density=320 \
	ro.lcd_min_brightness=20 \
	ro.ril.telephony.nstrings=4

# EGL blobs crash on screen off
PRODUCT_PROPERTY_OVERRIDES += \
	ro.egl.destroy_after_detach=true

# Camera API
PRODUCT_PROPERTY_OVERRIDES += \
	persist.camera.HAL3.enabled=1

# RAM optimizations
PRODUCT_PROPERTY_OVERRIDES += \
	config.disable_atlas=true \
	ro.sys.fw.trim_enable_memory=805306368 \
	ro.sys.fw.use_trim_settings=true \
	ro.sys.fw.empty_app_percent=25 \
	ro.sys.fw.trim_empty_percent=50 \
	ro.sys.fw.trim_cache_percent=50 \
	ro.sys.fw.bg_apps_limit=16 \
	ro.sys.fw.bservice_limit=7 \
	ro.sys.fw.bservice_age=6000 \
	ro.sys.fw.bservice_enable=true

# hwui
PRODUCT_PROPERTY_OVERRIDES += \
	ro.hwui.drop_shadow_cache_size=1 \
	ro.hwui.gradient_cache_size=0.2 \
	ro.hwui.layer_cache_size=6 \
	ro.hwui.path_cache_size=2 \
	ro.hwui.r_buffer_cache_size=1 \
	ro.hwui.texture_cache_size=8

# Free up RAM by purging assets.
PRODUCT_PROPERTY_OVERRIDES += \
	persist.sys.purgeable_assets=1

# Better RAM management
PRODUCT_PROPERTY_OVERRIDES += \
	ro.HOME_APP_ADJ=1

# Force high-end graphics in low ram mode
PRODUCT_PROPERTY_OVERRIDES += \
	persist.sys.force_highendgfx=true

# Reduce background apps limit to 16 on low-tier devices
PRODUCT_PROPERTY_OVERRIDES += \
	ro.sys.fw.bg_apps_limit=16

# Set max background services
PRODUCT_PROPERTY_OVERRIDES += \
	ro.config.max_starting_bg=4

# GPS
PRODUCT_PROPERTY_OVERRIDES += \
	ro.ril.def.agps.mode=1

# Wifi
PRODUCT_PROPERTY_OVERRIDES += \
	wifi.supplicant_scan_interval=240