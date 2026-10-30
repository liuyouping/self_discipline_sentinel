/*
 * Copyright (c) 2026 自律哨兵.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h> 
#include "ohos_init.h"
#include "cmsis_os2.h" 

#include "soc_osal.h" 
#include "errcode.h" 
//#include "iot_gpio.h"  
#include "iot_gpio_ex.h"
#include "spi.h" 
//#include "hal_spi.h" 
//#include "demo_font.h"  

#include "app.h"   
#include "app_font.h"   
#include "home.h" 

#include <stdbool.h>
#include <stddef.h> 
#include <stdlib.h> 

#include "qrcodegen.h"
#include "mpu6050.h" 
#include "imu.h"
 
//=====  lcd - ws63e  ===== 
//1. vcc   - 3.3v
//2. gnd   
//3. miso(sdi)  - 
//                spi0              spi1
 
//===========================

// SPI配置: 
#define SPI_IDX            1    // spi_0, 1=no  
 
// 设置DC引脚状态(命令/数据)
#define LCD_DC_0        uapi_gpio_set_val(LCD_DC_GPIO, 0) //0=command
#define LCD_DC_1        uapi_gpio_set_val(LCD_DC_GPIO, 1) //1=data

#define LCD_CS_0        uapi_gpio_set_val(LCD_CS_GPIO, 0) //0
#define LCD_CS_1        uapi_gpio_set_val(LCD_CS_GPIO, 1) //1

// 屏幕尺寸
#define LCD_WIDTH       240
#define LCD_HEIGHT      320 

/* 是否启用SPI通信
 * 0 => 禁用SPI，使用gpio模拟SPI通信
 * 1 => 启用SPI
 */
#define SPI_Enable      1

// 软件SPI延迟函数（根据实际速度调整）
#define SPI_DELAY()     usleep(1)
 

#if SPI_Enable
// 硬件SPI初始化
int spi_init(void)
{    
    spi_attr_t config = { 0 }; 
    config.freq_mhz = 10;  //10                                       /* 工作频率 */ 
    config.bus_clk = 24*1000000;         // 24*                     /* 总线速率 32MHz */
    config.is_slave = false;                                    /* 主机模式 */
    config.frame_size = HAL_SPI_FRAME_SIZE_8;                   /* 帧大小，8位 */
    config.slave_num = 1;//0                                       /* 片选 0 */
    config.spi_frame_format = HAL_SPI_FRAME_FORMAT_STANDARD;    /* 传输模式：标准 */
    config.frame_format = SPI_CFG_FRAME_FORMAT_MOTOROLA_SPI;    /* 协议格式：摩托罗拉SPI协议格式 */
    config.clk_phase = 1;                      /* 相位：空闲低电平 */
    config.clk_polarity = 1;                   /* 极性：第一个时钟沿采集 */ 
    config.tmod = HAL_SPI_TRANS_MODE_TXRX;                      /* 传输模式：收发模式 */
    //config.tmod = HAL_SPI_TRANS_MODE_TX;
    config.sste = 0; 

    spi_extra_attr_t ext_config = { 0 }; 
    ext_config.sspi_param.wait_cycles = 0x10;                   /* 等待周期 */

    // deinit
    uapi_spi_deinit(SPI_IDX);
 
    // 初始化SPI 
    int ret = uapi_spi_init(SPI_IDX, &config, &ext_config);
    if(ret) {
        osal_printk("SpiInit fail: ret = %X \r\n", ret);
        return 0;
    } 
 
    osal_printk("== SPI Initialized\n");
    return 1;
}


// == hardware   

// 硬件写入单个字节
void lcd_spi_write(uint8_t buffer) 
{   
    // 选中LCD 
    LCD_CS_0;  
     
    uint8_t txdata[] = { buffer };
    spi_xfer_data_t xdata;
    memset( &xdata, 0, sizeof(xdata));   
    xdata.tx_buff = txdata;
    xdata.tx_bytes = 1;

    int ret = uapi_spi_master_write(SPI_IDX, &xdata, 0xFF);
    if (ret != 0) {
        osal_printk("SPI write error: %X \r\n", ret); 
    } 

    // 取消选中LCD 
    LCD_CS_1;
}
 

