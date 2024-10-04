/*
 * board.h - Board definitions
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

/*
 * @@@ This is for an M0P Dock with the following additional devices:
 * - Display module (TFT controller on SPI, touch panel controller on I2C)
 * - BMA400 acceleration sensor on I2C
 * - ATECC608 Secure Element on I2C
 */

#ifndef BOARD_H
#define	BOARD_H

/* --- LED ----------------------------------------------------------------- */

#define LED		8	/* IO8_PWM_LED */

/* --- Buttons ------------------------------------------------------------- */

/* Left and right if USB is "down" and the display is facing upward */

#define	BUTTON_R	3	/* IO3_Button1 */
#define	BUTTON_L	3	/* IO3_Button2 */

/* --- Display module: TFT ------------------------------------------------- */

#define LCD_CS		17	/* IO17_LCD_DBI_CS */
#define LCD_DnC		18	/* IO18_LCD_DBI_DC */
#define LCD_MOSI	19	/* IO29_LCD_DBI_SDA */
#define LCD_SCLK	9	/* IO9_LCD_DBI_SCK */

#define LCD_RST		27	/* IO27_LCD_RESET */

#define LCD_SPI		0

#define LCD_WIDTH	240
#define LCD_HEIGHT	280

/* --- M1s I2C ------------------------------------------------------------- */

#define I2C0_SDA	1	/* IO1_CAM_I2C0_SDA */
#define I2C0_SCL	0	/* IO0_CAM_I2C0_SDA */

/* --- Display module: backlight ------------------------------------------- */

#define LCD_BL		16	/* IO16_LCD_BL_PWM */
#define LCD_BL_OFF	1
#define LCD_BL_ON	0

/* --- Display module: touch screen ---------------------------------------- */

#define TOUCH_INT	34	/* IO34_TP_TINT */

#define TOUCH_I2C	0
#define TOUCH_I2C_ADDR	0x15

/* --- BMA400 -------------------------------------------------------------- */

#define ACCEL_INT	33	/* IO33 */

#define ACCEL_I2C	0
#define ACCEL_ADDR	0x14

/* --- ATECC608 ------------------------------------------------------------ */

#define ATECC_I2C	0
#define ATECC_ADDR	0x60

#endif /* !BOARD_H */
