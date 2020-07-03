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
#define LOG_TAG "libgps-shim"

#include <utils/Log.h>
#include <hardware/gps.h>
#include <android/api-level.h>

#include <dlfcn.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "gps.h"
#define REAL_GPS_PATH "system/vendor/lib/hw/gps.exynos4.vendor.so"

// Speed conversion km/h to gps speed-value
#define SPEED_CONVERT 0.2728

static const GpsFilterLocation defaultFilterLocation = { 10, true, 1.0f, 5, 60000 };

static const GpsFilterLocation gpsFilterLocations[] = {
    {   5, false,  0,    0,     0 },
    {  10, true, 1.5f,   5, 60000 },
    {  20, true, 4.0f,   6, 60000 },
    {  30, true, 8.0f,   8, 60000 },
    {  40, true, 12.0f,  8, 60000 },
    {  50, true, 16.0f,  8, 60000 },
};

static gpsFilterLocationsLength = sizeof(gpsFilterLocations) / sizeof(GpsFilterLocation);

static GpsFilterLocation get_filterlocation(float accuracy) {
    for (int index = 0;index < gpsFilterLocationsLength; index++) {
        if (gpsFilterLocations[index].accuracy >= accuracy)
            return gpsFilterLocations[index];
    }

    return defaultFilterLocation;
}

static GpsLocation reportLocation;

static GpsCallbacks_vendor vendor_gpsCallbacks;

/* GPS methods */
static GpsInterface* (*vendor_get_gps_interface)(struct gps_device_t* dev);
static void* (*vendor_gps_get_extension)(const char* name);
static int (*vendor_gps_init)(GpsCallbacks* gpsCallbacks);
static int (*vendor_gps_start)();
static int (*vendor_gps_stop)();
static void (*vendor_gps_cleanup)();
static int (*vendor_gps_inject_time)(GpsUtcTime time, int64_t timeReference,
                         int uncertainty);
static int (*vendor_gps_inject_location)(double latitude, double longitude, float accuracy);
static void (*vendor_gps_delete_aiding_data)(GpsAidingData flags);
static int (*vendor_gps_set_position_mode)(GpsPositionMode mode, GpsPositionRecurrence recurrence,
            uint32_t min_interval, uint32_t preferred_accuracy, uint32_t preferred_time);


/* AGPS methods */
static void (*vendor_agps_init)(AGpsCallbacks* callbacks);
static int (*vendor_agps_data_conn_open)(const char* apn);
static int (*vendor_agps_data_conn_closed)();
static int (*vendor_agps_data_conn_failed)();
static int (*vendor_agps_set_server)(AGpsType type, const char* hostname, int port);
static int (*vendor_agps_data_conn_open_with_apn_ip_type)(
            const char* apn,
            ApnIpType apnIpType);

/* AGPS callback-methods */
static void (* vendor_agps_status_callback)(AGpsStatus* status);
static pthread_t (* vendor_agps_cb_create_thread_cb)(const char* name, void (*start)(void *), void* arg);

/* AGPS-RIL methods */
static void (*vendor_agpsril_init)( AGpsRilCallbacks* callbacks );
static void (*vendor_agpsril_set_ref_location)(const AGpsRefLocation_vendor *agps_reflocation, size_t sz_struct);
static void (*vendor_agpsril_set_set_id) (AGpsSetIDType type, const char* setid);
static void (*vendor_agpsril_ni_message) (uint8_t *msg, size_t len);
static void (*vendor_agpsril_update_network_state) (int connected, int type, int roaming, const char* extra_info);
static void (*vendor_agpsril_update_network_availability) (int avaiable, const char* apn);

/* AGPS-RIL callback-methods */
static void (*vendor_agpsril_cb_request_setid)(uint32_t flags);
static void (*vendor_agpsril_cb_request_refloc)(uint32_t flags);
static pthread_t (* vendor_agpsril_cb_create_thread_cb)(const char* name, void (*start)(void *), void* arg);

