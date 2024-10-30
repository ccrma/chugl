#include "sr_webcam_internal.h"

#if defined(__OBJC__)

#	import <Foundation/Foundation.h>
#	import <AVFoundation/AVFoundation.h>
#	import <Accelerate/Accelerate.h>
#	import <CoreMedia/CoreMedia.h>
#	import <CoreVideo/CoreVideo.h>

@interface SRWebcamVideoStream : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate> {

	AVCaptureSession* _captureSession;
	AVCaptureDevice* _captureDevice;
	AVCaptureVideoDataOutput* _captureDataOut;
	AVCaptureDeviceInput* _captureDataIn;

@public
	sr_webcam_device* _parent;

@public
	int _width;
@public
	int _height;
@public
	int _id;
@public
	int _framerate;
}

- (BOOL)setupWithID:(int)deviceID rate:(int)framerate width:(int)w height:(int)h device_name:(char*)device_name device_name_length:(int)device_name_length;

- (void)start;
- (void)stop;

@end

@implementation SRWebcamVideoStream

- (id)init {
	self = [super init];
	if(self) {
		_captureDataIn	= nil;
		_captureDataOut = nil;
		_captureDevice	= nil;

		_parent	   = NULL;
		_width	   = 0;
		_height	   = 0;
		_id		   = 0;
		_framerate = 0;
	}
	return self;
}

