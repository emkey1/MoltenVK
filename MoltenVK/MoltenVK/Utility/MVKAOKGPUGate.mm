/*
 * MVKAOKGPUGate.mm -- iSH-AOK; see MVKAOKGPUGate.h.
 */

#include "MVKAOKGPUGate.h"

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
