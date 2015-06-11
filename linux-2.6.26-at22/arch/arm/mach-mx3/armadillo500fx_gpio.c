#include <linux/errno.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/hardware.h>
#include <asm/arch/clock.h>
#include <asm/arch/gpio.h>
#include "iomux.h"

typedef enum {
	GPIO_INITIALIZE = -1,
	GPIO_INACTIVE = 0,
	GPIO_ACTIVE = 1,
} gpio_state_t;

typedef struct {
	u32	pin;

	u8	active_output;
	u8	active_input;
	u8	inactive_output;
	u8	inactive_input;

	u16	active_pad;
	u16	inactive_pad;
} pin_info_t;

#define O_GPIO (OUTPUTCONFIG_GPIO)
#define O_FUNC (OUTPUTCONFIG_FUNC)
#define O_ALT1 (OUTPUTCONFIG_ALT1)
#define O_ALT2 (OUTPUTCONFIG_ALT2)
#define O_ALT3 (OUTPUTCONFIG_ALT3)
#define O_ALT4 (OUTPUTCONFIG_ALT4)
#define O_ALT5 (OUTPUTCONFIG_ALT5)
#define O_ALT6 (OUTPUTCONFIG_ALT6)
#define O_GPR  (O_GPIO)

#define I_NONE (INPUTCONFIG_NONE)
#define I_GPIO (INPUTCONFIG_GPIO)
#define I_FUNC (INPUTCONFIG_FUNC)
#define I_ALT1 (INPUTCONFIG_ALT1)
#define I_ALT2 (INPUTCONFIG_ALT2)
#define I_GPR  (I_NONE)

#define P_NONE (0xffff)