static AGpsRilCallbacks *orgAGpsRilCallbacks = NULL;
static AGpsRilCallbacks shimmed_callbacks;
static GpsCallbacks *orgGpsCallbacks = NULL;
static GpsSvStatus gpsSvStatus_info;


static void log_gpsStatus_vendor(char* func, GpsStatus* status) {
    ALOGD("%s: size=%3d status=%3d", func, status->size, status->status);
}

static void copy_sv_info_from_vendor(GpsSvStatus_vendor source, GpsSvStatus* target) {
    target->size = source.size;
    target->num_svs = source.num_svs;
    target->ephemeris_mask = source.ephemeris_mask;
    target->almanac_mask = source.almanac_mask;
    target->used_in_fix_mask = source.used_in_fix_mask;
    for (int index = 0; index < source.num_svs; index++) {
	target->sv_list[index].size = source.sv_list[index].size;
        target->sv_list[index].prn = source.sv_list[index].prn;
        target->sv_list[index].snr = source.sv_list[index].snr;
        target->sv_list[index].elevation = source.sv_list[index].elevation;
        target->sv_list[index].azimuth = source.sv_list[index].azimuth;
    }
}

static void copyGpsLocation(GpsLocation source, GpsLocation *target) {
        target->size = source.size;
        target->flags = source.flags;
        target->latitude = source.latitude;
        target->longitude = source.longitude;
        target->altitude = source.altitude;
        target->speed = source.speed;
        target->bearing = source.bearing;
        target->accuracy = source.accuracy;
        target->timestamp = source.timestamp;
}

static void shim_location_cb(GpsLocation* location) {
    ALOGD("%s: vendor-data: speed:%f km/h latitude=%f longitude=%f altitude=%f speed=%f bearing=%f accuracy=%f timestamp:%lu",
        __func__,
        location->speed / SPEED_CONVERT,
        location->latitude,
        location->longitude,
        location->altitude,
        location->speed,
        location->bearing,
        location->accuracy,
        location->timestamp);

    GpsFilterLocation filterLocation = get_filterlocation(location->accuracy);
    if (!filterLocation.enabled) {
        orgGpsCallbacks->location_cb(location);
        ALOGD("%s: FilterLocations: Disabled. Current accuracy:%1fm",
            __func__,
            location->accuracy);
        reportLocation.timestamp = 0;
        return;
    }

    if (filterLocation.enabled == 0) {
        ALOGD("%s: FilterLocations: Enabled. speed:%.1f km/h locdiff:%f timeout:%dms",
            __func__, filterLocation.speed, filterLocation.locdiff, filterLocation.timeout);
        copyGpsLocation(*location, &reportLocation);
        orgGpsCallbacks->location_cb(location);
        return;
    }

    bool updateLocation = false;
    float locDiff =
            (location->latitude - reportLocation.latitude) +
            (location->longitude - reportLocation.longitude) +
            (location->altitude - reportLocation.altitude);
    bool changedFlags = (location->flags != reportLocation.flags);
    bool hasSpeed = (location->speed > filterLocation.speed * SPEED_CONVERT);
    bool hasMovement = (locDiff > filterLocation.locdiff ||
			 locDiff < -filterLocation.locdiff);
    bool hasTimedout = (location->timestamp > reportLocation.timestamp + filterLocation.timeout);

    // Determine if we are moving
    updateLocation = (reportLocation.timestamp == 0) ||
        changedFlags || hasSpeed || hasMovement || hasTimedout;

    if (updateLocation) {
        ALOGD("%s: FilterLocations: flags:%x speed:%.1f km/h locdiff:%f acc:%.0f m - Moving because of [%s,%s,%s,%s].",
            __func__,
            location->flags,
            location->speed / SPEED_CONVERT,
            locDiff,
            location->accuracy,
            hasSpeed ? "flags" : "",
            hasSpeed ? "speed" : "",
            hasMovement ? "movement" : "",
            hasTimedout ? "timeout":"");
        // Update location because we are moving
        reportLocation.size = location->size;
        reportLocation.flags = location->flags;
        reportLocation.latitude = location->latitude;
        reportLocation.longitude = location->longitude;
        reportLocation.altitude = location->altitude;
        reportLocation.speed = location->speed;
        reportLocation.bearing = location->bearing;
        reportLocation.accuracy = location->accuracy;
        reportLocation.timestamp = location->timestamp;
    } else {
        // Not moving, use old location
        location->latitude = reportLocation.latitude;
        location->longitude = reportLocation.longitude;
        location->altitude = reportLocation.altitude;
        location->bearing = reportLocation.bearing;
        location->speed = 0;
    }
    orgGpsCallbacks->location_cb(location);
}

