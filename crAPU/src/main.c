/**
 * \file
 *
 * \brief Empty user application template
 *
 */

/**
 * \mainpage User Application template doxygen documentation
 *
 * \par Empty user application template
 *
 * Bare minimum empty user application template
 *
 * \par Content
 *
 * -# Include the ASF header files (through asf.h)
 * -# Minimal main function that starts with a call to system_init()
 * -# "Insert application code here" comment
 *
 */

/*
 * Include header files for all drivers that have been imported from
 * Atmel Software Framework (ASF).
 */
/*
 * Support and FAQ: visit <a href="http://www.atmel.com/design-support/">Atmel Support</a>
 */
#include <asf.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#define DEGREES_PER_CYCLE 360
/* one degree is equal to 0.0174532925 radian */
#define DEGREE 0.0174532925
/* Waveform type macros */ 
#define SINE_WAVE		0
#define SAW_TOOTH_WAVE	1
#define TRIANGLE_WAVE	2

/* Waveform selection macro - Selects any one of the three waveforms */
#define WAVE_MODE SINE_WAVE
/* Waveform Frequency in Hz, specify a value less than 970Hz for 
   sine wave and less than 1360Hz for other two waves */
#define FREQUENCY 440

/* Function Prototypes */
void timer_init(void);
void dac_initialize(void);
void evsys_init(void);
void buffer_init(void);

/* Driver module structure declaration */
struct tc_module tc_inst;
struct dac_module dac_inst;

/* Buffer variable declaration */
#if WAVE_MODE==SINE_WAVE
uint16_t sine_wave_buf[DEGREES_PER_CYCLE];
#elif WAVE_MODE==SAW_TOOTH_WAVE
uint16_t sawtooth_wave_buf[256];
#elif WAVE_MODE==TRIANGLE_WAVE
uint16_t triangle_wave_buf[256];
#endif

/* Other global variables */
uint16_t arr_index;
uint16_t nextVal = 0;

uint16_t sin_table[20] = { 511, 669, 811, 924, 997, 1022, 997, 924, 811, 669, 511, 353, 211, 98, 25, 0, 25, 98, 211, 353 };
uint16_t saw_table[20] = { 0, 50, 100, 150, 200, 250, 300, 350, 400, 450, 500, 550, 600, 650, 700, 750, 800, 850, 900, 950 };

/* Timer 3 interrupt handler */
void TC3_Handler(void)
{
	// static uint16_t count = 0;
	static uint16_t dacCount = 0;

	TC3->COUNT16.INTFLAG.reg = TC_INTFLAG_MC0;

	// make sure DAC is done setting previous value
	while (dac_inst.hw->STATUS.reg & DAC_STATUS_SYNCBUSY) { ; };
	// set new DAC value
	dac_inst.hw->DATA.reg = sin_table[dacCount];

	dacCount = (dacCount + 1) % 20;

	// count++;
	// if (count < 10) {return;}
	// count = 10;
	// if (count < 20) {count = 0;}
	// #if WAVE_MODE==SINE_WAVE
	//dac_chan_write(&dac_inst, 0, newVal);
	//nextVal = 
	// dac_chan_write(&dac_inst, 0, sine_wave_buf[arr_index++]);
	// if ((count%10) == 0) {
	// 	port_pin_toggle_output_level(LED1_PIN);
	// }
	// if (arr_index == DEGREES_PER_CYCLE) {
	// 	arr_index = 0;
	// }
	// #elif WAVE_MODE==SAW_TOOTH_WAVE
	// dac_chan_write(&dac_inst, DAC_CHANNEL_0, saw[count]);
	// if (arr_index == 256) {
	// 	arr_index = 0;
	// }
	// #elif WAVE_MODE==TRIANGLE_WAVE
	// dac_chan_write(&dac_inst, 0, triangle_wave_buf[arr_index++]);
	// if (arr_index == 256) {
	// 	arr_index = 0;
	// }
	// #endif
}

/* Timer 3 Initialization */
void timer_init(void)
{
	struct tc_config conf_tc;
	struct tc_events conf_tc_events = {.generate_event_on_compare_channel[0] = 1};
	tc_get_config_defaults(&conf_tc);
	conf_tc.clock_source = GCLK_GENERATOR_0;
	//conf_tc.counter_size = TC_COUNTER_SIZE_8BIT;
	conf_tc.clock_prescaler = TC_CLOCK_PRESCALER_DIV1;
	conf_tc.wave_generation = TC_WAVE_GENERATION_MATCH_FREQ;
	// conf_tc.counter_8_bit.value = 0;
	// conf_tc.counter_8_bit.period = 40;
	conf_tc.counter_16_bit.compare_capture_channel[0] = 0xFFFF;
	tc_init(&tc_inst, TC3, &conf_tc);
	tc_enable_events(&tc_inst, &conf_tc_events);
	tc_enable(&tc_inst);
	tc_stop_counter(&tc_inst);
	/* Enable TC3 match/capture channel 0 interrupt */
	TC3->COUNT16.INTENSET.reg = TC_INTENSET_MC0;
	// tc_set_compare_value(&tc_inst, 0, 30);
	/* Enable TC3 module interrupt */
	NVIC_EnableIRQ(TC3_IRQn);
}

