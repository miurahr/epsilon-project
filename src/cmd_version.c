/*
 * $Id: cmd_version.c,v 1.12 2010/04/05 05:01:04 simakov Exp $
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

/* Some platforms have no autotools support. On such platforms
 * config.h header mentioned above is not available and VERSION
 * is not defined. To work this around, EPSILON's VERSION is
 * duplicated in separate header. */
#ifndef VERSION
# include <epsilon_version.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <options.h>
#include <epsilon.h>
#include <cmd_version.h>

/* Print program version */
void cmd_version(void)
{
    printf("This is EPSILON version %s\n\n", VERSION);

    printf("Copyright (C) 2006,2007,2010 Alexander Simakov, <xander@entropyware.info>\n");
    printf("Home page: http://epsilon-project.sourceforge.net\n\n");

    printf("EPSILON is free software: you can redistribute it and/or modify\n");
    printf("it under the terms of the GNU Lesser General Public License as published by\n");
    printf("the Free Software Foundation, either version 3 of the License, or\n");
    printf("(at your option) any later version.\n");
}