static void shim_status_cb(GpsStatus* status) {
    log_gpsStatus_vendor(__func__, status);
    orgGpsCallbacks->sv_status_cb(status);
}

static void shim_sv_status_cb(GpsSvStatus* sv_info) {
    GpsSvStatus_vendor* gpsSvStatus_vendor_info = (GpsSvStatus_vendor*)sv_info;

    ALOGD("%s: size=%d num_svs=%d ephemeris_mask=%x almanac_mask=%x used_in_fix_mask=%x",
       __func__,
       gpsSvStatus_vendor_info->size,
       gpsSvStatus_vendor_info->num_svs,
       gpsSvStatus_vendor_info->ephemeris_mask,
       gpsSvStatus_vendor_info->almanac_mask,
       gpsSvStatus_vendor_info->used_in_fix_mask);
    for (int index = 0; index < sv_info->num_svs; index++) {
        ALOGD("%s:   svinfo[%d] - size=%d prn=%d snr=%f elevation=%f azimuth=%f",
            __func__,
            index,
            gpsSvStatus_vendor_info->sv_list[index].size,
            gpsSvStatus_vendor_info->sv_list[index].prn,
            gpsSvStatus_vendor_info->sv_list[index].snr,
            gpsSvStatus_vendor_info->sv_list[index].elevation,
            gpsSvStatus_vendor_info->sv_list[index].azimuth);
    }

    copy_sv_info_from_vendor(*gpsSvStatus_vendor_info, &gpsSvStatus_info);
    orgGpsCallbacks->sv_status_cb(&gpsSvStatus_info);
}

static void shim_nmea_cb(GpsUtcTime timestamp, const char* nmea, int length) {
    orgGpsCallbacks->nmea_cb(timestamp, nmea, length);
}

static void shim_set_capabilities_cb(uint32_t capabilities) {
    ALOGD("%s: capabilities=%d", __func__, capabilities);
    orgGpsCallbacks->set_capabilities_cb(capabilities);
}

static void shim_acquire_wakelock_cb() {
    orgGpsCallbacks->acquire_wakelock_cb();
}

static void shim_release_wakelock_cb() {
    orgGpsCallbacks->release_wakelock_cb();
}

static pthread_t shim_create_thread_cb(const char* name, void (*start)(void *), void* arg) {
    return orgGpsCallbacks->create_thread_cb(name, start, arg);
}

static void shim_request_utc_time_cb() {
    orgGpsCallbacks->request_utc_time_cb();
}

/* AGPS-RIL shimmed methods */
static void shim_agpsril_cb_request_setid(uint32_t flags) {
    ALOGD("%s: called", __func__);
    vendor_agpsril_cb_request_setid(flags);
}

static void shim_agpsril_cb_request_refloc(uint32_t flags) {
    ALOGD("%s: called", __func__);
    vendor_agpsril_cb_request_refloc(flags);
}

static pthread_t shim_agpsril_cb_create_thread_cb(const char* name, void (*start)(void *), void* arg) {
    ALOGD("%s: called", __func__);
    return vendor_agpsril_cb_create_thread_cb(name, start, arg);
}

