/*
 * $Id: options.h,v 1.26 2010/02/05 23:50:22 simakov Exp $
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

#ifndef __OPTIONS_H__
#define __OPTIONS_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Handy flags */
#define OPT_YES                 1
#define OPT_NO                  0
#define OPT_NA                  0

/* Command */
#define OPT_CMD_ENCODE_FILE     1
#define OPT_CMD_DECODE_FILE     2
#define OPT_CMD_LIST_ALL_FB     3
#define OPT_CMD_VERSION         4
#define OPT_CMD_TRUNCATE_FILE   5
#define OPT_CMD_START_NODE      6
#define OPT_CMD_STOP_NODE       7

/* Splitting mode */
#define OPT_MODE_NORMAL         1
#define OPT_MODE_OTLPF          2

/* Default block size */
#define OPT_DEF_BLOCK           256

/* Default compression ratio */
#define OPT_DEF_RATIO           10.0

/* Default filterbank */
#define OPT_DEF_FB              "daub97lift"
#define OPT_DEF_FB_TYPE         BIORTHOGONAL

#ifdef __cplusplus
}
#endif

#endif /* __OPTIONS_H__ */
