#include "bmp_loader.h"

/**
 * 加载BMP图像
 */
int bmp_load(const char *filename, BMPImage *image) {
    if (!filename || !image) {
        fprintf(stderr, "Error: Invalid parameters\n");
        return -1;
    }
    
    FILE *file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Error: Cannot open file %s\n", filename);
        return -1;
    }
    
    BMPFileHeader file_header;
    BMPInfoHeader info_header;
    
    // 读取文件头
    if (fread(&file_header, sizeof(BMPFileHeader), 1, file) != 1) {
        fprintf(stderr, "Error: Cannot read BMP file header\n");
        fclose(file);
        return -1;
    }
    
    // 检查BMP标识
    if (file_header.bfType != 0x4D42) { // "BM"
        fprintf(stderr, "Error: Not a valid BMP file\n");
        fclose(file);
        return -1;
    }
    
    // 读取信息头
    if (fread(&info_header, sizeof(BMPInfoHeader), 1, file) != 1) {
        fprintf(stderr, "Error: Cannot read BMP info header\n");
        fclose(file);
        return -1;
    }
    
    // 只支持24位和32位BMP
    if (info_header.biBitCount != 24 && info_header.biBitCount != 32) {
        fprintf(stderr, "Error: Only 24-bit and 32-bit BMP files are supported\n");
        fclose(file);
        return -1;
    }
    
    // 不支持压缩
    if (info_header.biCompression != 0) {
        fprintf(stderr, "Error: Compressed BMP files are not supported\n");
        fclose(file);
        return -1;
    }
    
    // 设置图像参数
    image->width = info_header.biWidth;
    image->height = abs(info_header.biHeight);
    image->bpp = info_header.biBitCount;
    
    // 计算行的字节数（4字节对齐）
    int bytes_per_pixel = info_header.biBitCount / 8;
    int row_bytes = ((image->width * bytes_per_pixel + 3) / 4) * 4;
    
    // 分配内存
    size_t data_size = image->width * image->height * sizeof(uint16_t);
    image->data = (uint16_t *)malloc(data_size);
    if (!image->data) {
        fprintf(stderr, "Error: Cannot allocate memory for image data\n");
        fclose(file);
        return -1;
    }
    
    // 分配临时行缓冲区
    uint8_t *row_buffer = (uint8_t *)malloc(row_bytes);
    if (!row_buffer) {
        fprintf(stderr, "Error: Cannot allocate memory for row buffer\n");
        free(image->data);
        fclose(file);
        return -1;
    }
    
    // 移动到像素数据位置
    fseek(file, file_header.bfOffBits, SEEK_SET);
    
    // 读取像素数据
    int is_bottom_up = (info_header.biHeight > 0);
    
    for (int y = 0; y < image->height; y++) {
        // 读取一行数据
        if (fread(row_buffer, row_bytes, 1, file) != 1) {
            fprintf(stderr, "Error: Cannot read pixel data\n");
            free(row_buffer);
            free(image->data);
            fclose(file);
            return -1;
        }
        
        // 计算目标行索引
        int dst_y = is_bottom_up ? (image->height - 1 - y) : y;
        
        // 转换像素格式到RGB565
        for (int x = 0; x < image->width; x++) {
            uint8_t b, g, r;
            
            if (bytes_per_pixel == 3) {
                // 24位BMP: BGR
                b = row_buffer[x * 3 + 0];
                g = row_buffer[x * 3 + 1];
                r = row_buffer[x * 3 + 2];
            } else {
                // 32位BMP: BGRA (忽略alpha通道)
                b = row_buffer[x * 4 + 0];
                g = row_buffer[x * 4 + 1];
                r = row_buffer[x * 4 + 2];
                // uint8_t a = row_buffer[x * 4 + 3]; // alpha通道暂不使用
            }
            
            // 转换到RGB565
            image->data[dst_y * image->width + x] = bgr_to_rgb565(b, g, r);
        }
    }
    
    free(row_buffer);
    fclose(file);
    
    printf("BMP loaded: %s (%dx%d, %d-bit)\n", filename, image->width, image->height, image->bpp);
    return 0;
}

/**
 * 释放BMP图像内存
 */
void bmp_free(BMPImage *image) {
    if (!image) return;
    
    if (image->data) {
        free(image->data);
        image->data = NULL;
    }
    
    image->width = 0;
    image->height = 0;
    image->bpp = 0;
}

/**
 * 将BMP图像转换并复制到RGB565缓冲区
 */
int bmp_convert_to_rgb565(BMPImage *image, uint16_t *buffer, int buf_width, int buf_height) {
    if (!image || !image->data || !buffer) {
        return -1;
    }
    
    // 计算缩放和居中参数
    float scale_x = (float)buf_width / image->width;
    float scale_y = (float)buf_height / image->height;
    float scale = (scale_x < scale_y) ? scale_x : scale_y;
    
    int new_width = (int)(image->width * scale);
    int new_height = (int)(image->height * scale);
    int offset_x = (buf_width - new_width) / 2;
    int offset_y = (buf_height - new_height) / 2;
    
    // 首先清除缓冲区
    for (int i = 0; i < buf_width * buf_height; i++) {
        buffer[i] = 0x0000; // 黑色背景
    }
    
    // 简单最近邻插值缩放
    for (int y = 0; y < new_height; y++) {
        for (int x = 0; x < new_width; x++) {
            int src_x = (int)(x / scale);
            int src_y = (int)(y / scale);
            
            // 边界检查
            if (src_x >= image->width) src_x = image->width - 1;
            if (src_y >= image->height) src_y = image->height - 1;
            
            int dst_x = x + offset_x;
            int dst_y = y + offset_y;
            
            if (dst_x >= 0 && dst_x < buf_width && dst_y >= 0 && dst_y < buf_height) {
                buffer[dst_y * buf_width + dst_x] = image->data[src_y * image->width + src_x];
            }
        }
    }
    
    return 0;
}