static void shim_agpsril_init(AGpsRilCallbacks* callbacks) {
    ALOGD("%s: called", __func__);

    orgAGpsRilCallbacks = callbacks;
    vendor_agpsril_cb_request_setid = orgAGpsRilCallbacks->request_setid;
    vendor_agpsril_cb_request_refloc = orgAGpsRilCallbacks->request_refloc;
    vendor_agpsril_cb_create_thread_cb = orgAGpsRilCallbacks->create_thread_cb;

    shimmed_callbacks.request_setid = shim_agpsril_cb_request_setid;
    shimmed_callbacks.request_refloc = shim_agpsril_cb_request_refloc;
    shimmed_callbacks.create_thread_cb = shim_agpsril_cb_create_thread_cb;

    ALOGD("%s: vendor: setid:%p refloc:%p create_thread_cb:%p",
        __func__,
        vendor_agpsril_cb_request_setid,
        vendor_agpsril_cb_request_refloc,
        vendor_agpsril_cb_create_thread_cb);

    vendor_agpsril_init(&shimmed_callbacks);
}

static void shim_agpsril_set_ref_location(AGpsRefLocation *agps_reflocation, size_t sz_struct) {
	AGpsRefLocation_vendor vendor_ref;
	if (sizeof(AGpsRefLocation_vendor) > sz_struct) {
		ALOGE("%s: AGpsRefLocation is too small, bailing out!", __func__);
		return;
	}
	ALOGD("%s: shimming AGpsRefLocation", __func__);
	ALOGD("%s: AGpsRefLocation (%d) | AGpsRefLocation_vendor(%d)", __func__, sizeof(AGpsRefLocation), sizeof(AGpsRefLocation_vendor));
	vendor_ref.type = agps_reflocation->type;
	vendor_ref.u.cellID.type = agps_reflocation->u.cellID.type;
	vendor_ref.u.cellID.mcc = agps_reflocation->u.cellID.mcc;
	vendor_ref.u.cellID.mnc = agps_reflocation->u.cellID.mnc;
	vendor_ref.u.cellID.lac = agps_reflocation->u.cellID.lac;
	vendor_ref.u.cellID.cid = agps_reflocation->u.cellID.cid;
	vendor_ref.u.mac = agps_reflocation->u.mac;
	ALOGD("%s: Executing vendor_agpsril_set_ref_vendor_reflocation= > type:%d mcc:%d mnc:%d lac:%d cid:%d",
		__func__,
		vendor_ref.u.cellID.type,
		vendor_ref.u.cellID.mcc,
		vendor_ref.u.cellID.mnc,
		vendor_ref.u.cellID.lac,
		vendor_ref.u.cellID.cid);
	vendor_agpsril_set_ref_location(&vendor_ref, sizeof(AGpsRefLocation_vendor));

	agps_reflocation->type = vendor_ref.type;
	agps_reflocation->u.cellID.type = vendor_ref.u.cellID.type;
	agps_reflocation->u.cellID.mcc = vendor_ref.u.cellID.mcc;
	agps_reflocation->u.cellID.mnc = vendor_ref.u.cellID.mnc;
	agps_reflocation->u.cellID.lac = vendor_ref.u.cellID.lac;
	agps_reflocation->u.cellID.cid = vendor_ref.u.cellID.cid;
	agps_reflocation->u.mac = vendor_ref.u.mac;
}

static void shim_agpsril_set_set_id(AGpsSetIDType type, const char* setid) {
    ALOGD("%s: type:%d setid:%s", __func__, type, setid);
    vendor_agpsril_set_set_id(type, setid);
}

static void shim_agpsril_ni_message(uint8_t *msg, size_t len) {
    ALOGD("%s: msg:%d len:%d", __func__, *msg, len);
    vendor_agpsril_ni_message(msg, len);
}