// 初始化GPIO引脚
void lcd_gpio_init(void)
{   
 
    // g7: d10(sck) => scl(2),  hi_io_set_func ,  IOT_IO_FUNC_GPIO_7_SPI0_SCK (=3)
    uapi_pin_set_mode(LCD_SCK_GPIO, IOT_IO_FUNC_GPIO_7_SPI0_SCK);    
    uapi_gpio_set_dir(LCD_SCK_GPIO, 1);
    uapi_gpio_set_val(LCD_SCK_GPIO, 0);   // low
 
    // g11: miso(in) => sdo(6)
    uapi_pin_set_mode(LCD_MISO_GPIO, IOT_IO_FUNC_GPIO_11_SPI0_IN); 
    uapi_gpio_set_dir(LCD_MISO_GPIO, 0); 

#endif
    
    // g10: a12(dc) => d/cx, 初始化DC引脚, IOT_IO_FUNC_GPIO_10_SPI0_CS0_N(no)
    uapi_pin_set_mode(LCD_DC_GPIO, IOT_IO_FUNC_GPIO_10_GPIO);   
    uapi_gpio_set_dir(LCD_DC_GPIO, 1);
    uapi_gpio_set_val(LCD_DC_GPIO, 0);    // low
    
    // g6: 3v3(rst)  => reset  , 初始化RST引脚
    // uapi_pin_set_mode(LCD_RST_GPIO, IOT_IO_FUNC_GPIO_12_GPIO);
    // uapi_gpio_set_dir(LCD_RST_GPIO, 1);
    // uapi_gpio_set_val(LCD_RST_GPIO, 1);   // high, 默认高电平
    
    // g4: blk, 初始化背光控制引脚
    // uapi_pin_set_mode(LCD_BLK_GPIO, IOT_IO_FUNC_GPIO_11_GPIO);
    // uapi_gpio_set_dir(LCD_BLK_GPIO, 1);
    // uapi_gpio_set_val(LCD_BLK_GPIO, 1);   // high, 默认开启背光   

}


// == soft 

// 软件SPI写一个字节
void lcd_spi_write_soft(uint8_t data)
{
    // 选中LCD 
    LCD_CS_0;
    
    // 逐位发送数据 (MSB first)
    for (int i = 0; i < 8; i++) {
        // 设置MOSI引脚
        uapi_gpio_set_val(LCD_MOSI_GPIO, (data & 0x80) ? 1 : 0);
        SPI_DELAY();
        
        // 时钟上升沿
        uapi_gpio_set_val(LCD_SCK_GPIO, 1);
        SPI_DELAY();
        
        // 时钟下降沿
        uapi_gpio_set_val(LCD_SCK_GPIO, 0);
        SPI_DELAY();
        
        // 移位准备下一位
        data <<= 1;
    }
    
    // 取消选中LCD 
    LCD_CS_1;
}

// end soft ==

// SPI写一个字节
void lcd_spi_write_byte(uint8_t data)
{
#if SPI_Enable
    lcd_spi_write( data );
#else
    lcd_spi_write_soft(data);
#endif 
}
 
// 硬件复位LCD
void lcd_reset(void)
{
    uapi_gpio_set_val(LCD_RST_GPIO, 0);
    //hi_udelay(100000); // 延迟100ms
    osDelay(10);
 
}

// 发送命令到LCD
void lcd_write_cmd(uint8_t cmd)
{ 
    LCD_DC_0;  // DC=0表示命令
    lcd_spi_write_byte(cmd);
    //LCD_DC_1;  // DC=1表示数据
}

// 发送数据到LCD
void lcd_write_data(uint8_t data)
{ 
    LCD_DC_1;  // DC=1表示数据
    lcd_spi_write_byte(data);
}

// 发送16位数据到LCD
void lcd_write_data_16(uint16_t data)
{ 
    LCD_DC_1;  // DC=1表示数据
    lcd_spi_write_byte(data >> 8);   // 高字节
    lcd_spi_write_byte(data & 0xFF); // 低字节
} 

