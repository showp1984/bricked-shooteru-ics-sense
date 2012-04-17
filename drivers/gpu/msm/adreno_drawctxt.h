/* Copyright (c) 2002,2007-2011, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of Code Aurora Forum, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#ifndef __ADRENO_DRAWCTXT_H
#define __ADRENO_DRAWCTXT_H

#include "adreno_pm4types.h"
#include "a2xx_reg.h"

/* Flags */

#define CTXT_FLAGS_NOT_IN_USE		0x00000000
#define CTXT_FLAGS_IN_USE		0x00000001

/* state shadow memory allocated */
#define CTXT_FLAGS_STATE_SHADOW		0x00000010

/* gmem shadow memory allocated */
#define CTXT_FLAGS_GMEM_SHADOW		0x00000100
/* gmem must be copied to shadow */
#define CTXT_FLAGS_GMEM_SAVE		0x00000200
/* gmem can be restored from shadow */
#define CTXT_FLAGS_GMEM_RESTORE		0x00000400
/* shader must be copied to shadow */
#define CTXT_FLAGS_SHADER_SAVE		0x00002000
/* shader can be restored from shadow */
#define CTXT_FLAGS_SHADER_RESTORE	0x00004000
/* Context has caused a GPU hang */
#define CTXT_FLAGS_GPU_HANG		0x00008000

struct kgsl_device;
struct adreno_device;
struct kgsl_device_private;
struct kgsl_context;

/* draw context */
struct gmem_shadow_t {
	struct kgsl_memdesc gmemshadow;	/* Shadow buffer address */

	/* 256 KB GMEM surface = 4 bytes-per-pixel x 256 pixels/row x
	* 256 rows. */
	/* width & height must be a multiples of 32, in case tiled textures
	 * are used. */
	enum COLORFORMATX format;
	unsigned int size;	/* Size of surface used to store GMEM */
	unsigned int width;	/* Width of surface used to store GMEM */
	unsigned int height;	/* Height of surface used to store GMEM */
	unsigned int pitch;	/* Pitch of surface used to store GMEM */
	unsigned int gmem_pitch;	/* Pitch value used for GMEM */
	unsigned int *gmem_save_commands;
	unsigned int *gmem_restore_commands;
	unsigned int gmem_save[3];
	unsigned int gmem_restore[3];
	struct kgsl_memdesc quad_vertices;
	struct kgsl_memdesc quad_texcoords;
};

struct adreno_context {
	uint32_t flags;
	struct kgsl_pagetable *pagetable;
	struct kgsl_memdesc gpustate;
	unsigned int reg_save[3];
	unsigned int reg_restore[3];
	unsigned int shader_save[3];
	unsigned int shader_fixup[3];
	unsigned int shader_restore[3];
	unsigned int chicken_restore[3];
	unsigned int bin_base_offset;
	/* Information of the GMEM shadow that is created in context create */
	struct gmem_shadow_t context_gmem_shadow;
};

int adreno_drawctxt_create(struct kgsl_device *device,
			struct kgsl_pagetable *pagetable,
			struct kgsl_context *context,
			uint32_t flags);

void adreno_drawctxt_destroy(struct kgsl_device *device,
			  struct kgsl_context *context);

void adreno_drawctxt_switch(struct adreno_device *adreno_dev,
				struct adreno_context *drawctxt,
				unsigned int flags);
void adreno_drawctxt_set_bin_base_offset(struct kgsl_device *device,
				      struct kgsl_context *context,
					unsigned int offset);

/* GPU context switch helper functions */

void build_quad_vtxbuff(struct adreno_context *drawctxt,
		struct gmem_shadow_t *shadow, unsigned int **incmd);

unsigned int uint2float(unsigned int);

static inline unsigned int virt2gpu(unsigned int *cmd,
				    struct kgsl_memdesc *memdesc)
{
	return memdesc->gpuaddr + ((char *) cmd - (char *) memdesc->hostptr);
}

static inline void create_ib1(struct adreno_context *drawctxt,
			      unsigned int *cmd,
			      unsigned int *start,
			      unsigned int *end)
{
	cmd[0] = CP_HDR_INDIRECT_BUFFER_PFD;
	cmd[1] = virt2gpu(start, &drawctxt->gpustate);
	cmd[2] = end - start;
}


static inline unsigned int *reg_range(unsigned int *cmd, unsigned int start,
	unsigned int end)
{
	*cmd++ = CP_REG(start);		/* h/w regs, start addr */
	*cmd++ = end - start + 1;	/* count */
	return cmd;
}

static inline void calc_gmemsize(struct gmem_shadow_t *shadow, int gmem_size)
{
	int w = 64, h = 64;

	shadow->format = COLORX_8_8_8_8;

	/* convert from bytes to 32-bit words */
	gmem_size = (gmem_size + 3) / 4;

	while ((w * h) < gmem_size) {
		if (w < h)
			w *= 2;
		else
			h *= 2;
	}

	shadow->pitch = shadow->width = w;
	shadow->height = h;
	shadow->gmem_pitch = shadow->pitch;
	shadow->size = shadow->pitch * shadow->height * 4;
}

#endif  /* __ADRENO_DRAWCTXT_H */