/**
 * 直接将BMP文件绘制到缓冲区
 */
int bmp_draw_to_buffer(const char *filename, uint16_t *buffer, int buf_width, int buf_height, 
                      int dst_x, int dst_y) {
    BMPImage image;
    
    if (bmp_load(filename, &image) != 0) {
        return -1;
    }
    
    // 简单复制（不缩放）
    for (int y = 0; y < image.height && (dst_y + y) < buf_height; y++) {
        for (int x = 0; x < image.width && (dst_x + x) < buf_width; x++) {
            if ((dst_x + x) >= 0 && (dst_y + y) >= 0) {
                buffer[(dst_y + y) * buf_width + (dst_x + x)] = 
                    image.data[y * image.width + x];
            }
        }
    }
    
    bmp_free(&image);
    return 0;
}

/**
 * BGR转RGB565
 */
uint16_t bgr_to_rgb565(uint8_t b, uint8_t g, uint8_t r) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

/**
 * RGB565转BGR
 */
void rgb565_to_bgr(uint16_t color, uint8_t *b, uint8_t *g, uint8_t *r) {
    *r = (color >> 8) & 0xF8;
    *g = (color >> 3) & 0xFC;
    *b = (color << 3) & 0xF8;
}

/**
 * 智能适配BMP图像到缓冲区（支持自动旋转以最佳填充屏幕）
 */
int bmp_convert_to_rgb565_smart_fit(BMPImage *image, uint16_t *buffer, int buf_width, int buf_height, int auto_rotate) {
    if (!image || !image->data || !buffer) {
        return -1;
    }
    
    int src_width = image->width;
    int src_height = image->height;
    uint16_t *src_data = image->data;
    uint16_t *rotated_data = NULL;
    
    // 检查是否需要自动旋转
    if (auto_rotate) {
        // 如果图像是横屏(320x240)而屏幕是竖屏(240x320)，自动旋转90度
        if (src_width > src_height && buf_width < buf_height) {
            printf("Auto-rotating image 90° (landscape to portrait)\n");
            
            // 分配旋转后的缓冲区
            rotated_data = (uint16_t *)malloc(src_width * src_height * sizeof(uint16_t));
            if (!rotated_data) {
                return -1;
            }
            
            // 90度旋转: 将320x240转为240x320
            for (int y = 0; y < src_height; y++) {
                for (int x = 0; x < src_width; x++) {
                    // (x, y) -> (y, src_width - 1 - x)
                    int new_x = y;
                    int new_y = src_width - 1 - x;
                    rotated_data[new_y * src_height + new_x] = src_data[y * src_width + x];
                }
            }
            
            // 更新尺寸和数据指针
            src_data = rotated_data;
            int temp = src_width;
            src_width = src_height;
            src_height = temp;
        }
        // 如果图像是竖屏而屏幕是横屏，也可以类似处理
        else if (src_width < src_height && buf_width > buf_height) {
            printf("Auto-rotating image 270° (portrait to landscape)\n");
            
            rotated_data = (uint16_t *)malloc(src_width * src_height * sizeof(uint16_t));
            if (!rotated_data) {
                return -1;
            }
            
            // 270度旋转 (或-90度)
            for (int y = 0; y < src_height; y++) {
                for (int x = 0; x < src_width; x++) {
                    // (x, y) -> (src_height - 1 - y, x)
                    int new_x = src_height - 1 - y;
                    int new_y = x;
                    rotated_data[new_y * src_height + new_x] = src_data[y * src_width + x];
                }
            }
            
            src_data = rotated_data;
            int temp = src_width;
            src_width = src_height;
            src_height = temp;
        }
    }
    
    // 现在进行缩放适配
    // 选择拉伸填充还是保持宽高比
    // 这里我们使用拉伸填充以完全利用屏幕空间
    if (src_width == buf_width && src_height == buf_height) {
        // 完全匹配，直接复制
        memcpy(buffer, src_data, buf_width * buf_height * sizeof(uint16_t));
    } else {
        // 使用双线性插值进行缩放
        for (int y = 0; y < buf_height; y++) {
            for (int x = 0; x < buf_width; x++) {
                // 计算源图像中的对应位置
                float src_x = (float)x * src_width / buf_width;
                float src_y = (float)y * src_height / buf_height;
                
                // 简单最近邻插值
                int ix = (int)(src_x + 0.5f);
                int iy = (int)(src_y + 0.5f);
                
                // 边界检查
                if (ix >= src_width) ix = src_width - 1;
                if (iy >= src_height) iy = src_height - 1;
                if (ix < 0) ix = 0;
                if (iy < 0) iy = 0;
                
                buffer[y * buf_width + x] = src_data[iy * src_width + ix];
            }
        }
    }
    
    // 清理旋转缓冲区
    if (rotated_data) {
        free(rotated_data);
    }
    
    return 0;
}
