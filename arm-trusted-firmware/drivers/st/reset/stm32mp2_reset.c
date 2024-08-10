/*
 * Copyright (c) 2024, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <stdbool.h>

#include <common/debug.h>
#include <drivers/delay_timer.h>
#include <drivers/st/stm32mp_reset.h>
#include <lib/mmio.h>
#include <lib/utils_def.h>

#include <platform_def.h>

static uint32_t id2reg_offset(unsigned int reset_id)
{
	return ((reset_id & GENMASK(31, 5)) >> 5) * sizeof(uint32_t);
}

static uint8_t id2reg_bit_pos(unsigned int reset_id)
{
	return (uint8_t)(reset_id & GENMASK(4, 0));
}

static int reset_toggle(uint32_t id, unsigned int to_us, bool reset_status)
{
	uint32_t offset = id2reg_offset(id);
	uint32_t bitmsk = BIT(id2reg_bit_pos(id));
	uint32_t bit_check;
	uintptr_t rcc_base = stm32mp_rcc_base();

	if (reset_status) {
		mmio_setbits_32(rcc_base + offset, bitmsk);
		bit_check = bitmsk;
	} else {
		mmio_clrbits_32(rcc_base + offset, bitmsk);
		bit_check = 0U;
	}

	if (to_us != 0U) {
		uint64_t timeout_ref = timeout_init_us(to_us);

		while ((mmio_read_32(rcc_base + offset) & bitmsk) != bit_check) {
			if (timeout_elapsed(timeout_ref)) {
				return -ETIMEDOUT;
			}
		}
	}

	return 0;
}

int stm32mp_reset_assert(uint32_t id, unsigned int to_us)
{
	return reset_toggle(id, to_us, true);
}

int stm32mp_reset_deassert(uint32_t id, unsigned int to_us)
{
	return reset_toggle(id, to_us, false);
}

void __dead2 stm32mp_system_reset(void)
{
	uintptr_t rcc_base = stm32mp_rcc_base();

	mmio_setbits_32(rcc_base + RCC_GRSTCSETR, RCC_GRSTCSETR_SYSRST);

	/* Loop in case system reset is not immediately caught */
	while (true) {
		wfi();
	}
}
