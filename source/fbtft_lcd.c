#include "fbtft_lcd.h"

/**
 * 初始化FBTFT LCD设备
 */
int fbtft_lcd_init(fbtft_lcd_t *lcd, const char *device_path) {
    if (!lcd || !device_path) {
        fprintf(stderr, "Error: Invalid parameters\n");
        return -1;
    }
    
    // 保存设备路径
    strncpy(lcd->device_path, device_path, sizeof(lcd->device_path) - 1);
    lcd->device_path[sizeof(lcd->device_path) - 1] = '\0';
    
    // 打开framebuffer设备
    lcd->fb_fd = open(device_path, O_RDWR);
    if (lcd->fb_fd == -1) {
        perror("Error opening framebuffer device");
        return -1;
    }
    
    // 获取固定屏幕信息
    if (ioctl(lcd->fb_fd, FBIOGET_FSCREENINFO, &lcd->finfo) == -1) {
        perror("Error reading fixed information");
        close(lcd->fb_fd);
        return -1;
    }
    
    // 获取可变屏幕信息
    if (ioctl(lcd->fb_fd, FBIOGET_VSCREENINFO, &lcd->vinfo) == -1) {
        perror("Error reading variable information");
        close(lcd->fb_fd);
        return -1;
    }
    
    // 设置屏幕参数
    lcd->width = lcd->vinfo.xres;
    lcd->height = lcd->vinfo.yres;
    lcd->bpp = lcd->vinfo.bits_per_pixel;
    
    // 计算framebuffer大小
    lcd->fb_size = lcd->finfo.smem_len;
    
    // 内存映射framebuffer
    lcd->fb_mem = (uint16_t *)mmap(0, lcd->fb_size, PROT_READ | PROT_WRITE, 
                                   MAP_SHARED, lcd->fb_fd, 0);
    if (lcd->fb_mem == MAP_FAILED) {
        perror("Error mapping framebuffer to memory");
        close(lcd->fb_fd);
        return -1;
    }
    
    printf("FBTFT LCD initialized successfully:\n");
    printf("  Device: %s\n", device_path);
    printf("  Resolution: %dx%d\n", lcd->width, lcd->height);
    printf("  BPP: %d\n", lcd->bpp);
    printf("  FB Size: %zu bytes\n", lcd->fb_size);
    
    return 0;
}

/**
 * 释放FBTFT LCD设备
 */
void fbtft_lcd_deinit(fbtft_lcd_t *lcd) {
    if (!lcd) return;
    
    if (lcd->fb_mem != MAP_FAILED && lcd->fb_mem != NULL) {
        munmap(lcd->fb_mem, lcd->fb_size);
        lcd->fb_mem = NULL;
    }
    
    if (lcd->fb_fd >= 0) {
        close(lcd->fb_fd);
        lcd->fb_fd = -1;
    }
    
    printf("FBTFT LCD deinitialized\n");
}

/**
 * 清屏
 */
int fbtft_lcd_clear(fbtft_lcd_t *lcd, uint16_t color) {
    if (!lcd || !lcd->fb_mem) {
        fprintf(stderr, "Error: LCD not initialized\n");
        return -1;
    }
    
    size_t pixel_count = lcd->width * lcd->height;
    for (size_t i = 0; i < pixel_count; i++) {
        lcd->fb_mem[i] = color;
    }
    
    return 0;
}

/**
 * 显示缓冲区数据到LCD
 */
int fbtft_lcd_display_buffer(fbtft_lcd_t *lcd, const uint16_t *buffer) {
    if (!lcd || !lcd->fb_mem || !buffer) {
        fprintf(stderr, "Error: Invalid parameters\n");
        return -1;
    }
    
    size_t pixel_count = lcd->width * lcd->height;
    memcpy(lcd->fb_mem, buffer, pixel_count * sizeof(uint16_t));
    
    return 0;
}

