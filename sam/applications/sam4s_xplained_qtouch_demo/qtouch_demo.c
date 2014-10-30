/**
 * \file
 *
 * \brief SAM4S-Xplained QTouch and LED demonstration.
 *
 * Copyright (c) 2012 Atmel Corporation. All rights reserved.
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

/**
 * \mainpage SAM4S-Xplained QTouch and LED demonstration
 *
 * \section Purpose
 *
 * The QTouch LED demo will help new users get familiar with Atmel's
 * SAM family of microcontrollers. This basic application shows how to initialize
 * the QTouch library and flash the LEDs by setting the timer counter frequency
 * through the QTouch slider.
 *
 * \section Requirements
 *
 * This package can be used with SAM4S-Xplained evaluation kits.
 *
 * \section Description
 *
 * The demonstration program makes two LEDs on the board blink at a specific
 * rate. This rate is generated by using Time tick timer. The frequency of the
 * Time tick can be adjusted by the QTouch slider.
 *
 * \section Usage
 *
 * -# Build the program and download it inside the evaluation board. Please
 *    refer to the
 *    <a href="http://www.atmel.com/dyn/resources/prod_documents/doc6224.pdf">
 *    SAM-BA User Guide</a>, the
 *    <a href="http://www.atmel.com/dyn/resources/prod_documents/doc6310.pdf">
 *    GNU-Based Software Development</a>
 *    application note or to the
 *    <a href="ftp://ftp.iar.se/WWWfiles/arm/Guides/EWARM_UserGuide.ENU.pdf">
 *    IAR EWARM User Guide</a>,
 *    depending on your chosen solution.
 * -# Start the application.
 * -# Two LEDs should start blinking on the board.
 * -# Press and release the QTouch slider will set the LEDs blinking at a
 * different frequency.
 *
 */

#include "asf.h"
#include "conf_example.h"

/** QTouch library type: QTouch / QMatrix */
#define QTOUCH_LIB_TPYE_MASK    0x01

/** QTouch library compiler type offset: GCC / IAR */
#define QTOUCH_LIB_COMPILER_OFFSET    2

/** QTouch library compiler type mask */
#define QTOUCH_LIB_COMPILER_MASK    0x01

/** QTouch library maximum channels offset */
#define QTOUCH_LIB_MAX_CHANNEL_OFFSET    3

/** QTouch library maximum channels mask */
#define QTOUCH_LIB_MAX_CHANNEL_MASK   0x7F

/** QTouch library supports keys only offset */
#define QTOUCH_LIB_KEY_ONLY_OFFSET    10

/** QTouch library supports keys only mask */
#define QTOUCH_LIB_KEY_ONLY_MASK   0x01

/** QTouch library maximum rotors/sliders offset */
#define QTOUCH_LIB_ROTOR_NUM_OFFSET    11

/** QTouch library maximum rotors/sliders mask */
#define QTOUCH_LIB_ROTOR_NUM_MASK   0x1F

#define GET_SENSOR_STATE(SENSOR_NUMBER) (qt_measure_data.qt_touch_status. \
	sensor_states[(SENSOR_NUMBER / \
	8)] & (1 << (SENSOR_NUMBER % 8)))
#define GET_ROTOR_SLIDER_POSITION(ROTOR_SLIDER_NUMBER) qt_measure_data.	\
	qt_touch_status.rotor_slider_values[ROTOR_SLIDER_NUMBER]

/** Set default LED blink period to 250ms*3 */
#define DEFAULT_LED_FREQ   4

/** frequency divider for LED blinking */
#define LED_FREQ_DIV 3

/** LED blink period */
#define LED_BLINK_PERIOD    3

/** QTouch max data value */
#define QT_MAX_DATA    255

/* This configuration data structure parameters if needs to be changed will be
 * changed in the qt_set_parameters function */
extern qt_touch_lib_config_data_t qt_config_data;
/* Measurement data */
extern qt_touch_lib_measure_data_t qt_measure_data;

/** Flag set by timer ISR when it's time to measure touch */
static volatile uint8_t time_to_measure_touch = 0u;

/** Current time, set by timer ISR */
static volatile uint16_t current_time_ms_touch = 0u;

/** Timer period in msec */
uint16_t qt_measurement_period_msec = 25u;

/** Shift data gotten from the QTouch */
uint32_t qt_shift_data = 0;

/** LED blink period */
static volatile uint32_t led_blink_period = 0;

