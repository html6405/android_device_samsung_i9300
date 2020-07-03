/*
 * Copyright (C) 2016 The CyanogenMod Project <http://www.cyanogenmod.org>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <hardware/gps.h>

/**
 * Legacy struct to represents SV information.
 * Deprecated, to be removed in the next Android release.
 * Use GnssSvInfo instead.
 */
typedef struct {
    /** set to sizeof(GpsSvInfo) */
    size_t          size;
    /** Pseudo-random number for the SV. */
    int     prn;
    /** Signal to noise ratio. */
    float   snr;
    /** Elevation of SV in degrees. */
    float   elevation;
    /** Azimuth of SV in degrees. */
    float   azimuth;
} GpsSvInfo_vendor;

typedef struct {
    /** set to sizeof(GpsSvStatus) */
    size_t size;
    int num_svs;
    GpsSvInfo_vendor sv_list[GPS_MAX_SVS];
    uint32_t ephemeris_mask;
    uint32_t almanac_mask;
    uint32_t used_in_fix_mask;
} GpsSvStatus_vendor;


typedef struct {
    /** set to sizeof(GpsCallbacks_vendor) */
    size_t size;
    gps_location_callback location_cb;
    gps_status_callback status_cb;
    gps_sv_status_callback sv_status_cb;
    gps_nmea_callback nmea_cb;
    gps_set_capabilities set_capabilities_cb;
    gps_acquire_wakelock acquire_wakelock_cb;
    gps_release_wakelock release_wakelock_cb;
    gps_create_thread create_thread_cb;
    gps_request_utc_time request_utc_time_cb;
} GpsCallbacks_vendor;

/* CellID for 2G, 3G and LTE, used in AGPS. */
typedef struct {
    AGpsRefLocationType type;
    /** Mobile Country Code. */
    uint16_t mcc;
    /** Mobile Network Code .*/
    uint16_t mnc;
    /** Location Area Code in 2G, 3G and LTE. In 3G lac is discarded. In LTE,
     * lac is populated with tac, to ensure that we don't break old clients that
     * might rely in the old (wrong) behavior.
     */
    uint16_t lac;
    /** Cell id in 2G. Utran Cell id in 3G. Cell Global Id EUTRA in LTE. */
    uint32_t cid;
} AGpsRefLocationCellID_vendor;

/** Represents ref locations */
typedef struct {
    AGpsRefLocationType type;
    union {
        AGpsRefLocationCellID_vendor	cellID;
	AGpsRefLocationMac		mac;
    } u;
} AGpsRefLocation_vendor;

typedef struct {
    float accuracy;
    bool enabled;
    double locdiff;
    float speed;
    GpsUtcTime timeout;
} GpsFilterLocation;