- (BOOL)setupWithID:(int)deviceID rate:(int)framerate width:(int)w height:(int)h device_name:(char*)device_name device_name_length:(int)device_name_length {
	// List available devices
	NSArray* devices;
	NSArray* deviceTypes;
	// if 10.15 or later
	// if(@available(macOS 10.15, *)) {
		// NSArray* deviceTypes						   = @[AVCaptureDeviceTypeBuiltInWideAngleCamera, AVCaptureDeviceTypeExternal];

		// array of AVCaptureDeviceType builtin AND AVCaptureDeviceTypeExternal
	
	// if less than version 14.0, use AVCaptureDeviceTypeExternalUnknown
	// if (@available(macOS 14.0, *)) {
	// 	deviceTypes = @[AVCaptureDeviceTypeBuiltInWideAngleCamera, AVCaptureDeviceTypeExternal];
	// } else {
		deviceTypes = @[AVCaptureDeviceTypeBuiltInWideAngleCamera, AVCaptureDeviceTypeExternalUnknown];
	// }
	AVCaptureDeviceDiscoverySession* discovSession = [AVCaptureDeviceDiscoverySession discoverySessionWithDeviceTypes:deviceTypes mediaType:AVMediaTypeVideo position:AVCaptureDevicePositionUnspecified];
	devices										   = [discovSession devices];
	// } else {
	// 	devices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
	// }

	if([devices count] == 0 || deviceID < 0) {
		return NO;
	}

	// sort devices by uniqueID
	devices = [devices sortedArrayUsingDescriptors:@[[NSSortDescriptor sortDescriptorWithKey:@"uniqueID" ascending:YES]]];
	_id			   = MIN(deviceID, (int)([devices count]) - 1);
	_captureDevice = [devices objectAtIndex:_id];

	// store the localized name as device name
	const char* multibyte_name = [_captureDevice.localizedName UTF8String];
	strncpy(device_name, multibyte_name, device_name_length);

	// for each device in the devices array
	// for(AVCaptureDevice* device in devices) {
	// 	// log the device uniqueID
	// 	NSLog(@"Device uniqueID: %@", [device uniqueID]);
	// 	// log the device modelID
	// 	NSLog(@"Device modelID: %@", [device modelID]);
	// 	// log the device localizedName
	// 	NSLog(@"Device localizedName: %@", [device localizedName]);
	// 	// log the device manufacturer
	// 	NSLog(@"Device manufacturer: %@", [device manufacturer]);
	// }

	// Setup the device.
	NSError* err = nil;
	[_captureDevice lockForConfiguration:&err];
	if(err) {
		return NO;
	}
	// Look for the best format.
	AVCaptureDeviceFormat* bestFormat = nil;
	int wBest						  = 0;
	int hBest						  = 0;
	float bestFit					  = 1e9f;
	for(AVCaptureDeviceFormat* format in [_captureDevice formats]) {
		CMVideoDimensions dimensions = CMVideoFormatDescriptionGetDimensions(format.formatDescription);
		const int wFormat			 = dimensions.width;
		const int hFormat			 = dimensions.height;
		// Found the perfect mode.
		if(wFormat == w && hFormat == h) {
			wBest	   = wFormat;
			hBest	   = hFormat;
			bestFormat = format;
			break;
		}
		const float dw	= (float)(w - wFormat);
		const float dh	= (float)(h - hFormat);
		const float fit = sqrtf(dw * dw + dh * dh);
		if(fit < bestFit) {
			bestFit	   = fit;
			wBest	   = wFormat;
			hBest	   = hFormat;
			bestFormat = format;
		}
	}

	// If we found a valid format, set it.
	if(bestFormat) {
		[_captureDevice setActiveFormat:bestFormat];
		_width	= wBest;
		_height = hBest;
	}
	// Now try to adjust the framerate.
	if(framerate > 0) {
		NSArray* validRates			= _captureDevice.activeFormat.videoSupportedFrameRateRanges;
		AVFrameRateRange* bestRange = [AVFrameRateRange alloc];
		BOOL found					= NO;
		for(AVFrameRateRange* range in validRates) {
			if(framerate >= floor([range minFrameRate]) && framerate <= ceil([range maxFrameRate])) {
				bestRange = range;
				found	  = YES;
			}
		}
		if(found) {
			_captureDevice.activeVideoMinFrameDuration = bestRange.minFrameDuration;
			_captureDevice.activeVideoMaxFrameDuration = bestRange.maxFrameDuration;
		}
	}

	// Configuration done.
	[_captureDevice unlockForConfiguration];

	// Then, the device is ready.
	_captureDataIn								  = [AVCaptureDeviceInput deviceInputWithDevice:_captureDevice error:nil];
	_captureDataOut								  = [[AVCaptureVideoDataOutput alloc] init];
	_captureDataOut.alwaysDiscardsLateVideoFrames = YES;

	// We receive data on a secondary thread.
	dispatch_queue_t queue;
	queue = dispatch_queue_create("VideoStream", NULL);
	dispatch_set_target_queue(queue, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0));
	[_captureDataOut setSampleBufferDelegate:self queue:queue];
	// Not sure, OF does this but not the Apple sample.
	dispatch_release(queue);

	// Create video settings for the output, corresponding to the current format.
	NSDictionary* settings = [NSDictionary dictionaryWithObjectsAndKeys:
											   [NSNumber numberWithUnsignedInt:kCVPixelFormatType_32BGRA], kCVPixelBufferPixelFormatTypeKey,
										   [NSNumber numberWithInt:_width], kCVPixelBufferWidthKey,
										   [NSNumber numberWithInt:_height], kCVPixelBufferHeightKey,
										   nil];
	[_captureDataOut setVideoSettings:settings];

	_captureSession = [[AVCaptureSession alloc] init];
	[_captureSession beginConfiguration];
	[_captureSession addInput:_captureDataIn];
	[_captureSession addOutput:_captureDataOut];
	// Setup output (once added), limit acquisition frequency.
	AVCaptureConnection* connection = [_captureDataOut connectionWithMediaType:AVMediaTypeVideo];
	if([connection isVideoMinFrameDurationSupported] == YES) {
		[connection setVideoMinFrameDuration:CMTimeMake(1, framerate)];
	}
	if([connection isVideoMaxFrameDurationSupported] == YES) {
		[connection setVideoMaxFrameDuration:CMTimeMake(1, framerate)];
	}

	// Update framerate.
	int framerateConnection = (int)((float)(connection.videoMinFrameDuration.timescale) / (float)(connection.videoMinFrameDuration.value));
	int framerateDevice		= (int)((float)(_captureDevice.activeVideoMinFrameDuration.timescale) / (float)(_captureDevice.activeVideoMinFrameDuration.value));
	_framerate				= MIN(framerateDevice, framerateConnection);

	[_captureSession commitConfiguration];
	return YES;
}

- (void)start {
	[_captureSession startRunning];
	[_captureDataIn.device lockForConfiguration:nil];

	if([_captureDataIn.device isFocusModeSupported:AVCaptureFocusModeAutoFocus]) {
		[_captureDataIn.device setFocusMode:AVCaptureFocusModeAutoFocus];
	}
}

