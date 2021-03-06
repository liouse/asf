/**
 * \file
 *
 * \brief Unit tests for flash efc driver.
 *
 * Copyright (c) 2011 - 2014 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */

#include <board.h>
#include <sysclk.h>
#include <flash_efc.h>
#include <string.h>
#include <unit_test/suite.h>
#include <stdio_serial.h>
#include <conf_test.h>
#include <conf_board.h>

/**
 * \mainpage
 *
 * \section intro Introduction
 * This is the unit test application for the flash efc driver.
 * It consists of test cases for the following functionalities:
 * - Get flash device id
 * - Flash configure
 * - Get flash region information
 * - Flash write
 * - Flash lock
 * - Flash gpnvm utilities
 * - For sam3sd8, an additional erase test will be performed.
 *
 * \section files Main Files
 * - \ref unit_tests.c
 * - \ref conf_test.h
 * - \ref conf_board.h
 * - \ref conf_clock.h
 * - \ref conf_usart_serial.h
 *
 * \section device_info Device Info
 * All SAM devices can be used.
 * This example has been tested with the following setup:
 * - sam3n4c_sam3n_ek
 * - sam3s4c_sam3s_ek
 * - sam3sd8c_sam3s_ek2
 * - sam3u4e_sam3u_ek
 * - sam3x8h_sam3x_ek
 * - sam4s16c_sam4s_ek
 * - sam4sd32c_sam4s_ek2
 * - sam4e16e_sam4e_ek
 * - sam4n16c_sam4n_xplained_pro
 * - sam4c16c_sam4c_ek
 * - sam4cp16b_sam4cp16bmb
 * - sam4cmp16c_sam4cmp_db
 * - sam4cms16c_sam4cms_db
 *
 * \section compinfo Compilation info
 * This software was written for the GNU GCC and IAR for ARM. Other compilers
 * may or may not work.
 *
 * \section contactinfo Contact Information
 * For further information, visit <a href="http://www.atmel.com/">Atmel</a>.\n
 * Support and FAQ: http://www.atmel.com/design-support/
 */

//! \name Unit test configuration
//@{
/**
 * \def CONF_TEST_FLASH_EFC
 * \brief Test the functions provided by the flash driver.
 */
//@}

/**
 * \brief Test flash device id interface.
 *
 * This test gets the device id of the flash.
 *
 * \param test Current test case.
 */
static void run_flash_device_id_test(const struct test_case *test)
{
	uint32_t ul_rc;
	uint32_t unique_id[4];

	/* Read flash device id */
	ul_rc = flash_read_unique_id(unique_id, 4);

	/* Validate the get flash device id function */
	test_assert_true(test, ul_rc == FLASH_RC_OK,
			"Test flash device id: flash read device id error!");
}

/**
 * \brief Test flash configure functions.
 *
 * \note This test gets the default wait state first, and then it will
 * initialize the flash again and set/adaptively set the wait state.
 *
 * \param test Current test case.
 */
static void run_flash_configure_test(const struct test_case *test)
{
	uint32_t ul_default_ws;
	uint32_t ul_tmp_ws;
	uint32_t ul_mck = sysclk_get_cpu_hz();

	/* Backup default wait state */
	ul_default_ws = flash_get_wait_state(IFLASH_ADDR);

	/* Check the flash init function */
	flash_init(FLASH_ACCESS_MODE_128, 6);

	/* Get the wait state set by flash_init */
	ul_tmp_ws = flash_get_wait_state(IFLASH_ADDR);

	/* Validate the flash initialization function */
	test_assert_true(test, ul_tmp_ws == 6,
			"Test flash configure: flash init error!");

	/* Set the new wait state */
	flash_set_wait_state(IFLASH_ADDR, ul_default_ws+1);

	/* Get the wait state */
	ul_tmp_ws =  flash_get_wait_state(IFLASH_ADDR);

	/* Validate the set flash wait state function */
	test_assert_true(test, ul_tmp_ws == (ul_default_ws+1),
			"Test flash configure: set wait state error!");

	/* Adaptively set the wait state */
	flash_set_wait_state_adaptively(IFLASH_ADDR);

	/* Validate the adaptively set wait state function, get the wait state and
	 * check with the default>
	 */
	if (ul_mck < CHIP_FREQ_FWS_0) {
		ul_tmp_ws =  flash_get_wait_state(IFLASH_ADDR);
		test_assert_true(test, ul_tmp_ws == 0,
				"Test flash configure:adaptively set wait state error!");
	} else if (ul_mck < CHIP_FREQ_FWS_1) {
		ul_tmp_ws =  flash_get_wait_state(IFLASH_ADDR);
		test_assert_true(test, ul_tmp_ws == 1,
				"Test flash configure:adaptively set wait state error!");
	} else if (ul_mck < CHIP_FREQ_FWS_2) {
		ul_tmp_ws =  flash_get_wait_state(IFLASH_ADDR);
		test_assert_true(test, ul_tmp_ws == 2,
				"Test flash configure:adaptively set wait state error!");
#if (SAM3XA || SAM3U)
	} else if (ul_mck < CHIP_FREQ_FWS_3) {
		ul_tmp_ws =  flash_get_wait_state(IFLASH_ADDR);
		test_assert_true(test, ul_tmp_ws == 3,
				"Test flash configure:adaptively set wait state error!");
	} else {
		ul_tmp_ws =  flash_get_wait_state(IFLASH_ADDR);
		test_assert_true(test, ul_tmp_ws == 4,
				"Test flash configure:adaptively set wait state error!");
	}
#elif (SAM4S || SAM4E || SAM4N || SAM4C || SAM4CP || SAM4CM)
	} else if (ul_mck < CHIP_FREQ_FWS_3) {
		ul_tmp_ws =  flash_get_wait_state(IFLASH_ADDR);
		test_assert_true(test, ul_tmp_ws == 3,
				"Test flash configure:adaptively set wait state error!");
	} else if (ul_mck < CHIP_FREQ_FWS_4) {
		ul_tmp_ws =  flash_get_wait_state(IFLASH_ADDR);
		test_assert_true(test, ul_tmp_ws == 4,
				"Test flash configure:adaptively set wait state error!");
	} else {
		ul_tmp_ws =  flash_get_wait_state(IFLASH_ADDR);
		test_assert_true(test, ul_tmp_ws == 5,
				"Test flash configure:adaptively set wait state error!");
	}
