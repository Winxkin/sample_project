#include "sx1278.h"

/**
 * Write a value to a register.
 * @param reg Register index.
 * @param val Value to write.
 */

void sx1278_write_reg(int reg, int val)
{
   uint8_t out[2] = {0x80 | reg, val};
   uint8_t in[2];

   spi_transaction_t t = {
       .flags = 0,
       .length = 8 * sizeof(out),
       .tx_buffer = out,
       .rx_buffer = in};

   gpio_set_level(CONFIG_CS_GPIO, 0);
   spi_device_transmit(__spi, &t);
   gpio_set_level(CONFIG_CS_GPIO, 1);
}

/**
 * Read the current value of a register.
 * @param reg Register index.
 * @return Value of the register.
 */

int sx1278_read_reg(int reg)
{
   uint8_t out[2] = {reg, 0xff};
   uint8_t in[2];

   spi_transaction_t t = {
       .flags = 0,
       .length = 8 * sizeof(out),
       .tx_buffer = out,
       .rx_buffer = in};

   gpio_set_level(CONFIG_CS_GPIO, 0);
   spi_device_transmit(__spi, &t);
   gpio_set_level(CONFIG_CS_GPIO, 1);
   return in[1];
}

/**
 * Perform physical reset on the Lora chip
 */

void sx1278_reset(void)
{
   gpio_set_level(CONFIG_RST_GPIO, 0);
   vTaskDelay(pdMS_TO_TICKS(1));
   gpio_set_level(CONFIG_RST_GPIO, 1);
   vTaskDelay(pdMS_TO_TICKS(10));
}

/**
 * Configure explicit header mode.
 * Packet size will be included in the frame.
 */
void sx1278_explicit_header_mode(void)
{
   __implicit = 0;
   sx1278_write_reg(REG_MODEM_CONFIG_1, sx1278_read_reg(REG_MODEM_CONFIG_1) & 0xfe);
}

/**
 * Configure implicit header mode.
 * All packets will have a predefined size.
 * @param size Size of the packets.
 */

void sx1278_implicit_header_mode(int size)
{
   __implicit = 1;
   sx1278_write_reg(REG_MODEM_CONFIG_1, sx1278_read_reg(REG_MODEM_CONFIG_1) | 0x01);
   sx1278_write_reg(REG_PAYLOAD_LENGTH, size);
}

/**
 * Sets the radio transceiver in idle mode.
 * Must be used to change registers and access the FIFO.
 */

void sx1278_idle(void)
{
   sx1278_write_reg(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_STDBY);
}

/**
 * Sets the radio transceiver in sleep mode.
 * Low power consumption and FIFO is lost.
 */

void sx1278_sleep(void)
{
   sx1278_write_reg(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_SLEEP);
}

/**
 * Sets the radio transceiver in receive mode.
 * Incoming packets will be received.
 */

void sx1278_receive(void)
{
   sx1278_write_reg(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_RX_CONTINUOUS);
}

/**
 * Configure power level for transmission
 * @param level 2-17, from least to most power
 */

void sx1278_set_tx_power(int level)
{
   // RF9x module uses PA_BOOST pin
   if (level < 2)
      level = 2;
   else if (level > 17)
      level = 17;
   sx1278_write_reg(REG_PA_CONFIG, PA_BOOST | (level - 2));
}

/**
 * Set carrier frequency.
 * @param frequency Frequency in Hz
 */

void sx1278_set_frequency(long frequency)
{
   __frequency = frequency;

   uint64_t frf = ((uint64_t)frequency << 19) / 32000000;

   sx1278_write_reg(REG_FRF_MSB, (uint8_t)(frf >> 16));
   sx1278_write_reg(REG_FRF_MID, (uint8_t)(frf >> 8));
   sx1278_write_reg(REG_FRF_LSB, (uint8_t)(frf >> 0));
}

/**
 * Set spreading factor.
 * @param sf 6-12, Spreading factor to use.
 */

