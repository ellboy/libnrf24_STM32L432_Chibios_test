#include "ch.h"
#include "hal.h"

#include "string.h"

#include "debug.h"
#include "rf.h"

#define  TRANSMITTER                   FALSE

#define  GPIO_RF_CE_LINE				   LINE_ARD_D1
#define  GPIOA_RF_CE                       9 // D1 CH5
#define  GPIOA_RF_IRQ                      1 // A1 CH3
#define  GPIOB_RF_SPID1_CS                 0 // D5 CH4
#define  GPIOA_RF_SPID1_SCK                5 // A4 CH2
#define  GPIOA_RF_SPID1_MISO               6 // A5 CH1
#define  GPIOA_RF_SPID1_MOSI               7 // A6 CH0

static const SPIConfig std_spi_cfg = {
  NULL,
  GPIOB,                                   /*   port of CS  */
  GPIOB_RF_SPID1_CS,                       /*   pin of CS   */
  SPI_CR1_BR_1 | SPI_CR1_BR_0              /*   CR1 register*/
};

static const EXTConfig extcfg = {
  {
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_FALLING_EDGE | EXT_CH_MODE_AUTOSTART |EXT_MODE_GPIOA, rfExtCallBack},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
  }
};


static RFConfig nrf24l01_cfg = {
  GPIOA,
  GPIOA_RF_CE,
  GPIOA,
  GPIOA_RF_IRQ,
  &SPID1,
  &std_spi_cfg,
  &EXTD1,
  &extcfg,
  NRF24L01_ARC_15_times,     /* auto_retr_count */
  NRF24L01_ARD_250us,       /* auto_retr_delay */
  NRF24L01_AW_5_bytes,       /* address_width */
  76,                       /* channel_freq 2.4 + 0.12 GHz */
  NRF24L01_ADR_1Mbps,        /* data_rate */
  NRF24L01_CRC_8bit,
  NRF24L01_PWR_neg18dBm,         /* out_pwr */
  NRF24L01_LNA_disabled,     /* lna */
  NRF24L01_DPL_enabled ,     /* en_dpl */
  NRF24L01_AutoAck_enabled,
  NRF24L01_ACK_PAY_disabled, /* en_ack_pay */
  NRF24L01_DYN_ACK_disabled, /* en_dyn_ack */
  MS2ST(100)
};

/*===========================================================================*/
/* Generic code.                                                             */
/*===========================================================================*/


/*
 * Green LED blinker thread, times are in milliseconds.
 */
static THD_WORKING_AREA(waThread, 512);
static THD_FUNCTION(Thread, arg) {
	(void)arg;

  uint32_t strl;

  chRegSetThreadName("RF thread");

  rfStart(&RFD1, &nrf24l01_cfg);

  print_dbg("RF Running\r\n");
  uint8_t addresses[][6] = {"1Node","2Node"};
#if TRANSMITTER == TRUE
  print_dbg("TRANSMITTER: \r\n");
  char tx_string[RF_MAX_STRLEN + 1] = "test";
#else
  print_dbg("RECEIVER\r\n");
  char rx_string[RF_MAX_STRLEN + 1];
#endif

  while (TRUE) {
#if TRANSMITTER == TRUE
		rf_msg_t msg;

		strl = strlen(tx_string);
		if (strl) {
			msg = rfTransmitData(&RFD1, tx_string, strlen(tx_string) + 1, addresses[0]);
			if (msg == RF_OK) {
				print_dbg("Message send: %s\r\n", tx_string);
			} else if (msg == RF_ERROR) {
				chnWrite(&SD2,
						(const uint8_t * )"Message not sent (MAX_RT)\n\r", 27);
			} else {
				chnWrite(&SD2,
						(const uint8_t * )"Message not sent (TIMEOUT)\n\r", 28);
			}
		}
		chThdSleepMilliseconds(500);
#else
    strl = rfReceiveData(&RFD1, rx_string, sizeof(rx_string), addresses[0]);
    if(strl){
    	print_dbg("%d bytes: %s\r\n", strl, rx_string);
    }
#endif
  }
  rfStop(&RFD1);
}

/*
 * Application entry point.
 */
int main(void) {

  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();

  palSetLine(LINE_LED_GREEN);
  chThdSleepMilliseconds(50);
  palClearLine(LINE_LED_GREEN);

  /*
   * Application library initialization.
   * - PLAY initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   */
  rfInit();

  /*
   * SPID1 I/O pins setup.(It bypasses board.h configurations)
   */
  palSetPadMode(GPIOA, GPIOA_RF_SPID1_SCK ,
                 PAL_MODE_ALTERNATE(5)|  PAL_STM32_OSPEED_HIGH );   /* New SCK */
  palSetPadMode(GPIOA, GPIOA_RF_SPID1_MISO,
                 PAL_MODE_ALTERNATE(5)|  PAL_STM32_OSPEED_HIGH );   /* New MISO*/
  palSetPadMode(GPIOA, GPIOA_RF_SPID1_MOSI,
                 PAL_MODE_ALTERNATE(5) |  PAL_STM32_OSPEED_HIGH);   /* New MOSI*/
  palSetPadMode(GPIOB, GPIOB_RF_SPID1_CS,
		  PAL_MODE_OUTPUT_PUSHPULL  );/* New CS  */

//  palSetPad(GPIOB, GPIOB_RF_SPID1_CS);
//
//  palClearPad(GPIOB, GPIOB_RF_SPID1_CS);
//
//  palSetPad(GPIOB, GPIOB_RF_SPID1_CS);



  /*
   * CE and IRQ pins setup.
   */
  palSetLineMode(GPIO_RF_CE_LINE, PAL_MODE_OUTPUT_PUSHPULL |  PAL_STM32_OSPEED_HIGH);
  palSetLineMode(LINE_ARD_A1, PAL_MODE_INPUT);         /* New IRQ  */


  sdStart(&SD2, NULL);

  print_dbg("RUN DBG\r\n");
  chThdCreateStatic(waThread, sizeof(waThread), NORMALPRIO, Thread, NULL);
  while(TRUE){
	  //extChannelEnable(&EXTD1, 1);
    chThdSleepMilliseconds(500);
  }
}