// LCD初始化序列
void lcd_init(void)
{ 
    // 初始化SPI和GPIO 
    lcd_gpio_init();

    osal_printk("GPIO Initialized\n");

#if SPI_Enable 
    spi_init(); 
#endif 
    
    // 硬件复位LCD
    lcd_reset();
    osal_printk("LCD Reset\n");
    
    // 发送初始化命令序列
    lcd_write_cmd(0x11);    // Sleep out 
    osDelay(12);            // 延迟120ms

    // 检查初始化代码中的 0x3A（COLMOD）命令，确保设置为 RGB565 模式（值通常为 0x05 或 0x65）。 
    lcd_write_cmd(0x3A);    // COLMOD: Pixel Format Set
    lcd_write_data(0x05);   // 16 bits/pixel   // 05

    
    
    lcd_write_cmd(0xC6);    // FRCTRL2: Frame Rate Control in Normal Mode
    lcd_write_data(0x0F);
    
    lcd_write_cmd(0xD0);    // PWCTRL1: Power Control 1
    lcd_write_data(0xA4);
    lcd_write_data(0xA1);

    lcd_write_cmd(0xD6);    // FRCTRL2: Frame Rate Control in Normal Mode
    lcd_write_data(0xA1);
    
    lcd_write_cmd(0xE0);    // PVGAMCTRL: Positive Voltage Gamma Control
    lcd_write_data(0xF0);
    lcd_write_data(0x00);
    lcd_write_data(0x06);
   
    lcd_write_data(0x34);

    lcd_write_cmd(0xE1);    // NVGAMCTRL: Negative Voltage Gamma Control
    lcd_write_data(0xF0);
    lcd_write_data(0x0B);
    lcd_write_data(0x0F); 
    lcd_write_data(0x2C);
    lcd_write_data(0x32);
    
    lcd_write_cmd(0x21);    // Display Inversion On, 红色显示为蓝色：需要切换色彩顺序或启用显示反转
    lcd_write_cmd(0x29);    // Display On
    //lcd_write_cmd(0x2C);    // RAMWR: Memory Write

    osDelay(10);            // 延迟100ms

    osal_printk("LCD Initialized\n");
}

// 设置显示窗口
void lcd_set_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    lcd_write_cmd(0x2A);        // CASET: Column Address Set
    lcd_write_data(x1 >> 8);
    lcd_write_data(x1 & 0xFF);
    lcd_write_data(x2 >> 8);
    lcd_write_data(x2 & 0xFF);
    
    lcd_write_cmd(0x2B);        // RASET: Row Address Set
    lcd_write_data(y1 >> 8);
    lcd_write_data(y1 & 0xFF);
    lcd_write_data(y2 >> 8);
    lcd_write_data(y2 & 0xFF);
    
    lcd_write_cmd(0x2C);        // RAMWR: Memory Write
}
 
void lcd_fill_screen(uint16_t color)
{
    lcd_set_window(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);
    
    LCD_DC_1;   // 设置为数据模式
    LCD_CS_0;   // 选中LCD
    
    // 预先计算颜色的高低字节
    uint8_t high_byte = color >> 8;
    uint8_t low_byte = color & 0xFF;
    
#if SPI_Enable
    // 使用硬件SPI
    uint8_t buffer[2] = {high_byte, low_byte};
    spi_xfer_data_t xdata;
    memset(&xdata, 0, sizeof(xdata));
    
    // 发送整个屏幕的数据
    for (uint32_t i = 0; i < LCD_WIDTH * LCD_HEIGHT; i++) {
        xdata.tx_buff = buffer;
        xdata.tx_bytes = 2;
        uapi_spi_master_write(SPI_IDX, &xdata, 0xFF);
    }
#else
    // 使用软件SPI
    for (uint32_t i = 0; i < LCD_WIDTH * LCD_HEIGHT; i++) {
        // 发送高字节
        uint8_t temp = high_byte;
        for (int j = 0; j < 8; j++) {
            uapi_gpio_set_val(LCD_MOSI_GPIO, (temp >> 7) & 0x01);
            SPI_DELAY();
            uapi_gpio_set_val(LCD_SCK_GPIO, 1);
            SPI_DELAY();
            uapi_gpio_set_val(LCD_SCK_GPIO, 0);
            SPI_DELAY();
            temp <<= 1;
        }
        
        // 发送低字节
        temp = low_byte;
        for (int j = 0; j < 8; j++) {
            uapi_gpio_set_val(LCD_MOSI_GPIO, (temp >> 7) & 0x01);
            SPI_DELAY();
            uapi_gpio_set_val(LCD_SCK_GPIO, 1);
            SPI_DELAY();
            uapi_gpio_set_val(LCD_SCK_GPIO, 0);
            SPI_DELAY();
            temp <<= 1;
        }
    }
#endif
    
    LCD_CS_1;   // 取消选中LCD
} 
 
// 统一的绘制矩形函数
void lcd_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    if (x + w > LCD_WIDTH) w = LCD_WIDTH - x;
    if (y + h > LCD_HEIGHT) h = LCD_HEIGHT - y;
    
    lcd_set_window(x, y, x + w - 1, y + h - 1);
    
    LCD_DC_1;   // 设置为数据模式
    LCD_CS_0;   // 选中LCD
    
    // 预先计算颜色的高低字节
    uint8_t high_byte = color >> 8;
    uint8_t low_byte = color & 0xFF;
    