void sx1278_set_spreading_factor(int sf)
{
   if (sf < 6)
      sf = 6;
   else if (sf > 12)
      sf = 12;

   if (sf == 6)
   {
      sx1278_write_reg(REG_DETECTION_OPTIMIZE, 0xc5);
      sx1278_write_reg(REG_DETECTION_THRESHOLD, 0x0c);
   }
   else
   {
      sx1278_write_reg(REG_DETECTION_OPTIMIZE, 0xc3);
      sx1278_write_reg(REG_DETECTION_THRESHOLD, 0x0a);
   }

   sx1278_write_reg(REG_MODEM_CONFIG_2, (sx1278_read_reg(REG_MODEM_CONFIG_2) & 0x0f) | ((sf << 4) & 0xf0));
}

/**
 * Set bandwidth (bit rate)
 * @param sbw Bandwidth in Hz (up to 500000)
 */

void sx1278_set_bandwidth(long sbw)
{
   int bw;

   if (sbw <= 7.8E3)
      bw = 0;
   else if (sbw <= 10.4E3)
      bw = 1;
   else if (sbw <= 15.6E3)
      bw = 2;
   else if (sbw <= 20.8E3)
      bw = 3;
   else if (sbw <= 31.25E3)
      bw = 4;
   else if (sbw <= 41.7E3)
      bw = 5;
   else if (sbw <= 62.5E3)
      bw = 6;
   else if (sbw <= 125E3)
      bw = 7;
   else if (sbw <= 250E3)
      bw = 8;
   else
      bw = 9;
   sx1278_write_reg(REG_MODEM_CONFIG_1, (sx1278_read_reg(REG_MODEM_CONFIG_1) & 0x0f) | (bw << 4));
}

/**
 * Set coding rate
 * @param denominator 5-8, Denominator for the coding rate 4/x
 */

void sx1278_set_coding_rate(int denominator)
{
   if (denominator < 5)
      denominator = 5;
   else if (denominator > 8)
      denominator = 8;

   int cr = denominator - 4;
   sx1278_write_reg(REG_MODEM_CONFIG_1, (sx1278_read_reg(REG_MODEM_CONFIG_1) & 0xf1) | (cr << 1));
}

/**
 * Set the size of preamble.
 * @param length Preamble length in symbols.
 */

void sx1278_set_preamble_length(long length)
{
   sx1278_write_reg(REG_PREAMBLE_MSB, (uint8_t)(length >> 8));
   sx1278_write_reg(REG_PREAMBLE_LSB, (uint8_t)(length >> 0));
}

/**
 * Change radio sync word.
 * @param sw New sync word to use.
 */

void sx1278_set_sync_word(int sw)
{
   sx1278_write_reg(REG_SYNC_WORD, sw);
}

/**
 * Enable appending/verifying packet CRC.
 */

void sx1278_enable_crc(void)
{
   sx1278_write_reg(REG_MODEM_CONFIG_2, sx1278_read_reg(REG_MODEM_CONFIG_2) | 0x04);
}

/**
 * Disable appending/verifying packet CRC.
 */

void sx1278_disable_crc(void)
{
   sx1278_write_reg(REG_MODEM_CONFIG_2, sx1278_read_reg(REG_MODEM_CONFIG_2) & 0xfb);
}

/*------------------------------------------------------------------*/

/**
 * Perform hardware initialization.
 */
int sx1278_init(void)
{
   esp_err_t ret;

   /*
    * Configure CPU hardware to communicate with the radio chip
    */
   gpio_pad_select_gpio(CONFIG_RST_GPIO);
   gpio_set_direction(CONFIG_RST_GPIO, GPIO_MODE_OUTPUT);
   gpio_pad_select_gpio(CONFIG_CS_GPIO);
   gpio_set_direction(CONFIG_CS_GPIO, GPIO_MODE_OUTPUT);

   spi_bus_config_t bus = {
       .miso_io_num = CONFIG_MISO_GPIO,
       .mosi_io_num = CONFIG_MOSI_GPIO,
       .sclk_io_num = CONFIG_SCK_GPIO,
       .quadwp_io_num = -1,
       .quadhd_io_num = -1,
       .max_transfer_sz = 0};

   ret = spi_bus_initialize(VSPI_HOST, &bus, 0);
   assert(ret == ESP_OK);

   spi_device_interface_config_t dev = {
       .clock_speed_hz = 9000000,
       .mode = 0,
       .spics_io_num = -1,
       .queue_size = 1,
       .flags = 0,
       .pre_cb = NULL};
   ret = spi_bus_add_device(VSPI_HOST, &dev, &__spi);
   assert(ret == ESP_OK);

   /*
    * Perform hardware reset.
    */
   sx1278_reset();

   /*
    * Check version.
    */
   uint8_t version;
   uint8_t i = 0;
   while (i++ < TIMEOUT_RESET)
   {
      version = sx1278_read_reg(REG_VERSION);
      if (version == 0x12)
         break;
      vTaskDelay(2);
   }
   assert(i <= TIMEOUT_RESET + 1); // at the end of the loop above, the max value i can reach is TIMEOUT_RESET + 1

   /*
    * Default configuration.
    */
   sx1278_sleep();
   sx1278_write_reg(REG_FIFO_RX_BASE_ADDR, 0);
   sx1278_write_reg(REG_FIFO_TX_BASE_ADDR, 0);
   sx1278_write_reg(REG_LNA, sx1278_read_reg(REG_LNA) | 0x03);
   sx1278_write_reg(REG_MODEM_CONFIG_3, 0x04);
   sx1278_set_tx_power(17);

   sx1278_idle();
   return 1;
}