static gpio_state_t pin_uart1_state = GPIO_INITIALIZE;
static pin_info_t pin_uart1[] = {
  {MX31_PIN_RXD1,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_TXD1,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
};

static gpio_state_t pin_uart2_state = GPIO_INITIALIZE;
static pin_info_t pin_uart2[] = {
  {MX31_PIN_TXD2,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_RXD2,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_RTS2,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_CTS2,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_DTR_DTE1,	O_ALT3, I_GPIO, O_ALT3, I_GPIO, P_NONE, P_NONE},
  {MX31_PIN_RI_DTE1,	O_ALT1, I_ALT1, O_ALT1, I_ALT1, P_NONE, P_NONE},
  {MX31_PIN_DCD_DTE1,	O_ALT1, I_ALT1, O_ALT1, I_ALT1, P_NONE, P_NONE},
};

static gpio_state_t pin_uart5_state = GPIO_INITIALIZE;
static pin_info_t pin_uart5[] = {
  {MX31_PIN_PC_VS2,	O_ALT2, I_ALT2, O_ALT2, I_ALT2, P_NONE, P_NONE},
  {MX31_PIN_PC_BVD1,	O_ALT2, I_ALT2, O_ALT2, I_ALT2, P_NONE, P_NONE},
  {MX31_PIN_PC_BVD2,	O_ALT2, I_ALT2, O_ALT2, I_ALT2, P_NONE, P_NONE},
  {MX31_PIN_PC_RST,	O_ALT2, I_ALT2, O_ALT2, I_ALT2, P_NONE, P_NONE},
};

#define P_I2C (0x108)
static gpio_state_t pin_i2c1_state = GPIO_INITIALIZE;
static pin_info_t pin_i2c1[] = {
  {MX31_PIN_I2C_CLK,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_I2C, P_I2C},
  {MX31_PIN_I2C_DAT,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_I2C, P_I2C},
};

static gpio_state_t pin_i2c2_state = GPIO_INITIALIZE;
static pin_info_t pin_i2c2[] = {
  {MX31_PIN_CSPI2_MOSI,	O_ALT1, I_ALT1, O_GPIO, I_ALT1, P_I2C, P_I2C},
  {MX31_PIN_CSPI2_MISO,	O_ALT1, I_ALT1, O_GPIO, I_ALT1, P_I2C, P_I2C},
};

//#define P_SDHC (PAD_CTL_DRV_MAX | PAD_CTL_SRE_FAST)
#define P_SDHC (PAD_CTL_SRE_FAST)
static gpio_state_t pin_sdhc1_state = GPIO_INITIALIZE;
static pin_info_t pin_sdhc1[] = {
  {MX31_PIN_SD1_CMD,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_SDHC, P_SDHC},  
  {MX31_PIN_SD1_CLK,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_SDHC, P_SDHC},
  {MX31_PIN_SD1_DATA0,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_SDHC, P_SDHC},
  {MX31_PIN_SD1_DATA1,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_SDHC, P_SDHC},
  {MX31_PIN_SD1_DATA2,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_SDHC, P_SDHC},
  {MX31_PIN_SD1_DATA3,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_SDHC, P_SDHC},
  {MX31_PIN_ATA_DMACK,	O_GPIO, I_GPIO, O_GPIO, I_GPIO, P_NONE, P_NONE},
};

#define P_SDHC_IRQ (PAD_CTL_100K_PU | PAD_CTL_PUE_PUD | PAD_CTL_PKE_ENABLE)
static gpio_state_t pin_sdhc2_state = GPIO_INITIALIZE;
static pin_info_t pin_sdhc2[] = {
  {MX31_PIN_PC_CD1_B,   O_ALT1, I_ALT1, O_ALT1, I_ALT1, P_SDHC, P_SDHC},
  {MX31_PIN_PC_CD2_B,   O_ALT1, I_ALT1, O_ALT1, I_ALT1, P_SDHC, P_SDHC},
  {MX31_PIN_PC_WAIT_B,  O_ALT1, I_ALT1, O_ALT1, I_ALT1, P_SDHC, P_SDHC},
  {MX31_PIN_PC_READY,   O_ALT1, I_ALT1, O_ALT1, I_ALT1, P_SDHC, P_SDHC},
  {MX31_PIN_PC_PWRON,   O_ALT1, I_ALT1, O_ALT1, I_ALT1, P_SDHC, P_SDHC},
  {MX31_PIN_PC_VS1,     O_ALT1, I_ALT1, O_ALT1, I_ALT1, P_SDHC, P_SDHC},
  {MX31_PIN_CSI_D4,	O_GPIO, I_GPIO, O_GPIO, I_GPIO, P_SDHC_IRQ, P_SDHC_IRQ},
};

static gpio_state_t pin_lcd_state = GPIO_INITIALIZE;
static pin_info_t pin_lcd[] = {
  {MX31_PIN_LD0,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_LD1,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_LD2,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_LD3,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_LD4,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_LD5,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_LD6,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_LD7,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_LD8,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_LD9,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_LD10,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_LD11,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_LD12,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_LD13,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_LD14,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_LD15,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_LD16,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_LD17,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_HSYNC,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_FPSHIFT,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_DRDY0,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_VSYNC3,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},

  {MX31_PIN_CONTRAST,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
};

#define P_CSI_GPIO (PAD_CTL_100K_PU | PAD_CTL_PUE_PUD | PAD_CTL_PKE_ENABLE)
static gpio_state_t pin_csi_state = GPIO_INITIALIZE;
static pin_info_t pin_csi[] = {
  {MX31_PIN_CSI_D8,	O_FUNC, I_FUNC, O_GPIO, I_GPIO, P_NONE, P_CSI_GPIO},
  {MX31_PIN_CSI_D9,	O_FUNC, I_FUNC, O_GPIO, I_GPIO, P_NONE, P_CSI_GPIO},
  {MX31_PIN_CSI_D10,	O_FUNC, I_FUNC, O_GPIO, I_GPIO, P_NONE, P_CSI_GPIO},
  {MX31_PIN_CSI_D11,	O_FUNC, I_FUNC, O_GPIO, I_GPIO, P_NONE, P_CSI_GPIO},
  {MX31_PIN_CSI_D12,	O_FUNC, I_FUNC, O_GPIO, I_GPIO, P_NONE, P_CSI_GPIO},
  {MX31_PIN_CSI_D13,	O_FUNC, I_FUNC, O_GPIO, I_GPIO, P_NONE, P_CSI_GPIO},
  {MX31_PIN_CSI_D14,	O_FUNC, I_FUNC, O_GPIO, I_GPIO, P_NONE, P_CSI_GPIO},
  {MX31_PIN_CSI_D15,	O_FUNC, I_FUNC, O_GPIO, I_GPIO, P_NONE, P_CSI_GPIO},
  {MX31_PIN_CSI_MCLK,	O_FUNC, I_FUNC, O_GPIO, I_GPIO, P_NONE, P_CSI_GPIO},
  {MX31_PIN_CSI_VSYNC,	O_FUNC, I_FUNC, O_GPIO, I_GPIO, P_NONE, P_CSI_GPIO},
  {MX31_PIN_CSI_HSYNC,	O_FUNC, I_FUNC, O_GPIO, I_GPIO, P_NONE, P_CSI_GPIO},
  {MX31_PIN_CSI_PIXCLK,	O_FUNC, I_FUNC, O_GPIO, I_GPIO, P_NONE, P_CSI_GPIO},
  {MX31_PIN_GPIO3_0,	O_GPIO, I_GPIO, O_GPIO, I_GPIO, P_NONE, P_NONE},
  {MX31_PIN_GPIO3_1,	O_GPIO, I_GPIO, O_GPIO, I_GPIO, P_NONE, P_NONE},
};

#define P_USBH2 (PAD_CTL_SRE_FAST | PAD_CTL_DRV_MAX |\
		 PAD_CTL_100K_PU | PAD_CTL_PUE_PUD | PAD_CTL_PKE_NONE)

static gpio_state_t pin_usbh2_state = GPIO_INITIALIZE;
static pin_info_t pin_usbh2[] = {
  {MX31_PIN_USBH2_CLK,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_USBH2, P_USBH2},
  {MX31_PIN_USBH2_DIR,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_USBH2, P_USBH2},
  {MX31_PIN_USBH2_STP,	O_FUNC, I_FUNC, O_FUNC, I_NONE, 0x1a1, 0x1a1},
  {MX31_PIN_USBH2_NXT,	O_FUNC, I_FUNC, O_FUNC, I_NONE, P_USBH2, P_USBH2},
  {MX31_PIN_USBH2_DATA0,O_FUNC, I_FUNC, O_GPIO, I_NONE, P_USBH2, P_USBH2},
  {MX31_PIN_USBH2_DATA1,O_FUNC, I_FUNC, O_GPIO, I_NONE, P_USBH2, P_USBH2},
  {MX31_PIN_STXD3,	O_GPR,  I_GPR,  O_GPR,  I_GPR,  P_USBH2, P_USBH2},
  {MX31_PIN_SRXD3,	O_GPR,  I_GPR,  O_GPR,  I_GPR,  P_USBH2, P_USBH2},
  {MX31_PIN_SCK3,	O_GPR,  I_GPR,  O_GPR,  I_GPR,  P_USBH2, P_USBH2},
  {MX31_PIN_SFS3,	O_GPR,  I_GPR,  O_GPR,  I_GPR,  P_USBH2, P_USBH2},
  {MX31_PIN_STXD6,	O_GPR,  I_GPR,  O_GPR,  I_GPR,  P_USBH2, P_USBH2},
  {MX31_PIN_SRXD6,	O_GPR,  I_GPR,  O_GPR,  I_GPR,  P_USBH2, P_USBH2},

  /* CS */
  {MX31_PIN_GPIO1_3,	O_GPIO,	I_GPIO, O_GPIO, I_GPIO, P_USBH2, P_USBH2},
  /* RESET */
  {MX31_PIN_SCK6,	O_GPIO, I_GPIO, O_GPIO, I_GPIO, P_USBH2, P_USBH2},
};

#define P_USBOTG (PAD_CTL_SRE_FAST | PAD_CTL_DRV_MAX |\
		 PAD_CTL_100K_PU | PAD_CTL_PUE_PUD | PAD_CTL_PKE_ENABLE)
