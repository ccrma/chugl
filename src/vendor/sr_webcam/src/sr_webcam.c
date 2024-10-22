#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "sr_webcam_internal.h"

int sr_webcam_create(sr_webcam_device** device, int deviceId) {
	sr_webcam_device* pdev = (sr_webcam_device*)malloc(sizeof(sr_webcam_device));
	if(!pdev) {
		return -1;
	}
	memset(pdev, 0, sizeof(*pdev));
	pdev->deviceId = deviceId;
	*device		   = pdev;
	return 0;
}

void sr_webcam_set_format(sr_webcam_device* device, int width, int height, int framerate) {
	device->width	  = width;
	device->height	  = height;
	device->framerate = framerate;
}

void sr_webcam_set_callback(sr_webcam_device* device, sr_webcam_callback callback) {
	device->callback = callback;
}

void sr_webcam_set_user(sr_webcam_device* device, void* user) {
	device->user = user;
}

long sr_webcam_get_format_size(sr_webcam_device* device) {
	// Return the size in bytes, assume RGB for now.
	return (long)(device->width) * (long)(device->height) * 3;
}

void sr_webcam_get_dimensions(sr_webcam_device* device, int* width, int* height) {
	*width	= device->width;
	*height = device->height;
}

void sr_webcam_get_framerate(sr_webcam_device* device, int* fps) {
	*fps = device->framerate;
}

void* sr_webcam_get_user(sr_webcam_device* device) {
	return device->user;
}