#if SPI_Enable
    // 使用硬件SPI
    uint8_t buffer[2] = {high_byte, low_byte};
    spi_xfer_data_t xdata;
    memset(&xdata, 0, sizeof(xdata));
    
    // 发送矩形区域的数据
    for (uint32_t i = 0; i < w * h; i++) {
        xdata.tx_buff = buffer;
        xdata.tx_bytes = 2;
        uapi_spi_master_write(SPI_IDX, &xdata, 0xFF);
    }
#else
    // 使用软件SPI
    for (uint32_t i = 0; i < w * h; i++) {
        // 发送高字节
        uint8_t temp = high_byte;
        for (int j = 0; j < 8; j++) {
            uapi_gpio_set_val(LCD_MOSI_GPIO, (temp >> 7) & 0x01);
            SPI_DELAY();
            uapi_gpio_set_val(LCD_SCK_GPIO, 1);
            SPI_DELAY();
            uapi_gpio_set_val(LCD_SCK_GPIO, 0);
            SPI_DELAY();
            temp <<= 1;
        }
        
        // 发送低字节
        temp = low_byte;
        for (int j = 0; j < 8; j++) {
            uapi_gpio_set_val(LCD_MOSI_GPIO, (temp >> 7) & 0x01);
            SPI_DELAY();
            uapi_gpio_set_val(LCD_SCK_GPIO, 1);
            SPI_DELAY();
            uapi_gpio_set_val(LCD_SCK_GPIO, 0);
            SPI_DELAY();
            temp <<= 1;
        }
    }
#endif
    
    LCD_CS_1;   // 取消选中LCD
}

// 绘制一个像素
void lcd_draw_pixel(uint16_t x, uint16_t y, uint16_t color)
{
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
    
    lcd_set_window(x, y, x, y);
     
    LCD_DC_1;   // 设置为数据模式 
    LCD_CS_0;   // 选中LCD 
    
    // 发送颜色数据的高字节
    for (int j = 0; j < 8; j++) {
        uapi_gpio_set_val(LCD_MOSI_GPIO, (color >> 15) & 0x01);
        SPI_DELAY();
        uapi_gpio_set_val(LCD_SCK_GPIO, 1);
        SPI_DELAY();
        uapi_gpio_set_val(LCD_SCK_GPIO, 0);
        SPI_DELAY();
        color <<= 1;
    }
    
    // 发送颜色数据的低字节
    for (int j = 0; j < 8; j++) {
        uapi_gpio_set_val(LCD_MOSI_GPIO, (color >> 15) & 0x01);
        SPI_DELAY();
        uapi_gpio_set_val(LCD_SCK_GPIO, 1);
        SPI_DELAY();
        uapi_gpio_set_val(LCD_SCK_GPIO, 0);
        SPI_DELAY();
        color <<= 1;
    }
     
    LCD_CS_1;   // 取消选中LCD 
}
 
// 绘制水平线
void lcd_draw_hline(uint16_t x, uint16_t y, uint16_t w, uint16_t color)
{
    lcd_draw_rect(x, y, w, 1, color);
}

// 绘制垂直线
void lcd_draw_vline(uint16_t x, uint16_t y, uint16_t h, uint16_t color)
{
    lcd_draw_rect(x, y, 1, h, color);
}
 
//=== 显示图片
void lcd_display_image(const unsigned char *image) 
{
    lcd_set_window(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1); // 内部已发送 0x2C

    LCD_DC_1;   // 设置为数据模式
    LCD_CS_0;   // 选中LCD

    spi_xfer_data_t xdata;  
    memset(&xdata, 0, sizeof(xdata));          

    for (uint32_t i = 0; i < LCD_WIDTH * LCD_HEIGHT; i++) {
        uint8_t pixel[2] = { image[i*2], image[i*2+1] };

        xdata.tx_buff = pixel;
        xdata.tx_bytes = 2;

        uapi_spi_master_write(SPI_IDX, &xdata, 0xFF);   
    }

    LCD_CS_1;
}

// 显示一个8x16英文字符 ==> 

void display_char_en16(uint16_t x, uint16_t y, char c, uint16_t fg_color, uint16_t bg_color) {
    
    // 获取字符点阵数据
    const uint8_t* char_data = ascii_8x16[c-0x20];
    
    // 设置显示窗口
    lcd_set_window(x, y, x + 7, y + 15);
      
    LCD_DC_1; 
    LCD_CS_0;
    
    // 遍历点阵数据的每一位
    for (int row = 0; row < 16; row++) {
        uint8_t byte = char_data[row];
        
        // 处理当前字节的8位
        for (int col = 0; col < 8; col++) {
            uint16_t color = (byte & (0x80 >> col)) ? fg_color : bg_color; 

            // 发送颜色数据（先高字节后低字节）
            lcd_spi_write_byte(color >> 8);
            lcd_spi_write_byte(color & 0xFF);
        }
    }
     
    LCD_CS_1;
}

