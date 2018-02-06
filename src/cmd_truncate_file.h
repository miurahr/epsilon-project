/*
 * $Id: cmd_truncate_file.h,v 1.4 2010/02/05 23:50:22 simakov Exp $
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

#ifndef __CMD_TRUNCATE_FILE_H__
#define __CMD_TRUNCATE_FILE_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Messages for truncate command */
#define TRUNCATE_MSG            "Truncating"
#define OPTIMIZE_MSG            "Optimizing"

static void replace_psi_to_tmp(char *file);
void truncate_file(double ratio, int halt_on_errors,
                          int quiet, char *output_dir, char *file,
                          int current, int total, char *msg);
void cmd_truncate_file(double ratio, int halt_on_errors, int quiet,
                       char *output_dir, char **files, char *msg);

#ifdef __cplusplus
}
#endif

#endif /* __CMD_TRUNCATE_FILE_H__ */