/**
 * 设置单个像素
 */
int fbtft_lcd_set_pixel(fbtft_lcd_t *lcd, int x, int y, uint16_t color) {
    if (!lcd || !lcd->fb_mem) {
        fprintf(stderr, "Error: LCD not initialized\n");
        return -1;
    }
    
    if (x < 0 || x >= lcd->width || y < 0 || y >= lcd->height) {
        return -1; // 坐标超出范围
    }
    
    lcd->fb_mem[y * lcd->width + x] = color;
    return 0;
}

/**
 * 获取单个像素
 */
uint16_t fbtft_lcd_get_pixel(fbtft_lcd_t *lcd, int x, int y) {
    if (!lcd || !lcd->fb_mem) {
        return 0;
    }
    
    if (x < 0 || x >= lcd->width || y < 0 || y >= lcd->height) {
        return 0; // 坐标超出范围
    }
    
    return lcd->fb_mem[y * lcd->width + x];
}

/**
 * 绘制矩形边框
 */
int fbtft_lcd_draw_rectangle(fbtft_lcd_t *lcd, int x1, int y1, int x2, int y2, uint16_t color) {
    if (!lcd) return -1;
    
    // 确保坐标顺序正确
    if (x1 > x2) { int temp = x1; x1 = x2; x2 = temp; }
    if (y1 > y2) { int temp = y1; y1 = y2; y2 = temp; }
    
    // 绘制水平线
    for (int x = x1; x <= x2; x++) {
        fbtft_lcd_set_pixel(lcd, x, y1, color);
        fbtft_lcd_set_pixel(lcd, x, y2, color);
    }
    
    // 绘制垂直线
    for (int y = y1; y <= y2; y++) {
        fbtft_lcd_set_pixel(lcd, x1, y, color);
        fbtft_lcd_set_pixel(lcd, x2, y, color);
    }
    
    return 0;
}

/**
 * 填充矩形
 */
int fbtft_lcd_fill_rectangle(fbtft_lcd_t *lcd, int x1, int y1, int x2, int y2, uint16_t color) {
    if (!lcd) return -1;
    
    // 确保坐标顺序正确
    if (x1 > x2) { int temp = x1; x1 = x2; x2 = temp; }
    if (y1 > y2) { int temp = y1; y1 = y2; y2 = temp; }
    
    // 边界检查
    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 >= lcd->width) x2 = lcd->width - 1;
    if (y2 >= lcd->height) y2 = lcd->height - 1;
    
    for (int y = y1; y <= y2; y++) {
        for (int x = x1; x <= x2; x++) {
            fbtft_lcd_set_pixel(lcd, x, y, color);
        }
    }
    
    return 0;
}

/**
 * 同步framebuffer (强制刷新到硬件)
 */
int fbtft_lcd_sync(fbtft_lcd_t *lcd) {
    if (!lcd || lcd->fb_fd < 0) {
        return -1;
    }
    
    // 使用fsync强制刷新到硬件
    return fsync(lcd->fb_fd);
}

/**
 * 控制LCD电源模式
 * @param lcd LCD设备结构体
 * @param power_mode 电源模式：
 *                   FBTFT_LCD_POWER_ON (0) - 显示开启
 *                   FBTFT_LCD_POWER_OFF (1) - 显示关闭
 *                   FBTFT_LCD_POWER_SUSPEND (4) - 显示挂起
 * @return 成功返回0，失败返回-1
 */
