#include "ssd1306.h"
#include "font.h"

/**
 * @brief Initialize the SSD1306 display.
 * 
 * @param ssd Pointer to the SSD1306 structure.
 * @param width Width of the display.
 * @param height Height of the display.
 * @param external_vcc Use external VCC.
 * @param address I2C address of the display.
 * @param i2c Pointer to the I2C instance.
 */
void ssd1306_init(ssd1306_t *ssd, uint8_t width, uint8_t height, bool external_vcc, uint8_t address, i2c_inst_t *i2c) {
  ssd->width = width;
  ssd->height = height;
  ssd->pages = height / 8U;
  ssd->address = address;
  ssd->i2c_port = i2c;
  ssd->bufsize = ssd->pages * ssd->width + 1;
  ssd->ram_buffer = calloc(ssd->bufsize, sizeof(uint8_t));
  ssd->ram_buffer[0] = 0x40;
  ssd->port_buffer[0] = 0x80;
}

/**
 * @brief Configure the SSD1306 display.
 * 
 * @param ssd Pointer to the SSD1306 structure.
 */
void ssd1306_config(ssd1306_t *ssd) {
  ssd1306_command(ssd, SET_DISP | 0x00);
  ssd1306_command(ssd, SET_MEM_ADDR);
  ssd1306_command(ssd, 0x01);
  ssd1306_command(ssd, SET_DISP_START_LINE | 0x00);
  ssd1306_command(ssd, SET_SEG_REMAP | 0x01);
  ssd1306_command(ssd, SET_MUX_RATIO);
  ssd1306_command(ssd, HEIGHT - 1);
  ssd1306_command(ssd, SET_COM_OUT_DIR | 0x08);
  ssd1306_command(ssd, SET_DISP_OFFSET);
  ssd1306_command(ssd, 0x00);
  ssd1306_command(ssd, SET_COM_PIN_CFG);
  ssd1306_command(ssd, 0x12);
  ssd1306_command(ssd, SET_DISP_CLK_DIV);
  ssd1306_command(ssd, 0x80);
  ssd1306_command(ssd, SET_PRECHARGE);
  ssd1306_command(ssd, 0xF1);
  ssd1306_command(ssd, SET_VCOM_DESEL);
  ssd1306_command(ssd, 0x30);
  ssd1306_command(ssd, SET_CONTRAST);
  ssd1306_command(ssd, 0xFF);
  ssd1306_command(ssd, SET_ENTIRE_ON);
  ssd1306_command(ssd, SET_NORM_INV);
  ssd1306_command(ssd, SET_CHARGE_PUMP);
  ssd1306_command(ssd, 0x14);
  ssd1306_command(ssd, SET_DISP | 0x01);
}

/**
 * @brief Send a command to the SSD1306 display.
 * 
 * @param ssd Pointer to the SSD1306 structure.
 * @param command Command to send.
 */
void ssd1306_command(ssd1306_t *ssd, uint8_t command) {
  ssd->port_buffer[1] = command;
  i2c_write_blocking(
    ssd->i2c_port,
    ssd->address,
    ssd->port_buffer,
    2,
    false
  );
}

/**
 * @brief Send data to the SSD1306 display.
 * 
 * @param ssd Pointer to the SSD1306 structure.
 */
void ssd1306_send_data(ssd1306_t *ssd) {
  ssd1306_command(ssd, SET_COL_ADDR);
  ssd1306_command(ssd, 0);
  ssd1306_command(ssd, ssd->width - 1);
  ssd1306_command(ssd, SET_PAGE_ADDR);
  ssd1306_command(ssd, 0);
  ssd1306_command(ssd, ssd->pages - 1);
  i2c_write_blocking(
    ssd->i2c_port,
    ssd->address,
    ssd->ram_buffer,
    ssd->bufsize,
    false
  );
}

/**
 * @brief Draw a pixel on the SSD1306 display.
 * 
 * @param ssd Pointer to the SSD1306 structure.
 * @param x X coordinate of the pixel.
 * @param y Y coordinate of the pixel.
 * @param value Pixel value (true for on, false for off).
 */