static void shim_agpsril_update_network_state(int connected, int type, int roaming, const char* extra_info) {
    ALOGD("%s: connected:%d type:%d roaming:%d extra_info:%s",
        __func__,
        connected,
        type,
        roaming,
        extra_info);

    vendor_agpsril_update_network_state(connected, type, roaming, extra_info);
}

static void shim_agpsril_update_network_availability(int avaiable, const char* apn) {
    ALOGD("%s: avaiable:%d apn:%s",
        __func__,
        avaiable,
        apn);
    vendor_agpsril_update_network_availability(avaiable, apn);
}

static AGpsInterface *aGpsInterface = NULL;

static void shim_agps_status_callback(AGpsStatus* status) {
	ALOGD("%s: type:%d status:%d ipaddr:%x", __func__,
		status->type,
		status->status,
		status->ipaddr);
	vendor_agps_status_callback(status);
}

static void shim_agps_init(AGpsCallbacks* callbacks) {
	ALOGD("%s: shimming AGpsCallbacks", __func__);
	vendor_agps_status_callback = callbacks->status_cb;
	callbacks->status_cb = shim_agps_status_callback;
}

static int shim_agps_data_conn_open(const char* apn) {
	ALOGD("%s: apn:%s", __func__, apn);
	int result = vendor_agps_data_conn_open(apn);
	ALOGD("%s: result:%d", __func__, result);
	return result;
}
static int shim_agps_data_conn_closed() {
	ALOGD("%s:", __func__);
	int result = vendor_agps_data_conn_closed();
	ALOGD("%s: result:%d", __func__, result);
	return result;
}

static int shim_agps_data_conn_failed() {
	ALOGD("%s:", __func__);
	int result = vendor_agps_data_conn_failed();
	ALOGD("%s: result:%d", __func__, result);
	return result;
}

static int shim_agps_set_server(AGpsType type, const char* hostname, int port) {
	ALOGD("%s: type:%d hostname:%s port:%d", __func__,
		type, hostname, port);
	int result = vendor_agps_set_server(type, hostname, port);
	ALOGD("%s: result:%d", __func__, result);
	return result;
}

static int shim_agps_data_conn_open_with_apn_ip_type(
            const char* apn,
            ApnIpType apnIpType) {
	ALOGD("%s: apn:%s apnIpType:%d", __func__,
		apn, apnIpType);
	int result = vendor_agps_data_conn_open_with_apn_ip_type(apn, apnIpType);
	ALOGD("%s: result:%d", __func__, result);
	return result;
}

static int shim_gps_start() {
	ALOGD("%s", __func__);
	int result = vendor_gps_start();
	ALOGD("%s: result:%d", __func__, result);
	return result;
}

static int shim_gps_stop() {
	ALOGD("%s", __func__);
	int result = vendor_gps_stop();
	ALOGD("%s: result:%d", __func__, result);
	return result;
}

static void shim_gps_cleanup() {
	ALOGD("%s", __func__);
	vendor_gps_cleanup();
}

static int shim_gps_inject_time(GpsUtcTime time, int64_t timeReference,
                         int uncertainty) {
	ALOGD("%s: time:%llu timeReference:%llu uncertainty:%d", __func__,
		time, timeReference, uncertainty);
	int result = vendor_gps_inject_time(time, timeReference, uncertainty);
	ALOGD("%s: result:%d", __func__, result);
	return result;
}

static int shim_gps_inject_location(double latitude, double longitude, float accuracy) {
	ALOGD("%s: latitude:%f longitude:%f accuracy:%f", __func__,
		latitude, longitude, accuracy);
	int result = vendor_gps_inject_location(latitude, longitude, accuracy);
	ALOGD("%s: result:%d", __func__, result);
	return result;
}

static void shim_gps_delete_aiding_data(GpsAidingData flags) {
	ALOGD("%s: flags:%x", __func__, flags);
	vendor_gps_delete_aiding_data(flags);
}