static gpio_state_t pin_usbotg_state = GPIO_INITIALIZE;
static pin_info_t pin_usbotg[] = {
  {MX31_PIN_USBOTG_CLK,	  O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_USBOTG, P_USBOTG},
  {MX31_PIN_USBOTG_DIR,	  O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_USBOTG, P_USBOTG},
  {MX31_PIN_USBOTG_STP,	  O_FUNC, I_FUNC, O_FUNC, I_FUNC, 0x1a1, 0x1a1},
  {MX31_PIN_USBOTG_NXT,	  O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_USBOTG, P_USBOTG},
  {MX31_PIN_USBOTG_DATA0, O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_USBOTG, P_USBOTG},
  {MX31_PIN_USBOTG_DATA1, O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_USBOTG, P_USBOTG},
  {MX31_PIN_USBOTG_DATA2, O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_USBOTG, P_USBOTG},
  {MX31_PIN_USBOTG_DATA3, O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_USBOTG, P_USBOTG},
  {MX31_PIN_USBOTG_DATA4, O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_USBOTG, P_USBOTG},
  {MX31_PIN_USBOTG_DATA5, O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_USBOTG, P_USBOTG},
  {MX31_PIN_USBOTG_DATA6, O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_USBOTG, P_USBOTG},
  {MX31_PIN_USBOTG_DATA7, O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_USBOTG, P_USBOTG},

  /* CS */
  {MX31_PIN_ATA_CS1,	  O_GPIO, I_GPIO, O_GPIO, I_GPIO, P_USBOTG, P_USBOTG},
  /* RESET */
  {MX31_PIN_SRST0,	  O_GPIO, I_GPIO, O_GPIO, I_GPIO, P_USBOTG, P_USBOTG},
};

#define P_USBH1 (PAD_CTL_SRE_FAST | PAD_CTL_DRV_MAX)
static gpio_state_t pin_usbh1_state = GPIO_INITIALIZE;
static pin_info_t pin_usbh1[] = {
  {MX31_PIN_CSPI1_MOSI,    O_ALT1, I_ALT1, O_ALT1, I_ALT1, P_USBH1, P_USBH1},
  {MX31_PIN_CSPI1_MISO,    O_ALT1, I_ALT1, O_ALT1, I_ALT1, P_USBH1, P_USBH1},
  {MX31_PIN_CSPI1_SS0,     O_ALT1, I_ALT1, O_ALT1, I_ALT1, P_USBH1, P_USBH1},
  {MX31_PIN_CSPI1_SS1,     O_ALT1, I_ALT1, O_ALT1, I_ALT1, P_USBH1, P_USBH1},
  {MX31_PIN_CSPI1_SS2,     O_ALT1, I_ALT1, O_ALT1, I_ALT1, P_USBH1, P_USBH1},
  {MX31_PIN_CSPI1_SCLK,    O_ALT1, I_ALT1, O_ALT1, I_ALT1, P_USBH1, P_USBH1},
  {MX31_PIN_CSPI1_SPI_RDY, O_ALT1, I_ALT1, O_ALT1, I_ALT1, P_USBH1, P_USBH1},
};

