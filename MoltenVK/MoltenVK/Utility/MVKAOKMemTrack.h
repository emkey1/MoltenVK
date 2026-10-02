/*
 * MVKAOKMemTrack.h
 *
 * iSH-AOK: what MoltenVK allocates on its own, outside the VkDeviceMemory a
 * client asked for. On iOS it is the app's footprint, the ledger jetsam kills
 * on. Tux Racer's race took the iSH-AOK app from 1 GB to 3.9 GB and then past
 * its 6 GB limit while the guest's Vulkan device memory stayed under 512 MB,
 * so the rest was being allocated in here.
 *
 * Each allocation of 16 MB or more is logged to stderr, and each category's
 * live total every time it climbs past another 256 MB. Header-only so that no
 * build file has to list it: the inline function's statics are one instance
 * across every translation unit that uses it.
 */

#pragma once

#include <atomic>
#include <cstdint>
#include <cstdio>

enum MVKAOKMemKind { MVKAOKMemDescriptorPool, MVKAOKMemTexture, MVKAOKMemTempPrivate, MVKAOKMemTempShared, MVKAOKMemKindCount };

// Inline variables, one instance program-wide, so mvkAOKMemStats (MVKAOKGPUGate)
// can hand the totals to /proc/ish/host_vm -- stderr does not reliably reach
// anywhere on a device.
inline std::atomic<uint64_t> mvkAOKMemLive[MVKAOKMemKindCount];
inline std::atomic<uint32_t> mvkAOKMemCount[MVKAOKMemKindCount];

inline void mvkAOKMemTrack(MVKAOKMemKind kind, uint64_t bytes, bool add) {
	static const char* const names[MVKAOKMemKindCount] = { "descriptor pool buffers", "image textures", "temp buffers (private)", "temp buffers (shared)" };
	auto& live = mvkAOKMemLive;
	auto& count = mvkAOKMemCount;
	static std::atomic<uint64_t> mark[MVKAOKMemKindCount];
	if (bytes == 0) { return; }
	uint64_t now;
	if (add) {
		now = live[kind].fetch_add(bytes) + bytes;
		count[kind]++;
	} else {
		now = live[kind].fetch_sub(bytes) - bytes;
		count[kind]--;
	}
	if (bytes >= (16ull << 20)) {
		fprintf(stderr, "[mvk-aok] %s %llu MB %s; live %u, %llu MB\n", names[kind],
				(unsigned long long)(bytes >> 20), add ? "allocated" : "freed",
				count[kind].load(), (unsigned long long)(now >> 20));
	}
	uint64_t m = now >> 28;
	uint64_t reported = mark[kind].load();
	if (m > reported) {
		if (mark[kind].compare_exchange_strong(reported, m))
			fprintf(stderr, "[mvk-aok] %s live: %u, %llu MB\n", names[kind],
					count[kind].load(), (unsigned long long)(now >> 20));
	} else if (m + 1 < reported) {
		mark[kind].compare_exchange_strong(reported, m);
	}
}

// iSH-AOK: objects whose memory Metal keeps where no region of the app shows
// it -- the GPU side of a pipeline state, a compiled library. In Tux Racer's
// race the app's footprint ran ~2.6 GB past everything its own address space
// held, so the count of these is logged at every 250 more live.
enum MVKAOKCountKind { MVKAOKCountPipeline, MVKAOKCountShaderLibrary, MVKAOKCountKindCount };

inline std::atomic<int64_t> mvkAOKCountLive[MVKAOKCountKindCount];
inline std::atomic<int64_t> mvkAOKCountTotal[MVKAOKCountKindCount];

inline void mvkAOKCountTrack(MVKAOKCountKind kind, bool add) {
	static const char* const names[MVKAOKCountKindCount] = { "pipelines", "shader libraries" };
	auto& live = mvkAOKCountLive;
	auto& total = mvkAOKCountTotal;
	static std::atomic<int64_t> mark[MVKAOKCountKindCount];
	int64_t now = add ? ++live[kind] : --live[kind];
	if (add) { total[kind]++; }
	int64_t m = now / 250;
	int64_t reported = mark[kind].load();
	if (m > reported) {
		if (mark[kind].compare_exchange_strong(reported, m))
			fprintf(stderr, "[mvk-aok] %s live: %lld (created %lld)\n", names[kind],
					(long long)now, (long long)total[kind].load());
	} else if (m + 1 < reported) {
		mark[kind].compare_exchange_strong(reported, m);
	}
}