// 显示字符串
void display_string_en16(uint16_t x, uint16_t y, const char* str, uint16_t fg_color, uint16_t bg_color) {
    while (*str) {
        display_char_en16(x, y, *str, fg_color, bg_color);
        x += 8; // 移动到下一个字符位置
        str++;
    }
}

// end ==


// chinese 32*32 ==> 

// 查找汉字字模
const uint8_t* find_char_ch32(const char* gb2312_code)
{
    for (int i = 0; i < sizeof(ch32) / sizeof(Chinese_32); i++) {
        if (memcmp(ch32[i].index, gb2312_code, 2) == 0) {
            osal_printk(" ====== find: %d \r\n", i);
            return ch32[i].matrix;
        }
    }
    return NULL; // 未找到对应的汉字
} 
 

/**
 * @brief 显示32x32汉字
 * @param x: 起始X坐标
 * @param y: 起始Y坐标
 * @param gb2312_code: 指针
 * @param fg_color: 前景色
 * @param bg_color: 背景色
 */
void display_char_ch32(uint16_t x, uint16_t y, const uint8_t *gb2312_code, uint16_t fg_color, uint16_t bg_color)
{ 
    const uint8_t* matrix = find_char_ch32(gb2312_code);
    if (matrix == NULL) {
        osal_printk("Chinese character not found: %02X%02X\n", 
               (uint8_t)gb2312_code[0], (uint8_t)gb2312_code[1]);
        return;
    }

    int FONT_SIZE = 32;
    
    // 设置显示区域
    lcd_set_window(x, y, x + FONT_SIZE - 1, y + FONT_SIZE - 1);
    
    // 每行需要4个字节（32/8=4）
    for (int row = 0; row < FONT_SIZE; row++) {
        for (int col_byte = 0; col_byte < 4; col_byte++) {
            uint8_t byte = matrix[row * 4 + col_byte];
            
            for (int bit = 0; bit < 8; bit++) {
                int pixel_x = col_byte * 8 + bit;
                
                // 确保不超出字体宽度
                if (pixel_x >= FONT_SIZE) {
                    continue;
                }
                
                // 根据位值决定像素颜色
                if (byte & (0x80 >> bit)) {
                    lcd_write_data(fg_color >> 8);
                    lcd_write_data(fg_color & 0xFF);
                } else {
                    lcd_write_data(bg_color >> 8);
                    lcd_write_data(bg_color & 0xFF);
                }
            }
        }
    }
}

// 显示字符串
void display_string_ch32(uint16_t x, uint16_t y, const char* str, uint16_t fg_color, uint16_t bg_color) {
    uint16_t current_x = x;
    const char* ptr = str;
    
    while (*ptr != '\0') {
        // 显示一个汉字
        display_char_ch32(current_x, y, ptr, fg_color, bg_color);
        
        // 移动到下一个位置
        current_x += 32;
        ptr += 2; // 跳过GB2312编码的两个字节
        
        // 如果超出屏幕宽度，换行
        if (current_x + 32 > LCD_WIDTH) {
            current_x = x;
            y += 32;
        }
    }
}
// end chinese 36 <==
  
// 测试图案
void lcd_test(void)
{
    osal_printk("Drawing test pattern\n");
    
    // 填充屏幕为黑色 
    lcd_fill_screen(COLOR_BLACK);    
    osDelay(5);  
    
    // 绘制彩色矩形
    lcd_draw_rect(20, 20, 50, 50, COLOR_RED );
    lcd_draw_rect(80, 20, 50, 50, COLOR_GREEN ); 

    lcd_draw_rect(140, 20, 50, 50, COLOR_BLUE);
    lcd_draw_rect(200, 20, 30, 50, COLOR_YELLOW);
    
    lcd_draw_rect(20, 80, 50, 50, COLOR_CYAN);
    lcd_draw_rect(80, 80, 50, 50, COLOR_MAGENTA);

    lcd_draw_rect(140, 80, 50, 50, COLOR_WHITE);
    lcd_draw_rect(200, 80, 30, 50, COLOR_BLACK); 
    
    // 绘制线条
    for (int i = 0; i < 10; i++) {
        lcd_draw_hline(10, 150 + i * 2, 220, COLOR_GREEN); 
    }
    
    for (int i = 0; i < 10; i++) {
        lcd_draw_vline(10 + i * 2, 150, 90, COLOR_BLUE); 
    }   
 
    // 显示汉字
    osal_printk("Displaying Chinese characters...\n");
     
    // 显示英文字符
    display_string_en16(10, 80, "ST7789 lcd!", COLOR_RED, COLOR_GREEN); 
 
    osal_printk("Test pattern completed\n");
}



