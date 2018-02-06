/*
 * $Id: mem_alloc.h,v 1.16 2010/02/05 23:50:22 simakov Exp $
 *
 * EPSILON - wavelet image compression library.
 * Copyright (C) 2006,2007,2010 Alexander Simakov, <xander@entropyware.info>
 *
 * This file is part of EPSILON
 *
 * EPSILON is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * EPSILON is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with EPSILON.  If not, see <http://www.gnu.org/licenses/>.
 *
 * http://epsilon-project.sourceforge.net
 */

/** \file
 *
 *  \brief Two-dimensional memory management
 *
 *  This file contains two-dimensional memory management routines. */

#ifndef __MEM_ALLOC_H__
#define __MEM_ALLOC_H__

#ifdef __cplusplus
extern "C" {
#endif

/** \addtogroup mem_alloc Two-dimensional memory management */
/*@{*/

#include <common.h>

/** Memory allocation
 *
 *  This function allocates one-dimensional array of desired size.
 *
 *  \param size Size in bytes
 *
 *  \return Array pointer
 *
 *  \warning This function halts program if all virtual memory
 *  is exhausted. */
void *xmalloc(size_t size);

/** Two-dimensional memory allocation
 *
 *  This function allocates two-dimensional array of desired size.
 *
 *  \param width Array width
 *  \param height Array height
 *  \param size Element size
 *
 *  \return Array pointer
 *
 *  \warning This function halts program if all virtual memory
 *  is exhausted. */
void **malloc_2D(int width, int height, int size);

/** Two-dimensional memory releasing
 *
 *  This function releases two-dimensional array allocated by #malloc_2D.
 *
 *  \param ptr Array pointer
 *  \param width Array width
 *  \param height Array height
 *
 *  \return \c VOID */
void free_2D(void **ptr, int width, int height);

/*@}*/

#ifdef __cplusplus
}
#endif

#endif /* __MEM_ALLOC_H__ */