/// @cond 0
/**INDENT-OFF**/
#ifdef __cplusplus
extern "C" {
#endif
/**INDENT-ON**/
/// @endcond

/**
 *  \brief Handler for System Tick interrupt.
 *
 *  Process System Tick Event
 */
void SysTick_Handler(void)
{
	/* Set flag: it's time to measure touch */
	time_to_measure_touch = 1u;

	/* Update the current time */
	current_time_ms_touch += qt_measurement_period_msec;
}

/**
 * \brief Interrupt handler for TC0 interrupt. Toggles the state of LEDs.
 */
void TC0_Handler(void)
{
	/* Clear status bit to acknowledge interrupt */
	tc_get_status(TC0, 0);

	led_blink_period++;

	if (led_blink_period == LED_BLINK_PERIOD) {
		/** Toggle LED state. */
		gpio_toggle_pin(LED0_GPIO);
		gpio_toggle_pin(LED1_GPIO);
		led_blink_period = 0;
	}
}

/**
 * \brief Configure Timer Counter 0 to generate an interrupt with the specific
 * frequency.
 *
 * \param freq Timer counter frequency.
 */
static void configure_tc(uint32_t freq)
{
	uint32_t ul_div;
	uint32_t ul_tcclks;
	uint32_t ul_sysclk = sysclk_get_cpu_hz();

	/* Disable TC first */
	tc_stop(TC0, 0);
	tc_disable_interrupt(TC0, 0, TC_IER_CPCS);

	/** Configure TC with the frequency and trigger on RC compare. */
	tc_find_mck_divisor(freq, ul_sysclk, &ul_div, &ul_tcclks, ul_sysclk);
	tc_init(TC0, 0, ul_tcclks | TC_CMR_CPCTRG);
	tc_write_rc(TC0, 0, (ul_sysclk / ul_div) / 4);

	/* Configure and enable interrupt on RC compare */
	NVIC_EnableIRQ((IRQn_Type)ID_TC0);
	tc_enable_interrupt(TC0, 0, TC_IER_CPCS);

	/** Start the counter. */
	tc_start(TC0, 0);
}

/**
 * \brief Configure timer ISR to fire regularly.
 */
static void init_timer_isr(void)
{
	SysTick_Config((sysclk_get_cpu_hz() / 1000) *
			qt_measurement_period_msec);
}

/**
 * \brief This will fill the default threshold values in the configuration
 * data structure.But User can change the values of these parameters.
 */
static void qt_set_parameters(void)
{
	qt_config_data.qt_di = DEF_QT_DI;
	qt_config_data.qt_neg_drift_rate = DEF_QT_NEG_DRIFT_RATE;
	qt_config_data.qt_pos_drift_rate = DEF_QT_POS_DRIFT_RATE;
	qt_config_data.qt_max_on_duration = DEF_QT_MAX_ON_DURATION;
	qt_config_data.qt_drift_hold_time = DEF_QT_DRIFT_HOLD_TIME;
	qt_config_data.qt_recal_threshold = DEF_QT_RECAL_THRESHOLD;
	qt_config_data.qt_pos_recal_delay = DEF_QT_POS_RECAL_DELAY;
}

/**
 * \brief Configure the sensors.
 */
static void config_sensors(void)
{
	qt_enable_slider(BOARD_SLIDER_START_CHANNEL, BOARD_SLIDER_END_CHANNEL,
			AKS_GROUP_1, 16u, HYST_6_25, RES_8_BIT, 0u);
}

/**
 * \brief initialize pins, watchdog, etc.
 */
static void init_system(void)
{
	/* Disable the watchdog */
	wdt_disable(WDT);

        /* Initialize the system clock */
	sysclk_init();

	/* Configure LED pins */
	gpio_configure_pin(LED0_GPIO, LED0_FLAGS);
	gpio_configure_pin(LED1_GPIO, LED1_FLAGS);

	/* Enable PMC clock for key/slider PIOs  */
	pmc_enable_periph_clk(ID_PIOA);
	pmc_enable_periph_clk(ID_PIOC);

	/* Configure PMC */
	pmc_enable_periph_clk(ID_TC0);

        /* Configure the default TC frequency */
	configure_tc(DEFAULT_LED_FREQ);
}

/**
 *  \brief getting-started Application entry point.
 *
 *  \return Unused (ANSI-C compatibility).
 */
int main(void)
{
	/*Status flags to indicate the re-burst for library */
	uint16_t status_flag = 0u;
	uint16_t burst_flag = 0u;

	/* Initialize the SAM system */
	init_system();

	/* Reset touch sensing */
	qt_reset_sensing();

	/* Configure the Sensors as keys or Keys With Rotor/Sliders in this
	   function */
	config_sensors();

	/* Initialize touch sensing */
	qt_init_sensing();

	/* Set the parameters like recalibration threshold, Max_On_Duration etc
	   in this function by the user */
	qt_set_parameters();

	/* Configure timer ISR to fire regularly */
	init_timer_isr();

	/* Address to pass address of user functions */

	/* This function is called after the library has made capacitive
	   measurements, but before it has processed them. The user can
	   use this hook to apply filter functions to the measured signal
	   values.(Possibly to fix sensor layout faults) */
	qt_filter_callback = 0;

	/* Loop forever */
	for (;;) {
		if (time_to_measure_touch) {
			/* Clear flag: it's time to measure touch */
			time_to_measure_touch = 0u;

			do {
				/*  One time measure touch sensors    */
				status_flag = qt_measure_sensors
							(current_time_ms_touch);

				burst_flag = status_flag & QTLIB_BURST_AGAIN;

				/*Time critical host application code goes here */

			} while (burst_flag);
		}

		/* Get slider value and update the blink frequency */
		if (GET_ROTOR_SLIDER_POSITION(0) != qt_shift_data) {
			qt_shift_data = GET_ROTOR_SLIDER_POSITION(0);
#ifdef QT_DATA_REVERT
			configure_tc((QT_MAX_DATA - qt_shift_data)/LED_FREQ_DIV);
#else
			configure_tc(qt_shift_data/LED_FREQ_DIV);
#endif
		}
	}
}

/// @cond 0
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/// @endcond
