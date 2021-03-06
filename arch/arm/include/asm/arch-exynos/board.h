/*
 * Board header file for Exynos boards
 *
 * Copyright (c) 2012 The Chromium OS Authors.
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __EXYNOS5_BOARD_H
#define __EXYNOS5_BOARD_H

/**
 * Get the board revision information, including sub-revision
 *
 * @param board_rev	Returns board revision #, or -1 if not known; NULL OK.
 * @param subrev	Returns subrev #, or 0 if not known; NULL OK.
 */
void board_get_full_revision(int *board_rev, int *subrev);

/**
 * Get installed memory time at run time.
 *
 * Currently returns either 2GB or 3.5GB based on board subrevision settings.
 */
phys_size_t board_get_memory_size(void);

/**
 * Get the board revision information
 *
 * @return board revision, or -1 if not known
 */
int board_get_revision(void);

/**
 * Get whether the processor has reset since running u-boot last. In other
 * words, return whether we're in first stage or second stage.
 *
 * @return 1 if we've reset, 0 if not
 */
int board_is_processor_reset(void);

/* Enable the audio code ready for use */
int board_enable_audio_codec(void);

/* Set up available PMICs */
int board_init_max77686(void);
int board_init_max77802(void);
int board_init_s2mps11(void);
int board_init_tps65090(void);

/* Set up max frequency support for CPU*/
void set_max_cpu_freq(void);

/*
 * Exynos baord specific changes for
 * board_init
 */
int exynos_init(void);
#endif