// 测试图案
void lcd_run(void)
{  
    // 填充屏幕为白色
    lcd_fill_screen(COLOR_BLACK);
 
    osDelay(200);
 
    // 显示"圣象·神灯" (注意: 这里的GB2312编码需要与实际字库匹配)
    display_string_ch32(40, 16, "\xCA\xA5\xCF\xF3\xA1\xA4\xC9\xF1\xB5\xC6",  COLOR_BLACK, COLOR_WHITE);  

    // 绘制线条
    for (int i = 0; i < 2; i++) {
        lcd_draw_hline(10, 60 + i * 2, 220, COLOR_RED); 
    }   
 

    osal_printk("Test pattern completed\n");
}
 

/*---- Utilities ----*/ 

// 打印二维码到串口（每行输出字符，支持边框）
static void printQr_ex(const uint8_t qrcode[]) {
    int size = qrcodegen_getSize(qrcode);
    int border = 4;
    // 每行最大长度：size + 2*border 个模块，每个模块用两个字符（"##"或"  "），再加换行符
    int line_len = (size + 2 * border) * 2 + 1;  // +1 用于 '\0'
    char line[line_len];

    for (int y = -border; y < size + border; y++) {
        int pos = 0;
        for (int x = -border; x < size + border; x++) {
            bool is_black = qrcodegen_getModule(qrcode, x, y);
            // 用两个字符表示一个模块：黑色用 "##"，白色用 "  "
            if (is_black) {
                line[pos++] = '#';
                line[pos++] = '#';
            } else {
                line[pos++] = ' ';
                line[pos++] = ' ';
            }
        }
        line[pos] = '\0';   // 字符串结尾
        osal_printk("%s\n", line);   // 输出一行到串口
    }
    osal_printk("\n");   // 额外空行
} 


// RGB565 颜色定义
#define COLOR_WHITE_RGB565  0xFFFF      // 0xF800
#define COLOR_BLACK_RGB565  0x0000      // 0x07E0

/**
 * 将二维码数据转换为指定尺寸的 RGB565 图像（居中缩放）
 * @param qrcode        由 qrcodegen_encodeText 生成的二维码数据
 * @param rgb565_out    输出数组（必须已分配 target_w * target_h * 2 字节）
 * @param target_w      目标宽度（像素）
 * @param target_h      目标高度（像素）
 */
void qrcode_to_rgb565(const uint8_t *qrcode, uint8_t *rgb565_out,
                      int target_w, int target_h) {
    // 1. 使用官方 API 获取二维码尺寸
    int qr_size = qrcodegen_getSize(qrcode);
    if (qr_size <= 0) return;

    // 2. 计算缩放比例（保持方形，居中显示）
    float scale_w = (float)target_w / qr_size;
    float scale_h = (float)target_h / qr_size;
    float scale = (scale_w < scale_h) ? scale_w : scale_h;  // 取较小值，确保完整显示
    int scaled_size = (int)(qr_size * scale);               // 实际显示的方形边长
    int offset_x = (target_w - scaled_size) / 2;
    int offset_y = (target_h - scaled_size) / 2;

    // 3. 填充白色背景（RGB565 0xFFFF，高字节在前）
    memset(rgb565_out, 0xFF, target_w * target_h * 2);

    // 4. 使用官方 API 遍历每个模块
    for (int row = 0; row < qr_size; row++) {
        for (int col = 0; col < qr_size; col++) {
            // 使用 qrcodegen_getModule() 获取模块颜色（true=黑色）
            bool is_black = qrcodegen_getModule(qrcode, col, row);
            uint16_t color = is_black ? COLOR_BLACK_RGB565 : COLOR_WHITE_RGB565;
            uint8_t high_byte = (color >> 8) & 0xFF;
            uint8_t low_byte  = color & 0xFF;

            // 该模块在目标图像中的矩形区域
            int start_x = offset_x + (int)(col * scale);
            int start_y = offset_y + (int)(row * scale);
            int end_x   = offset_x + (int)((col + 1) * scale);
            int end_y   = offset_y + (int)((row + 1) * scale);

            // 填充该矩形区域
            for (int py = start_y; py < end_y; py++) {
                for (int px = start_x; px < end_x; px++) {
                    int idx = (py * target_w + px) * 2;
                    rgb565_out[idx]     = high_byte;
                    rgb565_out[idx + 1] = low_byte;
                }
            }
        }
    }
}


