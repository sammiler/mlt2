/**
 * \file mlt_slices.h
 * \brief sliced threading processing helper
 * \see mlt_slices_s
 *
 * Copyright (C) 2016-2022 Meltytech, LLC
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef MLT_SLICES_H
#define MLT_SLICES_H

#include "mlt_types.h"
#include "mlt_api.h"
/**
 * \envvar \em MLT_SLICES_COUNT Set the number of slices to use, which
 * defaults to number of CPUs found.
 */

struct mlt_slices_s;

typedef int (*mlt_slices_proc)(int id, int idx, int jobs, void *cookie);

MLT_API extern int mlt_slices_count_normal();

MLT_API extern int mlt_slices_count_rr();

MLT_API extern int mlt_slices_count_fifo();

MLT_API extern void mlt_slices_run_normal(int jobs, mlt_slices_proc proc, void *cookie);

MLT_API extern void mlt_slices_run_rr(int jobs, mlt_slices_proc proc, void *cookie);

MLT_API extern void mlt_slices_run_fifo(int jobs, mlt_slices_proc proc, void *cookie);

MLT_API extern int mlt_slices_size_slice(int jobs, int index, int input_size, int *start);

#endif
