/**
 * \file mlt_version.h
 * \brief contains version information
 *
 * Copyright (C) 2010-2025 Meltytech, LLC
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

#ifndef MLT_VERSION_H
#define MLT_VERSION_H

// Add quotes around any #define variables
#define MLT_STRINGIZE2(s) #s
#define MLT_STRINGIZE(s) MLT_STRINGIZE2(s)

#define LIBMLT_VERSION_MAJOR 7
#define LIBMLT_VERSION_MINOR 33
#define LIBMLT_VERSION_REVISION 0
#define LIBMLT_VERSION_INT \
    ((LIBMLT_VERSION_MAJOR << 16) + (LIBMLT_VERSION_MINOR << 8) + LIBMLT_VERSION_REVISION)
#define LIBMLT_VERSION \
    MLT_STRINGIZE(LIBMLT_VERSION_MAJOR.LIBMLT_VERSION_MINOR.LIBMLT_VERSION_REVISION)
#include "mlt_api.h"
MLT_API extern int mlt_version_get_int();
MLT_API extern int mlt_version_get_major();
MLT_API extern int mlt_version_get_minor();
MLT_API extern int mlt_version_get_revision();
MLT_API extern char *mlt_version_get_string();

#endif
