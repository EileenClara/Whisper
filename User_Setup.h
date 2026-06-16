// ================================================================
//  User_Setup.h — 和老板给的完全一致
//  驱动: ST7789_2_DRIVER, 引脚: MOSI=1, SCLK=18, DC=17, RST=6, BL=2
// ================================================================

#define USER_SETUP_INFO "User_Setup"

// ##################################################################
// Section 1. 驱动类型 — 老板用的是 ST7789_2_DRIVER！
// ##################################################################
#define ST7789_2_DRIVER

#define TFT_RGB_ORDER TFT_RGB

#define TFT_WIDTH  240
#define TFT_HEIGHT 240

#define ST7735_BLACKTAB

// ##################################################################
// Section 2. 引脚 — 老板的配置
// ##################################################################

// 3线 SPI (无 MISO), 与硬件 FPC 一致
#define TFT_MOSI 1
#define TFT_SCLK 18
#define TFT_DC   17
#define TFT_RST  6
#define TFT_BL   2

// ##################################################################
// Section 3. 字体
// ##################################################################
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4   // 26px, 秒数
#define LOAD_FONT6   // 48px, 时分数
#define LOAD_GFXFF
#define SMOOTH_FONT

// ##################################################################
// Section 4. SPI
// ##################################################################
#define SPI_FREQUENCY         27000000
#define SPI_READ_FREQUENCY    20000000
#define SPI_TOUCH_FREQUENCY   2500000