static gpio_state_t pin_keypad_state = GPIO_INITIALIZE;
static pin_info_t pin_keypad[] = {
  {MX31_PIN_KEY_ROW3,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_KEY_ROW4,	O_FUNC, I_FUNC, O_GPIO, I_GPIO, P_NONE, P_NONE},
  {MX31_PIN_KEY_ROW5,	O_FUNC, I_FUNC, O_GPIO, I_GPIO, P_NONE, P_NONE},
  {MX31_PIN_KEY_ROW6,	O_FUNC, I_FUNC, O_GPIO, I_GPIO, P_NONE, P_NONE},
  {MX31_PIN_KEY_ROW7,	O_FUNC, I_FUNC, O_GPIO, I_GPIO, P_NONE, P_NONE},
  {MX31_PIN_KEY_COL2,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_KEY_COL3,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_KEY_COL4,	O_FUNC, I_FUNC, O_GPIO, I_GPIO, P_NONE, P_NONE},
  {MX31_PIN_KEY_COL5,	O_FUNC, I_FUNC, O_GPIO, I_GPIO, P_NONE, P_NONE},
  {MX31_PIN_KEY_COL6,	O_FUNC, I_FUNC, O_GPIO, I_GPIO, P_NONE, P_NONE},
  {MX31_PIN_KEY_COL7,	O_FUNC, I_FUNC, O_GPIO, I_GPIO, P_NONE, P_NONE},
};

static gpio_state_t pin_jtag_state = GPIO_INITIALIZE;
static pin_info_t pin_jtag[] = {
  {MX31_PIN_RTCK,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},  
  {MX31_PIN_TCK,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},  
  {MX31_PIN_TMS,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},  
  {MX31_PIN_TDI,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},  
  {MX31_PIN_TDO,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},  
  {MX31_PIN_TRSTB,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},  
  {MX31_PIN_DE_B,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},  
};

static gpio_state_t pin_led_id_state = GPIO_INITIALIZE;
static pin_info_t pin_led_id[] = {
  {MX31_PIN_SIMPD0,	O_GPIO, I_GPIO, O_GPIO, I_GPIO, P_NONE, P_NONE},
};

