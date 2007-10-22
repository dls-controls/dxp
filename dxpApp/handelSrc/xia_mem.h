/*
 * xia_mem.h
 *
 * Copyright (c) 2005, XIA LLC
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, 
 * with or without modification, are permitted provided 
 * that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above 
 *     copyright notice, this list of conditions and the 
 *     following disclaimer.
 *   * Redistributions in binary form must reproduce the 
 *     above copyright notice, this list of conditions and the 
 *     following disclaimer in the documentation and/or other 
 *     materials provided with the distribution.
 *   * Neither the name of X-ray Instrumentation Associates 
 *     nor the names of its contributors may be used to endorse 
 *     or promote products derived from this software without 
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND 
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
 * IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE 
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON 
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR 
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF 
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
 * SUCH DAMAGE.
 *
 *
 * $Id: xia_mem.h,v 1.1 2007-10-22 02:42:53 rivers Exp $
 */

#ifndef __XIA_MEM_H__
#define __XIA_MEM_H__

#include <stdlib.h>

#include "dlldefs.h"

/* Opaque pointer to the mem_point_t struct */
typedef struct _mem_point *mem_check_pt_t;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

  XIA_IMPORT void *xia_mem_malloc(size_t n, char *file, int line);
  XIA_IMPORT void xia_mem_free(void *ptr);
  XIA_IMPORT void xia_mem_stats(unsigned long *total, unsigned long *current,
								unsigned long *peak);
  XIA_IMPORT void xia_mem_checkpoint(mem_check_pt_t *p);
  XIA_IMPORT void xia_mem_checkpoint_free(mem_check_pt_t *p); 
  XIA_IMPORT void xia_mem_checkpoint_cmp(mem_check_pt_t *pt, char *out);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __XIA_MEM_H__ */