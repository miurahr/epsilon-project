/*
 * $Id: cmd_list_all_fb.c,v 1.8 2010/02/05 23:50:22 simakov Exp $
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <options.h>
#include <epsilon.h>
#include <cmd_list_all_fb.h>

/* List all available filterbanks */
void cmd_list_all_fb(void)
{
    char **id, **next_id;
    char **name, **next_name;
    char **type, **next_type;

    next_id = id = eps_get_fb_info(EPS_FB_ID);
    next_name = name = eps_get_fb_info(EPS_FB_NAME);
    next_type = type = eps_get_fb_info(EPS_FB_TYPE);

    printf("+----------------+------------------------------------+--------------+\n");
    printf("| %-15s| %-35s| %-13s|\n", "ID", "NAME", "TYPE");
    printf("+----------------+------------------------------------+--------------+\n");

    for (;;) {
        if (!(*next_id) || !(*next_name) || !(*next_type)) {
            break;
        }

        printf("| %-15s| %-35s| %-13s|\n", *next_id++, *next_name++, *next_type++);
    }

    printf("+----------------+------------------------------------+--------------+\n");

    eps_free_fb_info(id);
    eps_free_fb_info(name);
    eps_free_fb_info(type);
}
