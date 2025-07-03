#ifndef _FBTFT_LCD_H_
#define _FBTFT_LCD_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <stdint.h>

// LCD 规格定义 (这些是默认值，实际分辨率从设备获取)
#define FBTFT_LCD_DEFAULT_WIDTH     320
#define FBTFT_LCD_DEFAULT_HEIGHT    240
#define FBTFT_LCD_BPP               16  // 16位色深

// 颜色定义 (RGB565格式)
#define FBTFT_WHITE         0xFFFF
#define FBTFT_BLACK         0x0000
#define FBTFT_RED           0xF800
#define FBTFT_GREEN         0x07E0
#define FBTFT_BLUE          0x001F
#define FBTFT_YELLOW        0xFFE0
#define FBTFT_CYAN          0x07FF
#define FBTFT_MAGENTA       0xF81F

// FBTFT LCD 结构体
typedef struct {
    int fb_fd;                          // framebuffer文件描述符
    uint16_t *fb_mem;                   // framebuffer内存映射地址
    size_t fb_size;                     // framebuffer大小
    struct fb_var_screeninfo vinfo;     // 可变屏幕信息
    struct fb_fix_screeninfo finfo;     // 固定屏幕信息
    int width;                          // 屏幕宽度
    int height;                         // 屏幕高度
    int bpp;                            // 每像素位数
    char device_path[256];              // 设备路径
} fbtft_lcd_t;

// 函数声明
int fbtft_lcd_init(fbtft_lcd_t *lcd, const char *device_path);
void fbtft_lcd_deinit(fbtft_lcd_t *lcd);
int fbtft_lcd_clear(fbtft_lcd_t *lcd, uint16_t color);
int fbtft_lcd_display_buffer(fbtft_lcd_t *lcd, const uint16_t *buffer);
int fbtft_lcd_set_pixel(fbtft_lcd_t *lcd, int x, int y, uint16_t color);
uint16_t fbtft_lcd_get_pixel(fbtft_lcd_t *lcd, int x, int y);
int fbtft_lcd_draw_rectangle(fbtft_lcd_t *lcd, int x1, int y1, int x2, int y2, uint16_t color);
int fbtft_lcd_fill_rectangle(fbtft_lcd_t *lcd, int x1, int y1, int x2, int y2, uint16_t color);
int fbtft_lcd_sync(fbtft_lcd_t *lcd);

// 颜色转换函数
uint16_t rgb_to_rgb565(uint8_t r, uint8_t g, uint8_t b);
void rgb565_to_rgb(uint16_t color, uint8_t *r, uint8_t *g, uint8_t *b);

// 工具函数
void fbtft_lcd_print_info(fbtft_lcd_t *lcd);
int fbtft_lcd_check_device(const char *device_path);

// 旋转和镜像枚举
typedef enum {
    ROTATE_0 = 0,
    ROTATE_90 = 90,
    ROTATE_180 = 180,
    ROTATE_270 = 270
} rotation_t;

typedef enum {
    MIRROR_NONE = 0,
    MIRROR_HORIZONTAL = 1,
    MIRROR_VERTICAL = 2,
    MIRROR_BOTH = 3
} mirror_t;

// 旋转和缩放函数
void fbtft_lcd_rotate_90(uint16_t *src, uint16_t *dst, int src_width, int src_height);
void fbtft_lcd_rotate_180(uint16_t *src, uint16_t *dst, int src_width, int src_height);
void fbtft_lcd_rotate_270(uint16_t *src, uint16_t *dst, int src_width, int src_height);
void fbtft_lcd_mirror_horizontal(uint16_t *buffer, int width, int height);
void fbtft_lcd_mirror_vertical(uint16_t *buffer, int width, int height);
void fbtft_lcd_transform_buffer(uint16_t *src, uint16_t *dst, int width, int height, 
                               rotation_t rotation, mirror_t mirror);
int fbtft_lcd_auto_fit_buffer(fbtft_lcd_t *lcd, const uint16_t *src_buffer, 
                             int src_width, int src_height);

#endif /* _FBTFT_LCD_H_ */