/**
 * Send a packet.
 * @param buf Data to be sent
 * @param size Size of data.
 */

void sx1278_send_packet(uint8_t *buf, int size)
{
   /*
    * Transfer data to radio.
    */
   sx1278_idle();
   sx1278_write_reg(REG_FIFO_ADDR_PTR, 0);

   for (int i = 0; i < size; i++)
      sx1278_write_reg(REG_FIFO, *buf++);

   sx1278_write_reg(REG_PAYLOAD_LENGTH, size);

   /*
    * Start transmission and wait for conclusion.
    */
   sx1278_write_reg(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_TX);
   while ((sx1278_read_reg(REG_IRQ_FLAGS) & IRQ_TX_DONE_MASK) == 0)
      vTaskDelay(2);

   sx1278_write_reg(REG_IRQ_FLAGS, IRQ_TX_DONE_MASK);
}

/**
 * Read a received packet.
 * @param buf Buffer for the data.
 * @param size Available size in buffer (bytes).
 * @return Number of bytes received (zero if no packet available).
 */

int sx1278_receive_packet(uint8_t *buf, int size)
{
   int len = 0;

   /*
    * Check interrupts.
    */
   int irq = sx1278_read_reg(REG_IRQ_FLAGS);
   sx1278_write_reg(REG_IRQ_FLAGS, irq);
   if ((irq & IRQ_RX_DONE_MASK) == 0)
      return 0;
   if (irq & IRQ_PAYLOAD_CRC_ERROR_MASK)
      return 0;

   /*
    * Find packet size.
    */
   if (__implicit)
      len = sx1278_read_reg(REG_PAYLOAD_LENGTH);
   else
      len = sx1278_read_reg(REG_RX_NB_BYTES);

   /*
    * Transfer data from radio.
    */
   sx1278_idle();
   sx1278_write_reg(REG_FIFO_ADDR_PTR, sx1278_read_reg(REG_FIFO_RX_CURRENT_ADDR));
   if (len > size)
      len = size;
   for (int i = 0; i < len; i++)
      *buf++ = sx1278_read_reg(REG_FIFO);

   return len;
}

/**
 * Returns non-zero if there is data to read (packet received).
 * using to check REG_IRQ_FLAGS
 */

int sx1278_received(void)
{
   if (sx1278_read_reg(REG_IRQ_FLAGS) & IRQ_RX_DONE_MASK)
      return 1;
   return 0;
}

/**
 * Return last packet's RSSI.
 */

int sx1278_packet_rssi(void)
{
   return (sx1278_read_reg(REG_PKT_RSSI_VALUE) - (__frequency < 868E6 ? 164 : 157));
}

/**
 * Return last packet's SNR (signal to noise ratio).
 */

float sx1278_packet_snr(void)
{
   return ((int8_t)sx1278_read_reg(REG_PKT_SNR_VALUE)) * 0.25;
}

/**
 * Shutdown hardware.
 */

void sx1278_close(void)
{
   sx1278_sleep();
}

/*test module sx1278*/

void sx1278_dump_registers(void)
{
   int i;
   printf("00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n");
   for (i = 0; i < 0x40; i++)
   {
      printf("%02X ", sx1278_read_reg(i));
      if ((i & 0x0f) == 0x0f)
         printf("\n");
   }
   printf("\n");
}