#else
	} else {
		ul_tmp_ws =  flash_get_wait_state(IFLASH_ADDR);
		test_assert_true(test, ul_tmp_ws == 3,
				"Test flash configure:adaptively set wait state error!");
	}
#endif

	/*Restore default wait state */
	flash_set_wait_state(IFLASH_ADDR, ul_default_ws+1);
}

/**
 * \brief Test get flash information interfaces.
 *
 * \note This test gets the descriptor first, and then it will read out the
 * page count, page count per region and region count number.
 *
 * \param test Current test case.
 */
static void run_flash_information_test(const struct test_case *test)
{
	uint32_t ul_flash_descriptor[8];
	uint32_t ul_page_count;
	uint32_t ul_page_count_per_region;
	uint32_t ul_region_count;

	/* Get the flash descriptor */
	flash_get_descriptor(IFLASH_ADDR, ul_flash_descriptor, 8);

	ul_page_count = flash_get_page_count(ul_flash_descriptor);

	/* Validate the get page count function */
	test_assert_true(test, ul_page_count == DEFAULT_PAGE_COUNT,
			"Test flash information:get page count error!");

	/* Read the page count number in one region */
	ul_page_count_per_region =
			flash_get_page_count_per_region(ul_flash_descriptor);

	/* Validate the get page count per region function */
	test_assert_true(test, (ul_page_count_per_region == IFLASH_NB_OF_PAGES),
			"Test flash information:get page count per region error!");

	/* Get the region number */
	ul_region_count = flash_get_region_count(ul_flash_descriptor);

	/* Validate the get region count function */
	test_assert_true(test, (ul_region_count == DEFAULT_REGION_COUNT),
			"Test flash information:get region count error!");
}

#if SAM3SD8
/**
 * \brief Test flash erasing.
 *
 * \note This test erases the whole flash and then reads out the flash content
 * to ensure the erase operation. As this test will erase one plane flash,
 * it is valid for dual plane MCU, SAM3SD8 only.
 *
 * \param test Current test case.
 */
static void run_flash_erase_test(const struct test_case *test)
{
	uint32_t ul_idx;
	volatile uint32_t ul_test_page_addr = IFLASH1_ADDR;
	uint32_t *pul_test_page = (uint32_t *) ul_test_page_addr;
	uint32_t ul_rc=0;

	/* Unlock the whole flash */
	flash_unlock(IFLASH1_ADDR, TEST_PAGE_ADDRESS + IFLASH_PAGE_SIZE - 1, 0, 0);

	/* Erase the second plane */
	flash_erase_plane(IFLASH1_ADDR);

	/* The data will all be 0xff after erasing all operation */
	for (ul_idx=0;ul_idx<(IFLASH1_SIZE/4);ul_idx++) {
		if ((uint32_t)pul_test_page[ul_idx] != 0xffffffff){
			ul_rc=1;
			break;
		}
	}

	/* Validate the plane erase function */
	test_assert_true(test, ul_rc == 0, "Test flash erase: erase plane error!");

}
#endif