static int shim_gps_set_position_mode(GpsPositionMode mode, GpsPositionRecurrence recurrence,
            uint32_t min_interval, uint32_t preferred_accuracy, uint32_t preferred_time) {
	ALOGD("%s: mode:%d recurrence:%d min_interval:%d preferred_accuracy, preffered_time:%x",
		__func__, mode, recurrence, min_interval, preferred_accuracy, preferred_time);
	int result = vendor_gps_set_position_mode(
		mode,
		recurrence,
		min_interval,
		preferred_accuracy,
		preferred_time);
	ALOGD("%s: result:%d", __func__, result);
	return result;
}

static void* shim_gps_get_extension(const char* name) {
	ALOGD("%s(%s)", __func__, name);
	if (strcmp(name, AGPS_RIL_INTERFACE) == 0) {
		if (aGpsInterface == NULL) {
			ALOGD("%s(%s): Get extension '%s' first before handling '%s'",
				__func__,
				name,
				AGPS_INTERFACE,
				AGPS_RIL_INTERFACE
				);
			aGpsInterface = (AGpsInterface*)vendor_gps_get_extension(name);
		}

		ALOGD("%s: shimming AGPS-RIL init", __func__);

		// RIL interface
		AGpsRilInterface *aGpsRil = (AGpsRilInterface*)vendor_gps_get_extension(name);

		vendor_agpsril_init = aGpsRil->init;
		aGpsRil->init = shim_agpsril_init;

		vendor_agpsril_set_ref_location = aGpsRil->set_ref_location;
		aGpsRil->set_ref_location = shim_agpsril_set_ref_location;

		vendor_agpsril_set_set_id = aGpsRil->set_set_id;
		aGpsRil->set_set_id = shim_agpsril_set_set_id;

		vendor_agpsril_ni_message = aGpsRil->ni_message;
		aGpsRil->ni_message = shim_agpsril_ni_message;

		vendor_agpsril_update_network_state = aGpsRil->update_network_state;
		aGpsRil->update_network_state = shim_agpsril_update_network_state;

		vendor_agpsril_update_network_availability = aGpsRil->update_network_availability;
		aGpsRil->update_network_availability = shim_agpsril_update_network_availability;

		return aGpsRil;
	} else if (strcmp(name, AGPS_INTERFACE) == 0) {
		ALOGD("%s: shimming AGPS", __func__);

		// AGPS_RIL interface
		aGpsInterface = (AGpsInterface*)vendor_gps_get_extension(name);
		vendor_agps_init = aGpsInterface->init;
		aGpsInterface->init = shim_agps_init;

		vendor_agps_data_conn_open = aGpsInterface->data_conn_open;
		aGpsInterface->data_conn_open = shim_agps_data_conn_open;

		vendor_agps_data_conn_closed = aGpsInterface->data_conn_closed;
		aGpsInterface->data_conn_closed = shim_agps_data_conn_closed;

		vendor_agps_data_conn_failed = aGpsInterface->data_conn_failed;
		aGpsInterface->data_conn_failed = shim_agps_data_conn_failed;

		vendor_agps_set_server = aGpsInterface->set_server;
		aGpsInterface->set_server = shim_agps_set_server;

		vendor_agps_data_conn_open_with_apn_ip_type = aGpsInterface->data_conn_open_with_apn_ip_type;
		aGpsInterface->data_conn_open_with_apn_ip_type = shim_agps_data_conn_open_with_apn_ip_type;
		return aGpsInterface;
	}
	return vendor_gps_get_extension(name);
}