int fbtft_lcd_power_mode(fbtft_lcd_t *lcd, int power_mode) {
    if (!lcd) {
        fprintf(stderr, "Error: Invalid LCD parameter\n");
        return -1;
    }
    
    // 检查电源模式参数
    if (power_mode != FBTFT_LCD_POWER_ON && 
        power_mode != FBTFT_LCD_POWER_OFF && 
        power_mode != FBTFT_LCD_POWER_SUSPEND) {
        fprintf(stderr, "Error: Invalid power mode %d\n", power_mode);
        return -1;
    }
    
    // 构建sysfs路径
    char sysfs_path[512];
    char *fb_device = strrchr(lcd->device_path, '/');
    if (!fb_device) {
        fprintf(stderr, "Error: Invalid device path format\n");
        return -1;
    }
    fb_device++; // 跳过 '/'
    
    // 根据设备路径构建对应的sysfs blank控制路径
    // 例如: /dev/fb0 -> /sys/bus/spi/devices/spi0.0/graphics/fb0/blank
    snprintf(sysfs_path, sizeof(sysfs_path), 
             "/sys/bus/spi/devices/spi0.0/graphics/%s/blank", fb_device);
    
    // 打开sysfs文件
    FILE *blank_file = fopen(sysfs_path, "w");
    if (!blank_file) {
        // 如果标准路径失败，尝试其他可能的路径
        snprintf(sysfs_path, sizeof(sysfs_path), 
                 "/sys/class/graphics/%s/blank", fb_device);
        blank_file = fopen(sysfs_path, "w");
        
        if (!blank_file) {
            perror("Error: Cannot open LCD blank control file");
            fprintf(stderr, "Tried paths:\n");
            fprintf(stderr, "  /sys/bus/spi/devices/spi0.0/graphics/%s/blank\n", fb_device);
            fprintf(stderr, "  /sys/class/graphics/%s/blank\n", fb_device);
            return -1;
        }
    }
    
    // 写入电源模式值
    if (fprintf(blank_file, "%d\n", power_mode) < 0) {
        perror("Error: Failed to write power mode");
        fclose(blank_file);
        return -1;
    }
    
    // 确保数据写入
    if (fflush(blank_file) != 0) {
        perror("Error: Failed to flush power mode");
        fclose(blank_file);
        return -1;
    }
    
    fclose(blank_file);
    
    // 打印操作结果
    const char *mode_str;
    switch (power_mode) {
        case FBTFT_LCD_POWER_ON:
            mode_str = "ON";
            break;
        case FBTFT_LCD_POWER_OFF:
            mode_str = "OFF";
            break;
        case FBTFT_LCD_POWER_SUSPEND:
            mode_str = "SUSPEND";
            break;
        default:
            mode_str = "UNKNOWN";
            break;
    }
    
    printf("LCD power mode set to %s (%d) via %s\n", mode_str, power_mode, sysfs_path);
    
    return 0;
}

/**
 * 开启LCD显示 (便利函数)
 */
int fbtft_lcd_power_on(fbtft_lcd_t *lcd) {
    return fbtft_lcd_power_mode(lcd, FBTFT_LCD_POWER_ON);
}

/**
 * 关闭LCD显示 (便利函数)
 */
int fbtft_lcd_power_off(fbtft_lcd_t *lcd) {
    return fbtft_lcd_power_mode(lcd, FBTFT_LCD_POWER_OFF);
}

/**
 * RGB转RGB565
 */
uint16_t rgb_to_rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

/**
 * RGB565转RGB
 */
void rgb565_to_rgb(uint16_t color, uint8_t *r, uint8_t *g, uint8_t *b) {
    *r = (color >> 8) & 0xF8;
    *g = (color >> 3) & 0xFC;
    *b = (color << 3) & 0xF8;
}

/**
 * 打印LCD信息
 */