void ssd1306_pixel(ssd1306_t *ssd, uint8_t x, uint8_t y, bool value) {
  uint16_t index = (y >> 3) + (x << 3) + 1;
  uint8_t pixel = (y & 0b111);
  if (value)
    ssd->ram_buffer[index] |= (1 << pixel);
  else
    ssd->ram_buffer[index] &= ~(1 << pixel);
}

/**
 * @brief Fill the SSD1306 display with a value.
 * 
 * @param ssd Pointer to the SSD1306 structure.
 * @param value Fill value (true for on, false for off).
 */
void ssd1306_fill(ssd1306_t *ssd, bool value) {
    // Itera por todas as posições do display
    for (uint8_t y = 0; y < ssd->height; ++y) {
        for (uint8_t x = 0; x < ssd->width; ++x) {
            ssd1306_pixel(ssd, x, y, value);
        }
    }
}

/**
 * @brief Draw a rectangle on the SSD1306 display.
 * 
 * @param ssd Pointer to the SSD1306 structure.
 * @param top Top coordinate of the rectangle.
 * @param left Left coordinate of the rectangle.
 * @param width Width of the rectangle.
 * @param height Height of the rectangle.
 * @param value Pixel value (true for on, false for off).
 * @param fill Fill the rectangle (true for fill, false for outline).
 */
void ssd1306_rect(ssd1306_t *ssd, uint8_t top, uint8_t left, uint8_t width, uint8_t height, bool value, bool fill) {
  for (uint8_t x = left; x < left + width; ++x) {
    ssd1306_pixel(ssd, x, top, value);
    ssd1306_pixel(ssd, x, top + height - 1, value);
  }
  for (uint8_t y = top; y < top + height; ++y) {
    ssd1306_pixel(ssd, left, y, value);
    ssd1306_pixel(ssd, left + width - 1, y, value);
  }

  if (fill) {
    for (uint8_t x = left + 1; x < left + width - 1; ++x) {
      for (uint8_t y = top + 1; y < top + height - 1; ++y) {
        ssd1306_pixel(ssd, x, y, value);
      }
    }
  }
}

/**
 * @brief Draw a character on the SSD1306 display.
 * 
 * @param ssd Pointer to the SSD1306 structure.
 * @param c Character to draw.
 * @param x X coordinate of the character.
 * @param y Y coordinate of the character.
 */
void ssd1306_draw_char(ssd1306_t *ssd, char c, uint8_t x, uint8_t y)
{
    uint16_t index = 0;

    if (c >= '0' && c <= '9') {
        index = (c - '0' + 1) * 8; // Deslocamento para números
    } else if (c >= 'A' && c <= 'Z') {
        index = (c - 'A' + 11) * 8; // Deslocamento para letras maiúsculas
    } else if (c >= 'a' && c <= 'z') {
        index = (c - 'a' + 37) * 8; // Adiciona suporte para minúsculas
    } else {
        return; // Caracter não suportado, não faz nada
    }

    for (uint8_t i = 0; i < 8; ++i) {
        uint8_t line = font[index + i];
        for (uint8_t j = 0; j < 8; j++) {
            ssd1306_pixel(ssd, x + i, y + j, (line >> j) & 1);
        }
    }
}

/**
 * @brief Draw a string on the SSD1306 display.
 * 
 * @param ssd Pointer to the SSD1306 structure.
 * @param str String to draw.
 * @param x X coordinate of the string.
 * @param y Y coordinate of the string.
 */
void ssd1306_draw_string(ssd1306_t *ssd, const char *str, uint8_t x, uint8_t y)
{
  while (*str)
  {
    ssd1306_draw_char(ssd, *str++, x, y);
    x += 8;
    if (x + 8 >= ssd->width)
    {
      x = 0;
      y += 8;
    }
    if (y + 8 >= ssd->height)
    {
      break;
    }
  }
}