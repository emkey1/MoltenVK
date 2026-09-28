/*
 * MVKAOKGPUGate.mm -- iSH-AOK; see MVKAOKGPUGate.h.
 */

#include "MVKAOKGPUGate.h"

#import <Metal/Metal.h>

#include <atomic>
#include <condition_variable>
#include <mutex>

static std::atomic<bool> gpuAllowed{true};
static std::mutex gateMutex;
static std::condition_variable gateOpened;

void mvkAOKSetGPUAllowed(int allowed) {
	{
		std::lock_guard<std::mutex> lock(gateMutex);
		gpuAllowed.store(allowed != 0);
	}
	if (allowed) { gateOpened.notify_all(); }
}

void mvkAOKWaitGPUAllowed() {
	if (gpuAllowed.load(std::memory_order_acquire)) { return; }
	std::unique_lock<std::mutex> lock(gateMutex);
	gateOpened.wait(lock, [] { return gpuAllowed.load(); });
}

unsigned mvkAOKLinearRowAlignment(void) {
	static std::atomic<unsigned> cached{0};
	unsigned align = cached.load();
	if (align == 0) {
		@autoreleasepool {
			id<MTLDevice> device = MTLCreateSystemDefaultDevice();
			align = device ? (unsigned)[device minimumLinearTextureAlignmentForPixelFormat: MTLPixelFormatBGRA8Unorm] : 0;
			[device release];
		}
		if (align < 16) { align = 16; }
		cached.store(align);
	}
	return align;
}

static thread_local unsigned long long nextImageRowPitch = 0;

void mvkAOKSetNextImageRowPitch(unsigned long long rowPitch) {
	nextImageRowPitch = rowPitch;
}

unsigned long long mvkAOKTakeNextImageRowPitch() {
	unsigned long long rowPitch = nextImageRowPitch;
	nextImageRowPitch = 0;
	return rowPitch;
}