void fbtft_lcd_print_info(fbtft_lcd_t *lcd) {
    if (!lcd) return;
    
    printf("=== FBTFT LCD Information ===\n");
    printf("Device Path: %s\n", lcd->device_path);
    printf("Resolution: %dx%d\n", lcd->width, lcd->height);
    printf("Bits Per Pixel: %d\n", lcd->bpp);
    printf("Framebuffer Size: %zu bytes\n", lcd->fb_size);
    printf("Line Length: %d bytes\n", lcd->finfo.line_length);
    printf("Memory Start: 0x%lx\n", lcd->finfo.smem_start);
    printf("X Resolution: %d\n", lcd->vinfo.xres);
    printf("Y Resolution: %d\n", lcd->vinfo.yres);
    printf("Virtual X: %d\n", lcd->vinfo.xres_virtual);
    printf("Virtual Y: %d\n", lcd->vinfo.yres_virtual);
    printf("Red: offset=%d, length=%d\n", lcd->vinfo.red.offset, lcd->vinfo.red.length);
    printf("Green: offset=%d, length=%d\n", lcd->vinfo.green.offset, lcd->vinfo.green.length);
    printf("Blue: offset=%d, length=%d\n", lcd->vinfo.blue.offset, lcd->vinfo.blue.length);
    printf("=============================\n");
}

/**
 * 检查设备是否存在
 */
int fbtft_lcd_check_device(const char *device_path) {
    if (!device_path) return 0;
    
    int fd = open(device_path, O_RDONLY);
    if (fd == -1) {
        return 0; // 设备不存在或无法访问
    }
    
    close(fd);
    return 1; // 设备存在
}

/**
 * 90度顺时针旋转缓冲区
 */
void fbtft_lcd_rotate_90(uint16_t *src, uint16_t *dst, int src_width, int src_height) {
    for (int y = 0; y < src_height; y++) {
        for (int x = 0; x < src_width; x++) {
            // (x, y) -> (y, src_width - 1 - x)
            dst[y * src_width + (src_width - 1 - x)] = src[y * src_width + x];
        }
    }
}

/**
 * 270度顺时针旋转缓冲区 (或90度逆时针)
 */
void fbtft_lcd_rotate_270(uint16_t *src, uint16_t *dst, int src_width, int src_height) {
    for (int y = 0; y < src_height; y++) {
        for (int x = 0; x < src_width; x++) {
            // (x, y) -> (src_height - 1 - y, x)
            dst[(src_height - 1 - y) * src_height + x] = src[y * src_width + x];
        }
    }
}

/**
 * 180度旋转缓冲区
 */
void fbtft_lcd_rotate_180(uint16_t *src, uint16_t *dst, int src_width, int src_height) {
    int total_pixels = src_width * src_height;
    for (int i = 0; i < total_pixels; i++) {
        dst[total_pixels - 1 - i] = src[i];
    }
}

/**
 * 水平镜像缓冲区
 */
void fbtft_lcd_mirror_horizontal(uint16_t *buffer, int width, int height) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width / 2; x++) {
            int left_idx = y * width + x;
            int right_idx = y * width + (width - 1 - x);
            
            // 交换左右像素
            uint16_t temp = buffer[left_idx];
            buffer[left_idx] = buffer[right_idx];
            buffer[right_idx] = temp;
        }
    }
}

/**
 * 垂直镜像缓冲区
 */
void fbtft_lcd_mirror_vertical(uint16_t *buffer, int width, int height) {
    for (int y = 0; y < height / 2; y++) {
        for (int x = 0; x < width; x++) {
            int top_idx = y * width + x;
            int bottom_idx = (height - 1 - y) * width + x;
            
            // 交换上下像素
            uint16_t temp = buffer[top_idx];
            buffer[top_idx] = buffer[bottom_idx];
            buffer[bottom_idx] = temp;
        }
    }
}

/**
 * 综合变换缓冲区（旋转 + 镜像）
 */
