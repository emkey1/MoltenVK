/*
 * MVKAOKGPUGate.h -- iSH-AOK
 *
 * iOS refuses GPU work from an app in the background: a command buffer
 * committed there fails with MTLCommandBufferErrorNotPermitted. iSH-AOK's
 * renderer submits whenever its guest programs do, background or not, so the
 * app closes this gate as it enters the background and opens it again as it
 * returns; until then every commit waits here.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/** Whether command buffers may be committed. Any thread. */
void mvkAOKSetGPUAllowed(int allowed);

/**
 * The byte alignment of a row of a linear 32-bit image on this GPU
 * (-[MTLDevice minimumLinearTextureAlignmentForPixelFormat:], which MoltenVK
 * pads VK_IMAGE_TILING_LINEAR rows to): 16 on M-series GPUs, 64 on an A10X.
 * Buffers shared with the host as linear images must use it.
 */
unsigned mvkAOKLinearRowAlignment(void);

/**
 * The row pitch the next VkImage this thread creates must use, 0 for its own.
 * A buffer another process drew into (a dma-buf, emulated on this host) has
 * the rows it was given, which may be wider than MoltenVK's own padding. Taken
 * only by a single-plane, single-level linear image, and only a pitch Metal can
 * use: at least the natural one, and a multiple of the linear row alignment.
 * The renderer sets it around one vkCreateImage and reads back the result.
 */
void mvkAOKSetNextImageRowPitch(unsigned long long rowPitch);

#ifdef __cplusplus
}

/** The pitch mvkAOKSetNextImageRowPitch set for this thread, cleared. */
unsigned long long mvkAOKTakeNextImageRowPitch();

/** Waits while the gate is closed. Call just before committing. */
void mvkAOKWaitGPUAllowed();
#endif
