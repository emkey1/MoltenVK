/*
 * MVKAOKGPUGate.mm -- iSH-AOK; see MVKAOKGPUGate.h.
 */

#include "MVKAOKGPUGate.h"
#include "MVKAOKMemTrack.h"

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

void mvkAOKMemStats(unsigned long long out[12]) {
	out[0] = mvkAOKMemLive[MVKAOKMemDescriptorPool].load();
	out[1] = mvkAOKMemCount[MVKAOKMemDescriptorPool].load();
	out[2] = mvkAOKMemLive[MVKAOKMemTexture].load();
	out[3] = mvkAOKMemCount[MVKAOKMemTexture].load();
	out[4] = (unsigned long long)mvkAOKCountLive[MVKAOKCountPipeline].load();
	out[5] = (unsigned long long)mvkAOKCountTotal[MVKAOKCountPipeline].load();
	out[6] = (unsigned long long)mvkAOKCountLive[MVKAOKCountShaderLibrary].load();
	out[7] = (unsigned long long)mvkAOKCountTotal[MVKAOKCountShaderLibrary].load();
	out[8] = mvkAOKMemLive[MVKAOKMemTempPrivate].load();
	out[9] = mvkAOKMemCount[MVKAOKMemTempPrivate].load();
	out[10] = mvkAOKMemLive[MVKAOKMemTempShared].load();
	out[11] = mvkAOKMemCount[MVKAOKMemTempShared].load();
}
