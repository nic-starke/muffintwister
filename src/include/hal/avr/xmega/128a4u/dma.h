/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/muffintwister               */
/*                   SPDX-License-Identifier: GPL-3.0-or-later                */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <avr/io.h>
#include "system/types.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

typedef struct {
  uint8_t             repeat_count; // 1 for single shot mode
  uint16_t            tx_count;
  DMA_CH_BURSTLEN_t   burst_len;
  DMA_CH_TRIGSRC_t    trig_source;
  DMA_DBUFMODE_t      dbuf_mode;
  isr_priority        int_prio;
  isr_priority        err_prio;
  uintptr_t           src_ptr;
  DMA_CH_SRCDIR_t     src_addr_mode;
  DMA_CH_SRCRELOAD_t  src_reload_mode;
  uintptr_t           dst_ptr;
  DMA_CH_DESTDIR_t    dst_addr_mode;
  DMA_CH_DESTRELOAD_t dst_reload_mode;
} dma_channel_cfg_t;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Initialise the DMA peripheral - call once during system startup.
 */
void dma_peripheral_init(void);

/**
 * @brief Initialise a specific DMA channel.
 *
 * @param ch Pointer to DMA_ch_t avr-libc struct.
 * @param cfg Channel configuration.
 * @return int 0 on success.
 *
 * @warning Disables interrupts during configuration.
 */
int dma_channel_init(DMA_CH_t* ch, dma_channel_cfg_t* cfg);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