- (void)stop {
	if(_captureSession) {
		if(_captureDataOut) {
			// Remove the delegate.
			if(_captureDataOut.sampleBufferDelegate != nil) {
				[_captureDataOut setSampleBufferDelegate:nil queue:NULL];
			}
		}
		// Remove the input and output.
		for(AVCaptureInput* input in _captureSession.inputs) {
			[_captureSession removeInput:input];
		}
		for(AVCaptureOutput* output in _captureSession.outputs) {
			[_captureSession removeOutput:output];
		}
		[_captureSession stopRunning];
	}
}

/// Implement the delegate.

- (void)captureOutput:(AVCaptureOutput*)captureOutput didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer fromConnection:(AVCaptureConnection*)connection {
#	pragma unused(captureOutput)
#	pragma unused(connection)
	if(!_parent) {
		return;
	}
	@autoreleasepool {
		CVImageBufferRef imgBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
		CVPixelBufferLockBaseAddress(imgBuffer, 0);

		const int wBuffer = (int)CVPixelBufferGetWidth(imgBuffer);
		const int hBuffer = (int)CVPixelBufferGetHeight(imgBuffer);
		if(wBuffer == _parent->width && hBuffer == _parent->height) {
			unsigned char* baseBuffer = (unsigned char*)CVPixelBufferGetBaseAddress(imgBuffer);
			// Convert to RGB.
			vImage_Buffer srcImg;
			srcImg.width	= wBuffer;
			srcImg.height	= hBuffer;
			srcImg.data		= baseBuffer;
			srcImg.rowBytes = CVPixelBufferGetBytesPerRow(imgBuffer);

			unsigned char* dstBuffer = (unsigned char*)malloc(wBuffer * hBuffer * 3);
			vImage_Buffer dstImg;
			dstImg.width	= wBuffer;
			dstImg.height	= hBuffer;
			dstImg.rowBytes = wBuffer * 3;
			dstImg.data		= dstBuffer;

			vImage_Error err = vImageConvert_BGRA8888toRGB888(&srcImg, &dstImg, kvImageNoFlags);
			if(err == kvImageNoError) {
				// Pass the data.
				_parent->callback(_parent, dstBuffer);
			}
			free(dstBuffer);
		}
		CVPixelBufferUnlockBaseAddress(imgBuffer, kCVPixelBufferLock_ReadOnly);
	}
}

- (void)dealloc {
	if(_captureSession) {
		[self stop];
		[_captureSession release];
		_captureSession = nil;
	}
	if(_captureDataOut) {
		if(_captureDataOut.sampleBufferDelegate != nil) {
			[_captureDataOut setSampleBufferDelegate:nil queue:NULL];
		}
		[_captureDataOut release];
		_captureDataOut = nil;
	}
	if(_parent) {
		_parent = NULL;
	}
	if(_captureDataIn) {
		[_captureDataIn release];
		_captureDataIn = nil;
	}

	if(_captureDevice) {
		[_captureDevice release];
		_captureDevice = nil;
	}
	[super dealloc];
}

@end

int sr_webcam_open(sr_webcam_device* device) {
	// Already setup.
	if(device->stream) {
		return -1;
	}
	SRWebcamVideoStream* stream = [[SRWebcamVideoStream alloc] init];
	stream->_parent				= device;
	BOOL res					= [stream setupWithID:device->deviceId rate:device->framerate width:device->width height:device->height device_name:device->user_friendly_name device_name_length:sizeof(device->user_friendly_name)];
	if(res == NO) {
		device->stream = NULL;
		return -1;
	}
	device->stream	  = stream;
	device->width	  = stream->_width;
	device->height	  = stream->_height;
	device->deviceId  = stream->_id;
	device->framerate = stream->_framerate;
	return 0;
}

void sr_webcam_start(sr_webcam_device* device) {
	if(device->stream && device->running == 0) {
		SRWebcamVideoStream* stream = (SRWebcamVideoStream*)(device->stream);
		[stream start];
		device->running = 1;
	}
}

void sr_webcam_stop(sr_webcam_device* device) {
	if(device->stream && device->running == 1) {
		SRWebcamVideoStream* stream = (SRWebcamVideoStream*)(device->stream);
		[stream stop];
		device->running = 0;
	}
}

void sr_webcam_delete(sr_webcam_device* device) {
	if(device->running == 1) {
		sr_webcam_stop(device);
	}
	if(device->stream) {
		SRWebcamVideoStream* stream = (SRWebcamVideoStream*)(device->stream);
		[stream release];
	}
	free(device);
}

#endif
