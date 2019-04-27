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
void configure_i2c_slave(void);
void configure_i2c_slave_callbacks(void);
void evsys_init(void);
void buffer_init(void);

void i2c_read_request_callback(struct i2c_slave_module *const module);
void i2c_slave_read_complete_callback(struct i2c_slave_module *const module);
void i2c_write_request_callback(struct i2c_slave_module *const module);
void i2c_slave_write_complete_callback(struct i2c_slave_module *const module);

void noteOn(uint8_t midiNote, uint8_t volume);
void noteOff(void);

/* Driver module structure declaration */
struct tc_module tc_inst;
struct dac_module dac_inst;
struct i2c_slave_module i2c_slave_inst;

#define DATA_LENGTH 10
static struct i2c_slave_packet packet_in;
static uint8_t read_buffer_in [DATA_LENGTH];

/* Buffer variable declaration */
#if WAVE_MODE==SINE_WAVE
uint16_t sine_wave_buf[DEGREES_PER_CYCLE];
#elif WAVE_MODE==SAW_TOOTH_WAVE
uint16_t sawtooth_wave_buf[256];
#elif WAVE_MODE==TRIANGLE_WAVE
uint16_t triangle_wave_buf[256];
#endif

/* Other global variables */
uint16_t freq = 0;
uint16_t arr_index;
uint16_t nextVal = 0;

uint16_t volatile midi_table[128] = { 8, 9, 9, 10, 10, 11, 12, 12, 13, 14, 15, 15, 16, 17, 18, 19, 21, 22, 23, 24, 26, 28, 29, 31, 33, 35, 37, 39, 41, 44, 46, 49, 52, 55, 58, 62, 65, 69, 73, 78, 82, 87, 92, 98, 104, 110, 117, 123, 131, 139, 147, 156, 165, 175, 185, 196, 208, 220, 233, 247, 262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494, 523, 554, 587, 622, 659, 698, 740, 784, 831, 880, 932, 988, 1047, 1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661, 1760, 1865, 1976, 2093, 2217, 2349, 2489, 2637, 2794, 2960, 3136, 3322, 3520, 3729, 3951, 4186, 4435, 4699, 4978, 5274, 5588, 5920, 6272, 6645, 7040, 7459, 7902, 8372, 8870, 9397, 9956, 10548, 11175, 11840, 12544 };

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
}

void i2c_read_request_callback(struct i2c_slave_module *const module)
{
	;
}

void i2c_write_request_callback(struct i2c_slave_module *const module)
{	
	/* Init i2c packet */
	packet_in.data_length = DATA_LENGTH;
	packet_in.data        = read_buffer_in;
	
	/* Read buffer from master */
	i2c_slave_read_packet_job(module, &packet_in);
}

void i2c_slave_write_complete_callback(struct i2c_slave_module *const module) {
	;
}

void i2c_slave_read_complete_callback(struct i2c_slave_module *const module) {
	// destination slave address
	port_pin_set_output_level(LED2_PIN, true);

	uint8_t midiNote = read_buffer_in[0];
	uint8_t velocity = read_buffer_in[1];

	if (midiNote & 0x80) {
		// note off
		noteOff();
	} else {
		// note on
		noteOn(midiNote, velocity);
	}
	
	// toggle LED to show activity
	port_pin_set_output_level(LED2_PIN, false);
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

void configure_i2c_slave(void)
{
	/* Initialize config structure and module instance */
	struct i2c_slave_config config_i2c_slave;
	i2c_slave_get_config_defaults(&config_i2c_slave);

	/* Change address and address_mode */
	// 	config_i2c_slave.address      = IN_ADDRESS;
	// 	config_i2c_slave.address_mode = I2C_SLAVE_ADDRESS_MODE_MASK;
	config_i2c_slave.address	  = 0xFF; // upper i2c address range
	config_i2c_slave.address_mask = 0x00; // lower i2c address range
	config_i2c_slave.address_mode = I2C_SLAVE_ADDRESS_MODE_RANGE;
	config_i2c_slave.pinmux_pad0  = PINMUX_PA22C_SERCOM3_PAD0;
	config_i2c_slave.pinmux_pad1  = PINMUX_PA23C_SERCOM3_PAD1;

	/* Initialize and enable device with config */
	i2c_slave_init(&i2c_slave_inst, SERCOM3, &config_i2c_slave);

	i2c_slave_enable(&i2c_slave_inst);
}

void configure_i2c_slave_callbacks(void)
{
	/* Register and enable callback functions */
	i2c_slave_register_callback(&i2c_slave_inst, i2c_read_request_callback,
			I2C_SLAVE_CALLBACK_READ_REQUEST);
	i2c_slave_enable_callback(&i2c_slave_inst,
			I2C_SLAVE_CALLBACK_READ_REQUEST);
			
	i2c_slave_register_callback(&i2c_slave_inst, i2c_slave_read_complete_callback,
			I2C_SLAVE_CALLBACK_READ_COMPLETE);
	i2c_slave_enable_callback(&i2c_slave_inst,
			I2C_SLAVE_CALLBACK_READ_COMPLETE);

	i2c_slave_register_callback(&i2c_slave_inst, i2c_write_request_callback,
			I2C_SLAVE_CALLBACK_WRITE_REQUEST);
	i2c_slave_enable_callback(&i2c_slave_inst,
			I2C_SLAVE_CALLBACK_WRITE_REQUEST);
			
	i2c_slave_register_callback(&i2c_slave_inst, i2c_slave_write_complete_callback,
			I2C_SLAVE_CALLBACK_WRITE_COMPLETE);
	i2c_slave_enable_callback(&i2c_slave_inst,
			I2C_SLAVE_CALLBACK_WRITE_COMPLETE);
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
	port_pin_set_output_level(PIN_PA08, true);
}

void noteOn(uint8_t midiNote, uint8_t volume) {
	// set new frequency
	freq = midi_table[midiNote];
	
	// update timer trigger for new frequency
	tc_set_compare_value(&tc_inst, 0, system_gclk_gen_get_hz(GCLK_GENERATOR_0)/(freq*20) - 1);
	
	// enable amplifier
	port_pin_set_output_level(PIN_PA08, false);
}

void noteOff() {
	// disable amplifier
	port_pin_set_output_level(PIN_PA08, true);
}

uint16_t volatile delay(uint16_t millis) {
	for (uint16_t j=millis; j>0; j--) {
		for (uint16_t i=650; i>0; i--) { ; }
	}
}

/* Main function */
int main(void)
{
	system_init();
	timer_init();
	config_pins();
	dac_initialize();
	configure_i2c_slave();
	configure_i2c_slave_callbacks();
	evsys_init();
	// buffer_init();

	/* Start TC3 timer */
	tc_start_counter(&tc_inst);
	/* Enable global interrupt */
	system_interrupt_enable_global();
	
	uint16_t i = 0;
	while (true) {
		// noteOn(60, 127);
		// delay(500);
		// noteOff();
		// delay(500);
		// noteOn(64, 127);
		// delay(500);
		// noteOff();
		// delay(500);
		// noteOn(67, 127);
		// delay(500);
		// noteOff();
		// delay(500);
	}
}