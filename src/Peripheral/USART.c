/*
 * File: USART.c ( 20th November 2021 )
 * Project: Muffin
 * Copyright 2021 Nicolaus Starke
 * -----
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 */

#include <avr/io.h>
#include <avr/pgmspace.h>

#include "system/CPU.h"
#include "system/types.h"
#include "Peripheral/GPIO.h"
#include "system/Interrupt.h"
#include "Peripheral/USART.h"
#include "Utility/Utility.h"

// Missing defines for xmega usart
#define USART_UCPHA_bm (0x02) // clock phase bitmask
#define USART_DORD_bm  (0x04) // data order bitmask

static inline uint8_t GetBitMask(USART_t* pUSART) {
  if (pUSART == &USARTC0 || pUSART == &USARTD0 || pUSART == &USARTE0) {
    return PR_USART0_bm;
  } else if (pUSART == &USARTC1 || pUSART == &USARTD1) {
    return PR_USART1_bm;
  }

  return 0;
}

static inline void EnablePower(USART_t* pUSART) {
  CLR_REG(PR.PRPC, GetBitMask(pUSART));
}

static inline void DisablePower(USART_t* pUSART) {
  SET_REG(PR.PRPC, GetBitMask(pUSART));
}

static inline void SetMode(USART_t* pUSART, USART_CMODE_t Mode) {
  pUSART->CTRLC = (pUSART->CTRLC & (~USART_CMODE_gm)) | Mode;
}

static inline void SetClockPhase(USART_t* pUSART, eSPI_Mode Mode) {
  if ((Mode == SPI_MODE_1) || (Mode == SPI_MODE_3)) {
    SET_REG(pUSART->CTRLC, USART_UCPHA_bm);
  } else {
    CLR_REG(pUSART->CTRLC, USART_UCPHA_bm);
  }
}

static inline void SetDataOrder(USART_t* pUSART, eDataOrder DataOrder) {
  if (DataOrder == DO_LSB_FIRST) {
    SET_REG(pUSART->CTRLC, USART_DORD_bm);
  } else {
    CLR_REG(pUSART->CTRLC, USART_DORD_bm);
  }
}

/**
 * @brief Set the Baud Rate for the USART peripheral.
 * This can fail if F_CPU define does not match the actual clock configuration .
 * Only call this once the CPU clock has been configured.
 *
 * @param pUSART
 * @param BaudRate
 * @param CPUFrequency
 */
static inline void SetBaudRate(USART_t* pUSART, uint32_t BaudRate,
                               uint32_t CPUFrequency) {
  uint16_t baudSelection = 0; // set to maximum possible to start

  // If user requires lower than maximum, calculate the correct baudSelection
  if (BaudRate < (CPUFrequency / 2)) {
    baudSelection = (CPUFrequency / (BaudRate * 2)) - 1;
  }

  // USART in Master SPI mode does NOT support double speed (other USART modes
  // do)
  pUSART->BAUDCTRLB = (uint8_t)((~USART_BSCALE_gm) & (baudSelection >> 0x08));
  pUSART->BAUDCTRLA = (uint8_t)(baudSelection);
}

/**
 * @brief Configures SCK, TX and RX for the corrects pins based on the port
 * number.
 *
 * @param pUSART A pointer to the USART module.
 */
static inline void ConfigureGPIO(USART_t* pUSART, eSPI_Mode SPIMode) {
  // Default pins are  SCK = 1, RX = 2, TX = 3
  // Remapped pins are SCK = 5, RX = 6, TX = 7

  bool    remap = false;
  PORT_t* pPort = NULL;

  if (pUSART == &USARTC0) {
    pPort = &PORTC;
    remap = (PORTC.REMAP & PORT_USART0_bm);
  } else if (pUSART == &USARTC1) {
    pPort = &PORTC;
    remap = true;
  } else if (pUSART == &USARTD0) {
    pPort = &PORTD;
    remap = (PORTD.REMAP & PORT_USART0_bm);
  } else if (pUSART == &USARTD1) {
    pPort = &PORTD;
    remap = true;
  } else if (pUSART == &USARTE0) {
    pPort = &PORTE;
    remap = (PORTE.REMAP & PORT_USART0_bm);
  }

  const uint8_t sckPin    = (remap ? 5 : 1);
  const uint8_t rxPin     = (remap ? 6 : 2);
  const uint8_t txPin     = (remap ? 7 : 3);
  const bool    invertSCK = (SPIMode == SPI_MODE_2 || SPIMode == SPI_MODE_3);

  GPIO_SetPinMode(pPort, sckPin,
                  PORT_OPC_TOTEM_gc | (invertSCK ? (0x01 << 6) : 0));

  GPIO_SetPinDirection(pPort, sckPin, GPIO_OUTPUT);
  GPIO_SetPinDirection(pPort, rxPin, GPIO_INPUT);
  GPIO_SetPinDirection(pPort, txPin, GPIO_OUTPUT);
  GPIO_SetPinLevel(pPort, sckPin, HIGH);
}

// ----------------- //

void USART_Init(void) {
  // uint8_t flags = IRQ_DisableInterrupts();

  // No global init required.

  // IRQ_EnableInterrupts(flags);
}

/**
 * @brief Initialise a USART peripheral (such as USARTC0) in Master SPI Mode
 * IMPORTANT!! Only call this after the CPU clock is configured to run at F_CPU.
 * Does not start UART tx/rx.
 * GPIO will be configured based on the port and PORT.REMAP
 *
 * Default pinout is:
 * Pin 1 - SCK
 * Pin 2 - RX
 * Pin 3 - TX
 *
 * Remapped pinout is:
 * Pin 5 - SCK
 * Pin 6 - RX
 * Pin 7 - TX
 *
 * @param pConfig A pointer to a USART config.
 */
void USART_InitModule(const sUSART_ModuleConfig* pConfig) {
  const uint8_t flags = IRQ_DisableInterrupts();

  EnablePower(pConfig->pUSART);
  USART_DisableRX(pConfig->pUSART);
  ConfigureGPIO(pConfig->pUSART, pConfig->SPIMode);
  SetMode(pConfig->pUSART, USART_CMODE_MSPI_gc);
  SetClockPhase(pConfig->pUSART, pConfig->SPIMode);
  SetDataOrder(pConfig->pUSART, pConfig->DataOrder);
  SetBaudRate(pConfig->pUSART, pConfig->BaudRate, CPU_GetMainClockSpeed());
  USART_EnableTX(pConfig->pUSART);
  IRQ_EnableInterrupts(flags);
}
