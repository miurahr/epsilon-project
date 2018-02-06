/*
 * $Id: mem_alloc.c,v 1.17 2010/02/05 23:50:22 simakov Exp $
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

#include <common.h>
#include <mem_alloc.h>

void *xmalloc(size_t size)
{
    void *ptr;

    ptr = malloc(size);
    assert(ptr);

    return ptr;
}

void **malloc_2D(int width, int height, int size)
{
    void **ptr;
    int i;

    assert((width > 0) && (height > 0) && (size > 0));
    ptr = (void **) xmalloc(height * sizeof(void *));

    for (i = 0; i < height; i++) {
        ptr[i] = (void *) xmalloc(width * size);
    }

    return ptr;
}

void free_2D(void **ptr, int width, int height)
{
    int i;

    assert((width > 0) && (height > 0));

    for (i = 0; i < height; i++) {
        free(ptr[i]);
    }

    free(ptr);
}