/**
 * \brief Test flash writing.
 *
 * This test tests the flash writing function in the test page of the flash.
 *
 * \param test Current test case.
 */
static void run_flash_write_test(const struct test_case *test)
{
	uint32_t ul_page_buffer[IFLASH_PAGE_SIZE / sizeof(uint32_t)];
	uint32_t ul_rc=0;
	uint32_t ul_idx;
	uint32_t ul_test_page_addr = TEST_PAGE_ADDRESS;
	uint32_t *pul_test_page = (uint32_t *) ul_test_page_addr;

	/* Unlock the whole flash */
	flash_unlock(IFLASH_ADDR,  ul_test_page_addr + IFLASH_PAGE_SIZE - 1, 0, 0);

	/* Write the test page with walking bit pattern */
	for (ul_idx = 0; ul_idx < (IFLASH_PAGE_SIZE / 4); ul_idx++) {
		ul_page_buffer[ul_idx] = 1 << (ul_idx % 32);
	}

#if (SAM4S || SAM4E || SAM4N || SAM4C || SAM4CP || SAM4CM)
	/* Write the test page */
	flash_erase_sector(ul_test_page_addr);

	flash_write(ul_test_page_addr,
			(void *)ul_page_buffer,
			IFLASH_PAGE_SIZE, 0);
#else
	flash_write(ul_test_page_addr,
			(void *)ul_page_buffer,
			IFLASH_PAGE_SIZE, 1);
#endif

	/* Validate page contents */
	for (ul_idx = 0; ul_idx < (IFLASH_PAGE_SIZE / 4); ul_idx++) {
		if (pul_test_page[ul_idx] != ul_page_buffer[ul_idx]) {
			ul_rc =1;
		}
	}

	/* Validate the flash write function */
	test_assert_true(test, ul_rc == 0, "Test flash write: flash write error!");
}

/**
 * \brief Test flash lock setting.
 *
 * \note This test gets the lock status of the whole flash space and
 * unlocks/locks the test page for validation.
 *
 * \param test Current test case.
 */
static void run_flash_lock_test(const struct test_case *test)
{
	volatile uint32_t ul_locked_region_num;
	volatile uint32_t lockerror = 0;
	uint32_t ul_test_page_addr = TEST_PAGE_ADDRESS;
#if (SAM4S || SAM4E || SAM4N || SAM4C || SAM4CP || SAM4CM)
	flash_erase_sector(ul_test_page_addr);
#endif
	uint32_t ul_page_buffer[IFLASH_PAGE_SIZE / sizeof(uint32_t)];
	memset((void *)ul_page_buffer, 0xFF, IFLASH_PAGE_SIZE);


	/* Check if there is any region blocked */
	ul_locked_region_num = flash_is_locked(IFLASH_ADDR,
			ul_test_page_addr + IFLASH_PAGE_SIZE - 1);

	/* If all the regions are unlocked, set the test page locked */
	if (ul_locked_region_num == 0) {
		/* Lock the test page */
		flash_lock(ul_test_page_addr,
				ul_test_page_addr + IFLASH_PAGE_SIZE - 1, 0, 0);
	}

	/* Unlock the test page region */
	flash_unlock(IFLASH_ADDR, ul_test_page_addr + IFLASH_PAGE_SIZE - 1, 0, 0);

	/* Check if there is any region blocked */
	ul_locked_region_num = flash_is_locked(IFLASH_ADDR,
			ul_test_page_addr + IFLASH_PAGE_SIZE - 1);

	/* Validate the the unlock function */
	test_assert_true(test, ul_locked_region_num == 0,
			"Test flash lock: flash unlock error!");

	/* Lock the test page region */
	flash_lock(ul_test_page_addr,
			ul_test_page_addr + IFLASH_PAGE_SIZE - 1, 0, 0);

	lockerror = flash_write(ul_test_page_addr, (void *)ul_page_buffer,
			IFLASH_PAGE_SIZE, 0);

#if (SAM3SD8 || SAM4S || SAM4E || SAM4N || SAM4C || SAM4CP || SAM4CM)
	/* SAM3SD8, SAM4S, SAM4E, SAM4N, SAM4C and SAM4CP have a bigger page region which
	 * requires special attention.
	 */
	ul_locked_region_num = flash_is_locked(IFLASH_ADDR,
			ul_test_page_addr + IFLASH_PAGE_SIZE - 1);
#else
	ul_locked_region_num = flash_is_locked(ul_test_page_addr,
			ul_test_page_addr + IFLASH_PAGE_SIZE - 1);
#endif

	/* Unlock the test page region */
	flash_unlock(IFLASH_ADDR, ul_test_page_addr + IFLASH_PAGE_SIZE - 1, 0, 0);

	/* Validate the lock function */
	test_assert_true(test, ul_locked_region_num == 1,
			"Test flash lock: flash lock error!");
	test_assert_true(test, lockerror == EEFC_FSR_FLOCKE,
			"Test flash lock: expect a flash lock error!");
}