// Creates a single QR Code, then prints it to the console.
static void doBasicDemo(void) {
    osal_printk("... doBasicDemo Start ... \n");

	const char *text = "https://iot.wit-iot.com/web/sleep.html"   /* 二维码跳转地址，按实际业务修改 */
	enum qrcodegen_Ecc errCorLvl = qrcodegen_Ecc_LOW;  // Error correction level
	
	// Make and print the QR Code symbol
	uint8_t qrcode[qrcodegen_BUFFER_LEN_MAX];
	uint8_t tempBuffer[qrcodegen_BUFFER_LEN_MAX];
	bool ok = qrcodegen_encodeText(text, tempBuffer, qrcode, errCorLvl,
		qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX, qrcodegen_Mask_AUTO, true);
	if (ok) {
        osal_printk("... doBasicDemo printQr ... \n");
		printQr_ex(qrcode);
 
        osal_printk("... doBasicDemo display image ... \n"); 

        // 分配输出缓冲区（240*320*2 = 153600 字节）
        uint8_t *rgb565_image = (uint8_t *)malloc(240 * 320 * 2);
        if (!rgb565_image) {
            osal_printk("Memory allocation failed\n");
            return;
        }

        // 转换为 240x320 RGB565  
        qrcode_to_rgb565(qrcode, rgb565_image, 240, 320);

        // 调用 LCD 显示（假设你的函数接受 RGB565 数据和尺寸）
        lcd_display_image(rgb565_image);

        free(rgb565_image);
    }
}

 
// 主函数
int LcdTask(void)
{
    osal_printk("ST7789 Software SPI Test Start\n");
    
    // 初始化LCD
    lcd_init();

    // 填充屏幕为黑色 
    lcd_fill_screen(COLOR_BLACK);    
    
    // 显示测试图案
    //lcd_test(); 
    osDelay(50);
    
    //lcd_run();
    osDelay(50); 
 
    //lcd_fill_screen(COLOR_BLACK);    
    lcd_display_image(gImage_hh);
    osDelay(50);
    
    lcd_display_image(image_data3); 
    osDelay(50);

    lcd_display_image(image_data4); 
    osDelay(50); 
     
    doBasicDemo();
    
    return 0;
} 
 
// ==== mpu6050 算法
  
MPU6050_Data_TypeDef MPU6050_Data; 
  


// 四元数及积分误差（全局）
float q0 = 1.0f, q1 = 0.0f, q2 = 0.0f, q3 = 0.0f;
float exInt = 0.0f, eyInt = 0.0f, ezInt = 0.0f;

// 滤波器增益
#define KP  0.8f
#define KI  0.001f

// 姿态角输出
float Pitch = 0.0f, Roll = 0.0f, Yaw = 0.0f;

// 陀螺仪零偏（静态校准值）
static int16_t gx_offset = 0, gy_offset = 0, gz_offset = 0;

// 校准函数：上电时调用（使用 osDelay(1) 即 10ms 间隔）
void calibrate_gyro(void) {
    int32_t sum_x = 0, sum_y = 0, sum_z = 0;
    const int count = 50;  // 采集50次，约500ms
    for (int i = 0; i < count; i++) {
        MPU6050_Read_Data();
        sum_x += MPU6050_Data.Gyro[0];
        sum_y += MPU6050_Data.Gyro[1];
        sum_z += MPU6050_Data.Gyro[2];
        osDelay(1);   // 实际延时 10ms
    }
    gx_offset = sum_x / count;
    gy_offset = sum_y / count;
    gz_offset = sum_z / count;
    osal_printk("Gyro offset: %d, %d, %d\n", gx_offset, gy_offset, gz_offset);
}