/* DAC Initialization */
void dac_initialize(void)
{
	struct dac_config conf_dac;
	//struct dac_events conf_dac_events = {.on_event_start_conversion = 1};
	dac_get_config_defaults(&conf_dac);
	//conf_dac.clock_source = GCLK_GENERATOR_0;
	conf_dac.reference = DAC_REFERENCE_AVCC;
	if (dac_init(&dac_inst, DAC, &conf_dac) != STATUS_OK) {
		port_pin_set_output_level(LED0_PIN, true);
	}
	//dac_enable_events(&dac_inst, &conf_dac_events);
	dac_enable(&dac_inst);
}

/* Event System Initialization */
void evsys_init(void)
{
	struct events_resource conf_event_resource;
	struct events_config conf_event;
	events_get_config_defaults(&conf_event);
	conf_event.edge_detect = EVENTS_EDGE_DETECT_NONE;
	conf_event.path = EVENTS_PATH_ASYNCHRONOUS;
	conf_event.generator = EVSYS_ID_GEN_TC3_MCX_0;
	events_allocate(&conf_event_resource, &conf_event);
	events_attach_user(&conf_event_resource, EVSYS_ID_USER_DAC_START);
}

/* Initialize the selected waveform buffer with output data */
// void buffer_init(void)
// {
// 	#if WAVE_MODE==SINE_WAVE
// 	for (i = 0; i < DEGREES_PER_CYCLE; i++)	{
// 		sine_wave_buf[i] = (uint16_t)(511 + (511*sin((double)i*DEGREE)));
// 		//sine_wave_buf[i] = 1023;
// 	}
// 	#elif WAVE_MODE==SAW_TOOTH_WAVE
// 	for (i = 0; i < 256; i++) {
// 		sawtooth_wave_buf[i] = i*4;
// 	}
// 	#elif WAVE_MODE==TRIANGLE_WAVE
// 	for (i = 0; i < 128; i++) {
// 		triangle_wave_buf[i] = i*8;
// 	}
// 	for (i = 128; i < 256; i++) {
// 		triangle_wave_buf[i] = 1023 - (i*8);
// 	}
// 	#endif
// }

/** Configure LED0, turn it off*/
static void config_led(void)
{
	struct port_config pin_conf;
	port_get_config_defaults(&pin_conf);

	pin_conf.direction  = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(LED0_PIN, &pin_conf);
	port_pin_set_config(LED1_PIN, &pin_conf);
	port_pin_set_config(LED2_PIN, &pin_conf);
	port_pin_set_output_level(LED0_PIN, false);
	port_pin_set_output_level(LED1_PIN, true);
	port_pin_set_output_level(LED2_PIN, false);
}

static void config_pins(void)
{
	struct port_config pin_conf;
	port_get_config_defaults(&pin_conf);

	pin_conf.direction  = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(LED0_PIN, &pin_conf);
	port_pin_set_config(LED1_PIN, &pin_conf);
	port_pin_set_config(LED2_PIN, &pin_conf);
	port_pin_set_config(PIN_PA08, &pin_conf); // shutdown
	port_pin_set_output_level(LED0_PIN, false);
	port_pin_set_output_level(LED1_PIN, false);
	port_pin_set_output_level(LED2_PIN, true);
	port_pin_set_output_level(PIN_PA08, false);
}

uint16_t delay() {
	uint16_t i = 100;
	while (i) {
		i--;
	}
	return i;
}

/* Main function */
int main(void)
{
	system_init();
	timer_init();
	config_pins();
	dac_initialize();
	evsys_init();
	// buffer_init();
	
	/* Set the TC3 compare value corresponding to specified frequency */
	//tc_set_compare_value(&tc_inst, 0, 10); // ~22.7kHz
	uint16_t freq = 440;
	tc_set_compare_value(&tc_inst, 0, system_gclk_gen_get_hz(GCLK_GENERATOR_0)/(freq*20) - 1);
	// dac_chan_write(&dac_inst, 0, 1000);

	/* Start TC3 timer */
	tc_start_counter(&tc_inst);
	/* Enable global interrupt */
	system_interrupt_enable_global();
	
	uint16_t i = 0;
	while (true) {
		// dac_chan_write(&dac_inst, DAC_CHANNEL_0, i);
		// i = (i + 10) % 0x3FF;
        // if (++i == 0x3FF) {
        //     i = 0;
        // }

		// port_pin_toggle_output_level(LED1_PIN);
		//delay();
	}
}