static int shim_gps_init (GpsCallbacks* gpsCallbacks) {
	ALOGD("%s: shimming GpsCallbacks", __func__);
        orgGpsCallbacks = gpsCallbacks;
	vendor_gpsCallbacks.size = sizeof(GpsCallbacks_vendor);
	vendor_gpsCallbacks.location_cb = shim_location_cb;
	vendor_gpsCallbacks.status_cb = shim_status_cb;
	vendor_gpsCallbacks.sv_status_cb = shim_sv_status_cb;
	vendor_gpsCallbacks.nmea_cb = shim_nmea_cb;
	vendor_gpsCallbacks.set_capabilities_cb = shim_set_capabilities_cb;
	vendor_gpsCallbacks.acquire_wakelock_cb = shim_acquire_wakelock_cb;
	vendor_gpsCallbacks.release_wakelock_cb = shim_release_wakelock_cb;
	vendor_gpsCallbacks.create_thread_cb = shim_create_thread_cb;
	vendor_gpsCallbacks.request_utc_time_cb = shim_request_utc_time_cb;

	return vendor_gps_init(&vendor_gpsCallbacks);
}

static GpsInterface* shim_get_gps_interface(struct gps_device_t* dev) {
	ALOGD("%s: shimming GpsInterface", __func__);
	GpsInterface *halInterface = vendor_get_gps_interface(dev);

	vendor_gps_get_extension = halInterface->get_extension;
	halInterface->get_extension = &shim_gps_get_extension;

	vendor_gps_init = halInterface->init;
	halInterface->init = &shim_gps_init;

	vendor_gps_start = halInterface->start;
	halInterface->start = &shim_gps_start;

	vendor_gps_stop = halInterface->stop;
	halInterface->stop = &shim_gps_stop;

	vendor_gps_cleanup = halInterface->cleanup;
	halInterface->cleanup = &shim_gps_cleanup;

	vendor_gps_inject_time = halInterface->inject_time;
	halInterface->inject_time = &shim_gps_inject_time;

	vendor_gps_inject_location = halInterface->inject_location;
	halInterface->inject_location = &shim_gps_inject_location;

	vendor_gps_delete_aiding_data = halInterface->delete_aiding_data;
	halInterface->delete_aiding_data = &shim_gps_delete_aiding_data;

	vendor_gps_set_position_mode = halInterface->set_position_mode;
	halInterface->set_position_mode = &shim_gps_set_position_mode;
	return halInterface;
}

static int open_gps(const struct hw_module_t* module, char const* name,
		struct hw_device_t** device) {
	void *realGpsLib;
	int gpsHalResult;
	struct hw_module_t *realHalSym;

	struct gps_device_t **gps = (struct gps_device_t **)device;

	realGpsLib = dlopen(REAL_GPS_PATH, RTLD_LOCAL);
	if (!realGpsLib) {
		ALOGE("Failed to load real GPS HAL '" REAL_GPS_PATH "': %s", dlerror());
		return -EINVAL;
	}

	realHalSym = (struct hw_module_t*)dlsym(realGpsLib, HAL_MODULE_INFO_SYM_AS_STR);
	if (!realHalSym) {
		ALOGE("Failed to locate the GPS HAL module sym: '" HAL_MODULE_INFO_SYM_AS_STR "': %s", dlerror());
		goto dl_err;
	}

	int result = realHalSym->methods->open(realHalSym, name, device);
	if (result < 0) {
		ALOGE("Real GPS open method failed: %d", result);
		goto dl_err;
	}
	ALOGD("Successfully loaded real GPS hal, shimming get_gps_interface...");
	// now, we shim hw_device_t
	vendor_get_gps_interface = (*gps)->get_gps_interface;
	(*gps)->get_gps_interface = &shim_get_gps_interface;

	return 0;
dl_err:
	dlclose(realGpsLib);
	return -EINVAL;
}

static struct hw_module_methods_t gps_module_methods = {
	.open = open_gps
};

struct hw_module_t HAL_MODULE_INFO_SYM = {
	.tag = HARDWARE_MODULE_TAG,
	.module_api_version = 1,
	.hal_api_version = 0,
	.id = GPS_HARDWARE_MODULE_ID,
	.name = "BCM451x GPS shim",
	.author = "The CyanogenMod Project",
	.methods = &gps_module_methods
};