// 改进后的IMU更新函数（halfT作为参数传入）
void IMUupdate(float gx, float gy, float gz, float ax, float ay, float az, float halfT) {
    float norm, vx, vy, vz, ex, ey, ez;

    // 归一化加速度计
    norm = sqrtf(ax*ax + ay*ay + az*az);
    if (norm == 0.0f) return;
    ax /= norm; ay /= norm; az /= norm;

    // 估计重力方向
    vx = 2.0f * (q1*q3 - q0*q2);
    vy = 2.0f * (q0*q1 + q2*q3);
    vz = q0*q0 - q1*q1 - q2*q2 + q3*q3;

    // 误差（叉积）
    ex = ay*vz - az*vy;
    ey = az*vx - ax*vz;
    ez = ax*vy - ay*vx;

    // 积分误差
    exInt += ex * KI;
    eyInt += ey * KI;
    ezInt += ez * KI;

    // 补偿陀螺仪
    gx += KP*ex + exInt;
    gy += KP*ey + eyInt;
    gz += KP*ez + ezInt;

    // 四元数微分方程
    q0 += (-q1*gx - q2*gy - q3*gz) * halfT;
    q1 += ( q0*gx + q2*gz - q3*gy) * halfT;
    q2 += ( q0*gy - q1*gz + q3*gx) * halfT;
    q3 += ( q0*gz + q1*gy - q2*gx) * halfT;

    // 归一化
    norm = sqrtf(q0*q0 + q1*q1 + q2*q2 + q3*q3);
    if (norm == 0.0f) return;
    q0 /= norm; q1 /= norm; q2 /= norm; q3 /= norm;

    // 计算欧拉角（度）
    Pitch = asinf(2.0f * (q0*q2 - q1*q3)) * 57.2957795f;
    Roll  = atan2f(2.0f * (q2*q3 + q0*q1), -2.0f*q1*q1 - 2.0f*q2*q2 + 1.0f) * 57.2957795f;
    Yaw   = atan2f(2.0f * (q1*q2 + q0*q3), q0*q0 + q1*q1 - q2*q2 - q3*q3) * 57.2957795f;
}


// MpuTask 任务
static void MpuTask(void) {
    IoTPwmInit(0);
    MPU6050_GPIO_Init();

    if (MPU6050ReadID() == 0) {
        osal_printk("MPU6050 not detected!\n");
        return;
    }

    calibrate_gyro();   // 校准零偏

    uint64_t prev_time = osKernelGetSysTimerCount();
    float dt = 0.01f;

    while (1) {
        uint64_t cur_time = osKernelGetSysTimerCount();
        dt = (float)(cur_time - prev_time) / (float)osKernelGetSysTimerFreq();
        prev_time = cur_time;
        if (dt < 0.001f) dt = 0.001f;
        if (dt > 0.05f) dt = 0.05f;

        MPU6050_Read_Data();  // 读取传感器

        // 获取原始数据并转换单位
        int16_t ax_raw = MPU6050_Data.Accel[0];
        int16_t ay_raw = MPU6050_Data.Accel[1];
        int16_t az_raw = MPU6050_Data.Accel[2];
        int16_t gx_raw = MPU6050_Data.Gyro[0] - gx_offset;
        int16_t gy_raw = MPU6050_Data.Gyro[1] - gy_offset;
        int16_t gz_raw = MPU6050_Data.Gyro[2] - gz_offset;

        // 转换为物理单位
        float ax = ax_raw / 8192.0f;
        float ay = ay_raw / 8192.0f;
        float az = az_raw / 8192.0f;
        float gx = (gx_raw / 65.5f) * 0.0174533f;  // 度/秒 -> 弧度/秒
        float gy = (gy_raw / 65.5f) * 0.0174533f;
        float gz = (gz_raw / 65.5f) * 0.0174533f;

        // 姿态更新（halfT = dt/2）
        IMUupdate(gx, gy, gz, ax, ay, az, dt * 0.5f);

        // 打印角度（整数形式）
        int p = (int)(Pitch * 100);
        int r = (int)(Roll * 100);
        int y = (int)(Yaw * 100);
        osal_printk("P=%d.%02d, R=%d.%02d, Y=%d.%02d\n",
                    p/100, abs(p%100), r/100, abs(r%100), y/100, abs(y%100));

        osDelay(1);   // 实际延时 10ms → 约 100Hz 采样率
    }
}



static void LcdEntry(void)
{  
    osThreadAttr_t attr={0};
    attr.name = "LcdTask";
    attr.stack_size = 10*4096;
    attr.priority = osPriorityNormal;

    if(osThreadNew((osThreadFunc_t)LcdTask, NULL, &attr)==NULL) {
        osal_printk("[LcdEntry] create LcdTask failed! \r\n");
    }
    osal_printk("[LcdEntry] create LcdTask ok! \r\n");

    osThreadAttr_t attr2={0};
    attr2.name = "MpuTask";
    attr2.stack_size = 4096;
    attr2.priority = osPriorityNormal;
    if(osThreadNew((osThreadFunc_t)MpuTask, NULL, &attr2)==NULL) {
        osal_printk("[LcdEntry] create MpuTask failed! \r\n");
    }
    osal_printk("[LcdEntry] create MpuTask ok! \r\n");
}

//SYS_RUN(LcdTask); 
APP_FEATURE_INIT(LcdEntry);
