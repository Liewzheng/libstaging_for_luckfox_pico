#ifndef _BMP_LOADER_H_
#define _BMP_LOADER_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// BMP文件头结构
#pragma pack(push, 1)
typedef struct {
    uint16_t bfType;        // 文件类型，必须是"BM"
    uint32_t bfSize;        // 文件大小
    uint16_t bfReserved1;   // 保留字段
    uint16_t bfReserved2;   // 保留字段
    uint32_t bfOffBits;     // 像素数据偏移量
} BMPFileHeader;

typedef struct {
    uint32_t biSize;        // 信息头大小
    int32_t  biWidth;       // 图像宽度
    int32_t  biHeight;      // 图像高度
    uint16_t biPlanes;      // 颜色平面数
    uint16_t biBitCount;    // 每像素位数
    uint32_t biCompression; // 压缩类型
    uint32_t biSizeImage;   // 图像大小
    int32_t  biXPelsPerMeter;  // 水平分辨率
    int32_t  biYPelsPerMeter;  // 垂直分辨率
    uint32_t biClrUsed;     // 使用的颜色数
    uint32_t biClrImportant; // 重要颜色数
} BMPInfoHeader;
#pragma pack(pop)

// BMP图像数据结构
typedef struct {
    int width;
    int height;
    int bpp;                // 每像素位数
    uint16_t *data;         // RGB565格式的像素数据
} BMPImage;

// 函数声明
int bmp_load(const char *filename, BMPImage *image);
void bmp_free(BMPImage *image);
int bmp_convert_to_rgb565(BMPImage *image, uint16_t *buffer, int buf_width, int buf_height);
int bmp_convert_to_rgb565_smart_fit(BMPImage *image, uint16_t *buffer, int buf_width, int buf_height, int auto_rotate);
int bmp_draw_to_buffer(const char *filename, uint16_t *buffer, int buf_width, int buf_height, 
                      int dst_x, int dst_y);

// 颜色转换工具
uint16_t bgr_to_rgb565(uint8_t b, uint8_t g, uint8_t r);
void rgb565_to_bgr(uint16_t color, uint8_t *b, uint8_t *g, uint8_t *r);

#endif /* _BMP_LOADER_H_ */
