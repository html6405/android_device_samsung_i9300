/*
   Copyright (c) 2016, The Linux Foundation. All rights reserved.
   Copyright (c) 2017-2020, The LineageOS Project. All rights reserved.
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of The Linux Foundation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.
   THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
   ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
   BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
   BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
   OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
   IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#include <sys/_system_properties.h>

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android-base/strings.h>

#include "property_service.h"
#include "vendor_init.h"

using android::base::GetProperty;
using android::base::ReadFileToString;
using android::base::Trim;

#define SERIAL_NUMBER_FILE "/efs/FactoryApp/serial_no"

void property_override(char const prop[], char const value[])
{
    prop_info *pi;

    pi = (prop_info*) __system_property_find(prop);
    if (pi)
        __system_property_update(pi, value, strlen(value));
    else
        __system_property_add(prop, strlen(prop), value, strlen(value));
}

void property_override_dual(char const system_prop[], char const vendor_prop[], char const value[])
{
    property_override(system_prop, value);
    property_override(vendor_prop, value);
}

void vendor_load_properties()
{
    const std::string bootloader = GetProperty("ro.bootloader", "");
    const std::string platform = GetProperty("ro.board.platform", "");

    char const *serial_number_file = SERIAL_NUMBER_FILE;
    std::string serial_number;

    //property_override("ro.treble.enabled", "true");
    property_override("vendor.rild.libargs", "-d /dev/ttyS0");

    if (ReadFileToString(serial_number_file, &serial_number)) {
        serial_number = Trim(serial_number);
        property_override("ro.serialno", serial_number.c_str());
    }

    if (bootloader.find("I9300") != std::string::npos) {
        property_override_dual("ro.product.model", "ro.vendor.product.model", "GT-I9300");
        property_override_dual("ro.product.device", "ro.vendor.product.device", "m0");
        property_override_dual("ro.product.device", "ro.vendor.product.name", "i9300");
        property_override_dual("ro.build.description", "ro.vendor.build.description", "m0xx-user 4.3 JSS15J I9300XXUGMJ9 release-keys");
        property_override_dual("ro.build.fingerprint", "ro.vendor.build.fingerprint", "samsung/m0xx/m0:4.3/JSS15J/I9300XXUGMJ9:user/release-keys");
        property_override("ro.build.product", "i9300");
    }
    else {
        property_override_dual("ro.product.model", "ro.vendor.product.model", "GT-I9300");
        property_override_dual("ro.product.device", "ro.vendor.product.device", "m0");
        property_override_dual("ro.product.device", "ro.vendor.product.name", "i9300");
        property_override_dual("ro.build.description", "ro.vendor.build.description", "m0xx-user 4.3 JSS15J I9300XXUGMJ9 release-keys");
        property_override_dual("ro.build.fingerprint", "ro.vendor.build.fingerprint", "samsung/m0xx/m0:4.3/JSS15J/I9300XXUGMJ9:user/release-keys");
        property_override("ro.build.product", "i9300");
    }

    const std::string device = GetProperty("ro.product.device", "");
    LOG(INFO) << "Found bootloader " << bootloader << ". " << "Setting build properties for " << device << ".\n";
}