static gpio_state_t pin_bus_state = GPIO_INITIALIZE;
static pin_info_t pin_bus[] = {
  {MX31_PIN_A0,		O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_A1,		O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_A2,		O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_A3,		O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_A4,		O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_A5,		O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_A6,		O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},

  {MX31_PIN_D0,		O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_D1,		O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_D2,		O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_D3,		O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_D4,		O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_D5,		O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_D6,		O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_D7,		O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_D8,		O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_D9,		O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_D10,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_D11,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_D12,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_D13,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_D14,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_D15,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},

  {MX31_PIN_OE,		O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_RW,		O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
};

static gpio_state_t pin_cspi3_state = GPIO_INITIALIZE;
static pin_info_t pin_cspi3[] = {
  {MX31_PIN_CSPI3_MOSI,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_CSPI3_MISO,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_CSPI3_SCLK,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_CSPI2_SS0,	O_ALT1, I_ALT1, O_ALT1, I_ALT1, P_NONE, P_NONE},
  {MX31_PIN_CSPI2_SS1,	O_ALT1, I_ALT1, O_ALT1, I_ALT1, P_NONE, P_NONE},
};

static gpio_state_t pin_audio4_state = GPIO_INITIALIZE;
static pin_info_t pin_audio4[] = {
  {MX31_PIN_STXD4,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_SRXD4,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_SCK4,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_SFS4,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
};

static gpio_state_t pin_audio5_state = GPIO_INITIALIZE;
static pin_info_t pin_audio5[] = {
  {MX31_PIN_STXD5,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_SRXD5,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_SCK5,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
  {MX31_PIN_SFS5,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},
};

#define P_RTC_IRQ	(PAD_CTL_100K_PU | PAD_CTL_PUE_PUD | PAD_CTL_PKE_ENABLE)
#define P_WSIM		(PAD_CTL_PKE_NONE)
static gpio_state_t pin_etc_state = GPIO_INITIALIZE;
static pin_info_t pin_etc[] = {
  /* CS3 */
  {MX31_PIN_CS3,	O_FUNC, I_FUNC, O_FUNC, I_FUNC, P_NONE, P_NONE},

  /* LAN: IRQ */
  {MX31_PIN_GPIO1_0,	O_GPIO, I_GPIO, O_GPIO, I_GPIO, P_NONE, P_NONE},
  /* LAN: PME */
  {MX31_PIN_GPIO1_1,	O_GPIO, I_GPIO, O_GPIO, I_GPIO, P_NONE, P_NONE},
  /* LAN: AMDIX EN */
  {MX31_PIN_ATA_CS0,	O_GPIO, I_GPIO, O_GPIO, I_GPIO, P_NONE, P_NONE},

  /* RTC_INT */
  {MX31_PIN_CSI_D5,	O_GPIO, I_GPIO, O_GPIO, I_GPIO, P_RTC_IRQ, P_RTC_IRQ},

  /* W-SIM */
  {MX31_PIN_DSR_DTE1,	O_GPIO, I_GPIO, O_GPIO, I_GPIO, P_NONE, P_NONE},
  {MX31_PIN_SVEN0,	O_GPIO, I_GPIO, O_GPIO, I_GPIO, P_WSIM, P_WSIM},
  {MX31_PIN_STX0,	O_GPIO, I_GPIO, O_GPIO, I_GPIO, P_WSIM, P_WSIM},
  {MX31_PIN_SRX0,	O_GPIO, I_GPIO, O_GPIO, I_GPIO, P_WSIM, P_WSIM},
  {MX31_PIN_ATA_DIOW,	O_GPIO, I_GPIO, O_GPIO, I_GPIO, P_NONE, P_NONE},

  /* jumpers */
  {MX31_PIN_COMPARE,	O_GPIO, I_GPIO, O_GPIO, I_GPIO, P_NONE, P_NONE},
  {MX31_PIN_CAPTURE,	O_GPIO, I_GPIO, O_GPIO, I_GPIO, P_NONE, P_NONE},

  /* touchscreen */
  {MX31_PIN_SCLK0,	O_GPIO, I_GPIO, O_GPIO, I_GPIO, P_NONE, P_NONE},

  /* one wire */
  {MX31_PIN_BATT_LINE,  O_FUNC, I_FUNC, O_GPIO, I_GPIO, P_NONE, P_NONE},
};

/*****************************************************************************
 * 
 ****************************************************************************/
static void
gpio_change_state(pin_info_t *info, int nr_pin,
		  gpio_state_t *cur_state, gpio_state_t set_state)
{
	int i;

	if (info == NULL) return;
	if (*cur_state == set_state) return;

	switch (*cur_state) {
	case GPIO_ACTIVE:
		for (i=0; i<nr_pin; i++) {
			mxc_free_iomux(info[i].pin,
				       info[i].active_output,
				       info[i].active_input);
		}
		break;
	case GPIO_INACTIVE:
		for (i=0; i<nr_pin; i++) {
			mxc_free_iomux(info[i].pin,
				       info[i].inactive_output,
				       info[i].inactive_input);
		}
		break;
	case GPIO_INITIALIZE:
	default:
		break;
	}

	switch (set_state) {
	case GPIO_ACTIVE:
		for (i=0; i<nr_pin; i++) {
			mxc_request_iomux(info[i].pin,
					  info[i].active_output,
					  info[i].active_input);
			if (info[i].active_pad != P_NONE)
				mxc_iomux_set_pad(info[i].pin,
						  info[i].active_pad);
		}
		break;
	case GPIO_INACTIVE:
	default:
		for (i=0; i<nr_pin; i++) {
			mxc_request_iomux(info[i].pin,
					  info[i].inactive_output,
					  info[i].inactive_input);
			if (info[i].inactive_pad != P_NONE)
				mxc_iomux_set_pad(info[i].pin,
						  info[i].inactive_pad);
		}
		break;
	};

	*cur_state = set_state;
}

/*****************************************************************************
 * UART
 ****************************************************************************/
void gpio_uart_active(int port, int no_irda)
{
	/*
	 * Configure the IOMUX control registers for the UART signals
	 */
	switch (port) {
		/* UART 1 IOMUX Configs */
	case 0:
		gpio_change_state(pin_uart1, ARRAY_SIZE(pin_uart1),
				  &pin_uart1_state, GPIO_ACTIVE);
		break;
		/* UART 2 IOMUX Configs */
	case 1:
		gpio_change_state(pin_uart2, ARRAY_SIZE(pin_uart2),
				  &pin_uart2_state, GPIO_ACTIVE);
		break;
		/* UART 3 IOMUX Configs */
	case 2:
		break;
		/* UART 4 IOMUX Configs */
	case 3:
		break;
		/* UART 5 IOMUX Configs */
	case 4:
		gpio_change_state(pin_uart5, ARRAY_SIZE(pin_uart5),
				  &pin_uart5_state, GPIO_ACTIVE);
		break;
	default:
		break;
	}
}
EXPORT_SYMBOL(gpio_uart_active);

void gpio_uart_inactive(int port, int no_irda)
{
	switch (port) {
	case 0:
		gpio_change_state(pin_uart1, ARRAY_SIZE(pin_uart1),
				  &pin_uart1_state, GPIO_INACTIVE);
		break;
	case 1:
		gpio_change_state(pin_uart2, ARRAY_SIZE(pin_uart2),
				  &pin_uart2_state, GPIO_INACTIVE);
		break;
	case 2:
		break;
	case 3:
		break;
	case 4:
		gpio_change_state(pin_uart5, ARRAY_SIZE(pin_uart5),
				  &pin_uart5_state, GPIO_INACTIVE);
		break;
	default:
		break;
	}
}
EXPORT_SYMBOL(gpio_uart_inactive);

void config_uartdma_event(int port)
{
	switch (port) {
	case 1:
		/* Configure to receive UART 2 SDMA events */
		mxc_iomux_set_gpr(MUX_PGP_FIRI, false);
		break;
	case 2:
		/* Configure to receive UART 3 SDMA events */
		mxc_iomux_set_gpr(MUX_CSPI1_UART3, true);
		break;
	case 4:
		/* Configure to receive UART 5 SDMA events */
		mxc_iomux_set_gpr(MUX_CSPI3_UART5_SEL, true);
		break;
	default:
		break;
	}
}
EXPORT_SYMBOL(config_uartdma_event);

/*****************************************************************************
 * Keypad
 ****************************************************************************/
void gpio_keypad_active(void)
{
	gpio_change_state(pin_keypad, ARRAY_SIZE(pin_keypad),
			  &pin_keypad_state, GPIO_ACTIVE);
}
EXPORT_SYMBOL(gpio_keypad_active);

void gpio_keypad_inactive(void)
{
	gpio_change_state(pin_keypad, ARRAY_SIZE(pin_keypad),
			  &pin_keypad_state, GPIO_INACTIVE);
}
EXPORT_SYMBOL(gpio_keypad_inactive);

/*****************************************************************************
 * CSPI
 ****************************************************************************/
void gpio_spi_active(int select)
{
	switch (select) {
	case 0: /* SPI1 */
		break;
	case 1: /* SPI2 */
		break;
	case 2: /* SPI3 */
		gpio_change_state(pin_cspi3, ARRAY_SIZE(pin_cspi3),
				  &pin_cspi3_state, GPIO_ACTIVE);
		break;
	default:
		break;
	}
}
EXPORT_SYMBOL(gpio_spi_active);

void gpio_spi_inactive(int select)
{
	switch (select) {
	case 0: /* SPI1 */
		break;
	case 1: /* SPI2 */
		break;
	case 2: /* SPI3 */
		gpio_change_state(pin_cspi3, ARRAY_SIZE(pin_cspi3),
				  &pin_cspi3_state, GPIO_INACTIVE);
		break;
	default:
		break;
	}
}
EXPORT_SYMBOL(gpio_spi_inactive);

/*****************************************************************************
 * I2C 
 ****************************************************************************/
void gpio_i2c_active(int select)
{
	switch (select) {
	case 0:
		gpio_change_state(pin_i2c1, ARRAY_SIZE(pin_i2c1),
				  &pin_i2c1_state, GPIO_ACTIVE);
		break;
	case 1:
		gpio_change_state(pin_i2c2, ARRAY_SIZE(pin_i2c2),
				  &pin_i2c2_state, GPIO_ACTIVE);
		break;
	case 2:
		break;
	default:
		break;
	}
}
EXPORT_SYMBOL(gpio_i2c_active);

void gpio_i2c_inactive(int select)
{
	switch (select) {
	case 0:
		gpio_change_state(pin_i2c1, ARRAY_SIZE(pin_i2c1),
				  &pin_i2c1_state, GPIO_INACTIVE);
		break;
	case 1:
		gpio_change_state(pin_i2c2, ARRAY_SIZE(pin_i2c2),
				  &pin_i2c2_state, GPIO_INACTIVE);
		break;
	case 2:
		break;
	default:
		break;
	}
}
EXPORT_SYMBOL(gpio_i2c_inactive);

/*****************************************************************************
 * SDHC
 ****************************************************************************/
void gpio_sdhc_active(int select)
{
	switch (select) {
	case 0:
		gpio_change_state(pin_sdhc1, ARRAY_SIZE(pin_sdhc1),
				  &pin_sdhc1_state, GPIO_ACTIVE);
		break;
	case 1:
		gpio_change_state(pin_sdhc2, ARRAY_SIZE(pin_sdhc2),
				  &pin_sdhc2_state, GPIO_ACTIVE);
		break;
	default:
		break;
	}
}
EXPORT_SYMBOL(gpio_sdhc_active);

void gpio_sdhc_inactive(int select)
{
	switch (select) {
	case 0:
		gpio_change_state(pin_sdhc1, ARRAY_SIZE(pin_sdhc1),
				  &pin_sdhc1_state, GPIO_INACTIVE);
		break;
	case 1:
		gpio_change_state(pin_sdhc2, ARRAY_SIZE(pin_sdhc2),
				  &pin_sdhc2_state, GPIO_INACTIVE);
		break;
	default:
		break;
	}
}
EXPORT_SYMBOL(gpio_sdhc_inactive);

int sdhc_get_card_det_status(struct device *dev)
{
	if (to_platform_device(dev)->id == 0)
		return mxc_get_gpio_datain(MX31_PIN_ATA_DMACK);

	if (to_platform_device(dev)->id == 1)
		return mxc_get_gpio_datain(MX31_PIN_CSI_D4);

	return 0;
}

EXPORT_SYMBOL(sdhc_get_card_det_status);

int sdhc_get_ro(struct device *dev)
{
	return 0;
}
EXPORT_SYMBOL(sdhc_get_ro);

int sdhc_init_card_det(int id)
{
	if (id == 0)
		return IOMUX_TO_IRQ(MX31_PIN_ATA_DMACK);

	if (id == 1)
		return IOMUX_TO_IRQ(MX31_PIN_CSI_D4);

	return -1;
}
EXPORT_SYMBOL(sdhc_init_card_det);

/*****************************************************************************
 * Audio
 ****************************************************************************/
/* todo: make these 2 use selects */
void gpio_audio_active(int select)
{
	gpio_change_state(pin_audio4, ARRAY_SIZE(pin_audio4),
			  &pin_audio4_state, GPIO_ACTIVE);

	gpio_change_state(pin_audio5, ARRAY_SIZE(pin_audio5),
			  &pin_audio5_state, GPIO_ACTIVE);
}
EXPORT_SYMBOL(gpio_audio_active);

void gpio_audio_inactive(int select)
{
	gpio_change_state(pin_audio4, ARRAY_SIZE(pin_audio4),
			  &pin_audio4_state, GPIO_INACTIVE);

	gpio_change_state(pin_audio5, ARRAY_SIZE(pin_audio5),
			  &pin_audio5_state, GPIO_INACTIVE);
}
EXPORT_SYMBOL(gpio_audio_inactive);

/*****************************************************************************
 * LCD
 ****************************************************************************/
void gpio_lcd_active(void)
{
	gpio_change_state(pin_lcd, ARRAY_SIZE(pin_lcd),
			  &pin_lcd_state, GPIO_ACTIVE);
}
EXPORT_SYMBOL(gpio_lcd_active);

void gpio_lcd_inactive(void)
{
	gpio_change_state(pin_lcd, ARRAY_SIZE(pin_lcd),
			  &pin_lcd_state, GPIO_INACTIVE);
}
EXPORT_SYMBOL(gpio_lcd_inactive);

/*****************************************************************************
 * CSI - Camera Sensor Interface
 ****************************************************************************/
void gpio_sensor_active(void)
{
	gpio_change_state(pin_csi, ARRAY_SIZE(pin_csi),
			  &pin_csi_state, GPIO_ACTIVE);
}
EXPORT_SYMBOL(gpio_sensor_active);

void gpio_sensor_inactive(void)
{
	gpio_change_state(pin_csi, ARRAY_SIZE(pin_csi),
			  &pin_csi_state, GPIO_INACTIVE);
}
EXPORT_SYMBOL(gpio_sensor_inactive);

/*****************************************************************************
 * USB Host2
 ****************************************************************************/
int gpio_usbh2_active(void)
{
	gpio_change_state(pin_usbh2, ARRAY_SIZE(pin_usbh2),
			  &pin_usbh2_state, GPIO_ACTIVE);
	mxc_iomux_set_gpr(MUX_PGP_UH2, true);

	return 0;
}
EXPORT_SYMBOL(gpio_usbh2_active);

void gpio_usbh2_inactive(void)
{
	gpio_change_state(pin_usbh2, ARRAY_SIZE(pin_usbh2),
			  &pin_usbh2_state, GPIO_INACTIVE);
	mxc_iomux_set_gpr(MUX_PGP_UH2, false);
}
EXPORT_SYMBOL(gpio_usbh2_inactive);

void gpio_usbh2_xcvr_cs_enable(int enable)
{
	if (enable) {
		mxc_set_gpio_direction(MX31_PIN_GPIO1_3, 1/*INPUT*/);
	} else {
		mxc_set_gpio_direction(MX31_PIN_GPIO1_3, 0/*OUTPUT*/);
		mxc_set_gpio_dataout(MX31_PIN_GPIO1_3, 1/*HIGH*/);
	}
}
EXPORT_SYMBOL(gpio_usbh2_xcvr_cs_enable);

void gpio_usbh2_xcvr_reset(void)
{
	gpio_usbh2_xcvr_cs_enable(0);

	/* phy reset */
	mxc_set_gpio_direction(MX31_PIN_SCK6, 0/*OUTPUT*/);
	mxc_set_gpio_dataout(MX31_PIN_SCK6, 1/*HIGH*/);
	mxc_set_gpio_dataout(MX31_PIN_SCK6, 0/*LOW*/);
	msleep(1);
	mxc_set_gpio_dataout(MX31_PIN_SCK6, 1/*HIGH*/);

	gpio_usbh2_xcvr_cs_enable(1);
}
EXPORT_SYMBOL(gpio_usbh2_xcvr_reset);

/*****************************************************************************
 * USB OTG
 ****************************************************************************/
int gpio_usbotg_hs_active(void)
{
	gpio_change_state(pin_usbotg, ARRAY_SIZE(pin_usbotg),
			  &pin_usbotg_state, GPIO_ACTIVE);

	/* phy reset*/
	mxc_set_gpio_direction(MX31_PIN_SRST0, 0/*OUTPUT*/);
        mxc_set_gpio_dataout(MX31_PIN_SRST0, 1/*HIGH*/);
        mxc_set_gpio_dataout(MX31_PIN_SRST0, 0/*LOW*/);
        msleep(1);
        mxc_set_gpio_dataout(MX31_PIN_SRST0, 1/*HIGH*/);

	/* phy cs */
	mxc_set_gpio_direction(MX31_PIN_ATA_CS1, 0/*OUTPUT*/);
	mxc_set_gpio_dataout(MX31_PIN_ATA_CS1, 0/*LOW*/);

	return 0;
}
EXPORT_SYMBOL(gpio_usbotg_hs_active);

void gpio_usbotg_hs_inactive(void)
{
	gpio_change_state(pin_usbotg, ARRAY_SIZE(pin_usbotg),
			  &pin_usbotg_state, GPIO_INACTIVE);
}
EXPORT_SYMBOL(gpio_usbotg_hs_inactive);

/*****************************************************************************
 * USB Host1
 ****************************************************************************/
int gpio_usbh1_active(void)
{
	gpio_change_state(pin_usbh1, ARRAY_SIZE(pin_usbh1),
			  &pin_usbh1_state, GPIO_ACTIVE);
	mxc_iomux_set_gpr(MUX_PGP_USB_SUSPEND, true);

	return 0;
}
EXPORT_SYMBOL(gpio_usbh1_active);

void gpio_usbh1_inactive(void)
{
	gpio_change_state(pin_usbh1, ARRAY_SIZE(pin_usbh1),
			  &pin_usbh1_state, GPIO_INACTIVE);
	mxc_iomux_set_gpr(MUX_PGP_USB_SUSPEND, false);
}
EXPORT_SYMBOL(gpio_usbh1_inactive);

/*****************************************************************************
 * JTAG
 ****************************************************************************/
void gpio_jtag_active(int select)
{
	switch (select) {
	case 0:
		gpio_change_state(pin_jtag, ARRAY_SIZE(pin_jtag),
				  &pin_jtag_state, GPIO_ACTIVE);
		break;
	case 1: /* ETM8 */
		break;
	case 2: /* ETM16 */
		break;
	default:
		break;
	}
}
EXPORT_SYMBOL(gpio_jtag_active);

void gpio_jtag_inactive(int select)
{
	switch (select) {
	case 0:
		gpio_change_state(pin_jtag, ARRAY_SIZE(pin_jtag),
				  &pin_jtag_state, GPIO_INACTIVE);
		break;
	case 1: /* ETM8 */
		break;
	case 2: /* ETM16 */
		break;
	default:
		break;
	}
}
EXPORT_SYMBOL(gpio_jtag_inactive);

/*****************************************************************************
 * LED and Board ID
 ****************************************************************************/
void gpio_led_id_active(void)
{
	gpio_change_state(pin_led_id, ARRAY_SIZE(pin_led_id),
			  &pin_led_id_state, GPIO_ACTIVE);
}
EXPORT_SYMBOL(gpio_led_id_active);

void gpio_led_id_inactive(void)
{
	gpio_change_state(pin_led_id, ARRAY_SIZE(pin_led_id),
			  &pin_led_id_state, GPIO_INACTIVE);
}
EXPORT_SYMBOL(gpio_led_id_inactive);

/*****************************************************************************
 * ETC
 ****************************************************************************/
void gpio_etc_active(void)
{
	gpio_change_state(pin_etc, ARRAY_SIZE(pin_etc),
			  &pin_etc_state, GPIO_ACTIVE);
}
EXPORT_SYMBOL(gpio_etc_active);

void gpio_etc_inactive(void)
{
	gpio_change_state(pin_etc, ARRAY_SIZE(pin_etc),
			  &pin_etc_state, GPIO_INACTIVE);
}
EXPORT_SYMBOL(gpio_etc_inactive);

/*****************************************************************************
 * Bus
 ****************************************************************************/
void gpio_bus_active(void)
{
	gpio_change_state(pin_bus, ARRAY_SIZE(pin_bus),
			  &pin_bus_state, GPIO_ACTIVE);
}
EXPORT_SYMBOL(gpio_bus_active);

void gpio_bus_inactive(void)
{
	gpio_change_state(pin_bus, ARRAY_SIZE(pin_bus),
			  &pin_bus_state, GPIO_INACTIVE);
}
EXPORT_SYMBOL(gpio_bus_inactive);

#if 0 /* template */
/*****************************************************************************
 * 
 ****************************************************************************/
void gpio_@_active(void)
{
	gpio_change_state(pin_@, ARRAY_SIZE(pin_@),
			  &pin_@_state, GPIO_ACTIVE);
}
EXPORT_SYMBOL(gpio_@_active);

void gpio_@_inactive(void)
{
	gpio_change_state(pin_@, ARRAY_SIZE(pin_@),
			  &pin_@_state, GPIO_INACTIVE);
}
EXPORT_SYMBOL(gpio_@_inactive);
#endif

/*****************************************************************************
 * mux init
 ****************************************************************************/
void armadillo500fx_gpio_init(void)
{
	gpio_led_id_active();
	gpio_etc_active();
	gpio_bus_active();
}