/**
 * \brief Test flash GPNVM setting.
 *
 * This test gets the gpnvm 1 status and then sets and clears the status.
 *
 * \param test Current test case.
 */
static void run_flash_gpnvm_test(const struct test_case *test)
{
	uint32_t ul_rc;

	/* Get the default gpnvm 1 status */
	ul_rc = flash_is_gpnvm_set(1);

	/* Reverse the test, clear/set  the gpnvm */
	if (ul_rc == FLASH_RC_YES) {  /* default gpnvm 1 is set */
		/* Clear the gpnvm 1 */
		flash_clear_gpnvm(1);

		/* Get the gpnvm 1 status */
		ul_rc = flash_is_gpnvm_set(1);

		/* Validate the gpnvm clear interface */
		test_assert_true(test, ul_rc == FLASH_RC_NO,
				"Test flash GPNVM: flash GPNVM clear error!");
	} else {
		/* Set the gpnvm 1 */
		flash_set_gpnvm(1);

		ul_rc = flash_is_gpnvm_set(1);

		/* Validate the gpnvm set interface */
		test_assert_true(test, ul_rc == FLASH_RC_YES,
				"Test flash GPNVM: flash GPNVM set error!");
	}

	/* Reverse the test, set/clear the gpnvm */
	if (ul_rc == FLASH_RC_YES) {
		/* Clear the gpnvm 1 */
		flash_clear_gpnvm(1);

		/* Get the gpnvm 1 status */
		ul_rc = flash_is_gpnvm_set(1);

		/* Validate the gpnvm clear interface */
		test_assert_true(test, ul_rc == FLASH_RC_NO,
				"Test flash GPNVM: flash GPNVM clear error!");
	} else {
		/* Set the gpnvm 1 */
		flash_set_gpnvm(1);

		/* Get the gpnvm 1 status */
		ul_rc = flash_is_gpnvm_set(1);

		/* Validate the gpnvm set interface */
		test_assert_true(test, ul_rc == FLASH_RC_YES,
				"Test flash GPNVM: flash GPNVM set error!");
	}
}


/**
 * \brief Run flash efc driver unit tests.
 */
int main(void)
{
	const usart_serial_options_t usart_serial_options = {
		.baudrate   = CONF_TEST_BAUDRATE,
		.paritytype = CONF_TEST_PARITY
	};

	/* Initialize the system clock and board */
	sysclk_init();
	board_init();

	/* Enable the debug uart */
	sysclk_enable_peripheral_clock(CONSOLE_UART_ID);
	stdio_serial_init(CONF_TEST_USART, &usart_serial_options);

	/* Define all the test cases */
	DEFINE_TEST_CASE(flash_device_id_test, NULL, run_flash_device_id_test, NULL,
			"Readout flash device id");

	DEFINE_TEST_CASE(flash_configure_test, NULL, run_flash_configure_test, NULL,
			"Set, adaptively set and get the wait state for the flash");

	DEFINE_TEST_CASE(flash_information_test, NULL, run_flash_information_test,
			NULL,
			"Get the descriptor, page count, page count per region and region count");

	DEFINE_TEST_CASE(flash_write_test, NULL, run_flash_write_test, NULL,
			"Flash write test");

	DEFINE_TEST_CASE(flash_lock_test, NULL, run_flash_lock_test, NULL,
			"Flash lock, unlock and get lock status");

	DEFINE_TEST_CASE(flash_gpnvm_test, NULL, run_flash_gpnvm_test, NULL,
			"GPNVM set, clear and get status");

#if SAM3SD8
	DEFINE_TEST_CASE(flash_erase_test, NULL, run_flash_erase_test, NULL,
			"Erase function test");
#endif
	/* Put test case addresses in an array */
	DEFINE_TEST_ARRAY(flash_efc_tests) = {
		&flash_device_id_test,
		&flash_configure_test,
		&flash_information_test,
		&flash_write_test,
		&flash_lock_test,
		&flash_gpnvm_test,
#if SAM3SD8
		&flash_erase_test,
#endif
	};

	/* Define the test suite */
	DEFINE_TEST_SUITE(flash_efc_suite, flash_efc_tests,
			"SAM flash efc driver test suite");

	/* Run all tests in the test suite */
	test_suite_run(&flash_efc_suite);

	while (1) {
		/* Busy-wait forever */
	}
}