void fbtft_lcd_transform_buffer(uint16_t *src, uint16_t *dst, int width, int height, 
                               rotation_t rotation, mirror_t mirror) {
    if (!src || !dst) return;
    
    uint16_t *work_buffer = dst;
    int work_width = width;
    int work_height = height;
    
    // 首先进行旋转
    switch (rotation) {
        case ROTATE_0:
            // 不旋转，直接复制
            memcpy(dst, src, width * height * sizeof(uint16_t));
            break;
            
        case ROTATE_90:
            fbtft_lcd_rotate_90(src, dst, width, height);
            work_width = height;
            work_height = width;
            break;
            
        case ROTATE_180:
            fbtft_lcd_rotate_180(src, dst, width, height);
            break;
            
        case ROTATE_270:
            fbtft_lcd_rotate_270(src, dst, width, height);
            work_width = height;
            work_height = width;
            break;
    }
    
    // 然后进行镜像
    switch (mirror) {
        case MIRROR_NONE:
            // 不镜像
            break;
            
        case MIRROR_HORIZONTAL:
            fbtft_lcd_mirror_horizontal(work_buffer, work_width, work_height);
            break;
            
        case MIRROR_VERTICAL:
            fbtft_lcd_mirror_vertical(work_buffer, work_width, work_height);
            break;
            
        case MIRROR_BOTH:
            fbtft_lcd_mirror_horizontal(work_buffer, work_width, work_height);
            fbtft_lcd_mirror_vertical(work_buffer, work_width, work_height);
            break;
    }
}

/**
 * 自动适配缓冲区到LCD (包含旋转和缩放)
 */
int fbtft_lcd_auto_fit_buffer(fbtft_lcd_t *lcd, const uint16_t *src_buffer, 
                             int src_width, int src_height) {
    if (!lcd || !src_buffer || !lcd->fb_mem) {
        return -1;
    }
    
    // 如果源缓冲区大小与LCD完全匹配，直接复制
    if (src_width == lcd->width && src_height == lcd->height) {
        memcpy(lcd->fb_mem, src_buffer, lcd->width * lcd->height * sizeof(uint16_t));
        return 0;
    }
    
    // 如果源缓冲区是横屏(320x240)而LCD是竖屏(240x320)，进行旋转
    if (src_width == lcd->height && src_height == lcd->width) {
        uint16_t *temp_buffer = (uint16_t *)malloc(lcd->width * lcd->height * sizeof(uint16_t));
        if (!temp_buffer) return -1;
        
        // 90度旋转: 320x240 -> 240x320
        for (int y = 0; y < src_height; y++) {
            for (int x = 0; x < src_width; x++) {
                // (x, y) -> (y, src_width - 1 - x)
                int new_x = y;
                int new_y = src_width - 1 - x;
                if (new_x < lcd->width && new_y < lcd->height) {
                    temp_buffer[new_y * lcd->width + new_x] = src_buffer[y * src_width + x];
                }
            }
        }
        
        memcpy(lcd->fb_mem, temp_buffer, lcd->width * lcd->height * sizeof(uint16_t));
        free(temp_buffer);
        return 0;
    }
    
    // 简单缩放复制 (中心对齐)
    float scale_x = (float)lcd->width / src_width;
    float scale_y = (float)lcd->height / src_height;
    float scale = (scale_x < scale_y) ? scale_x : scale_y;
    
    int new_width = (int)(src_width * scale);
    int new_height = (int)(src_height * scale);
    int offset_x = (lcd->width - new_width) / 2;
    int offset_y = (lcd->height - new_height) / 2;
    
    // 清屏
    fbtft_lcd_clear(lcd, FBTFT_BLACK);
    
    // 缩放复制
    for (int y = 0; y < new_height; y++) {
        for (int x = 0; x < new_width; x++) {
            int src_x = (int)(x / scale);
            int src_y = (int)(y / scale);
            
            if (src_x < src_width && src_y < src_height) {
                int dst_x = x + offset_x;
                int dst_y = y + offset_y;
                if (dst_x >= 0 && dst_x < lcd->width && dst_y >= 0 && dst_y < lcd->height) {
                    lcd->fb_mem[dst_y * lcd->width + dst_x] = src_buffer[src_y * src_width + src_x];
                }
            }
        }
    }
    
    return 0;
}
