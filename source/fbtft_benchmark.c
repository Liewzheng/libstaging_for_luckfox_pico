#include "fbtft_benchmark.h"

static volatile int benchmark_running = 1;

/**
 * 信号处理器
 */
void benchmark_signal_handler(int sig) {
    (void)sig; // 避免未使用参数警告
    printf("\nBenchmark interrupted by user\n");
    benchmark_running = 0;
}

/**
 * 获取当前时间（毫秒）
 */
unsigned long long get_current_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (unsigned long long)(tv.tv_sec) * 1000 + (unsigned long long)(tv.tv_usec) / 1000;
}

/**
 * 扫描 pic 目录获取所有 BMP 文件
 */
int scan_bmp_files(ImageInfo images[], int max_images) {
    DIR *dir;
    struct dirent *entry;
    int count = 0;
    
    dir = opendir("./pic/");
    if (dir == NULL) {
        printf("Error: Cannot open pic directory\n");
        return 0;
    }
    
    printf("Scanning BMP files in ./pic/ directory:\n");
    
    while ((entry = readdir(dir)) != NULL && count < max_images) {
        // 检查文件扩展名是否为 .bmp
        int len = strlen(entry->d_name);
        if (len > 4 && strcasecmp(&entry->d_name[len-4], ".bmp") == 0) {
            // 使用更安全的路径拼接
            int result = snprintf(images[count].path, MAX_PATH_LEN, "./pic/%s", entry->d_name);
            if (result >= MAX_PATH_LEN) {
                printf("Warning: Path too long for %s, skipping\n", entry->d_name);
                continue;
            }
            images[count].valid = 1;
            printf("  [%d] %s\n", count + 1, images[count].path);
            count++;
        }
    }
    
    closedir(dir);
    printf("Found %d BMP files\n\n", count);
    return count;
}

/**
 * 简单的5x7点阵字体数据 (ASCII 32-126)
 */
static const unsigned char font_5x7[][7] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // ' ' (space)
    {0x04, 0x04, 0x04, 0x04, 0x00, 0x04, 0x00}, // '!'
    {0x0A, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00}, // '"'
    {0x0A, 0x1F, 0x0A, 0x1F, 0x0A, 0x00, 0x00}, // '#'
    {0x04, 0x0F, 0x14, 0x0E, 0x05, 0x1E, 0x04}, // '$'
    {0x18, 0x19, 0x02, 0x04, 0x08, 0x13, 0x03}, // '%'
    {0x0C, 0x12, 0x14, 0x08, 0x15, 0x12, 0x0D}, // '&'
    {0x04, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00}, // '''
    {0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02}, // '('
    {0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08}, // ')'
    {0x00, 0x0A, 0x04, 0x1F, 0x04, 0x0A, 0x00}, // '*'
    {0x00, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x00}, // '+'
    {0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x08}, // ','
    {0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00}, // '-'
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00}, // '.'
    {0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x00}, // '/'
    {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E}, // '0'
    {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E}, // '1'
    {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F}, // '2'
    {0x1F, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0E}, // '3'
    {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}, // '4'
    {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E}, // '5'
    {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E}, // '6'
    {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}, // '7'
    {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}, // '8'
    {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C}, // '9'
    {0x00, 0x04, 0x00, 0x00, 0x04, 0x00, 0x00}, // ':'
    {0x00, 0x04, 0x00, 0x00, 0x04, 0x04, 0x08}, // ';'
    {0x02, 0x04, 0x08, 0x10, 0x08, 0x04, 0x02}, // '<'
    {0x00, 0x00, 0x1F, 0x00, 0x1F, 0x00, 0x00}, // '='
    {0x08, 0x04, 0x02, 0x01, 0x02, 0x04, 0x08}, // '>'
    {0x0E, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04}, // '?'
    {0x0E, 0x11, 0x01, 0x0D, 0x15, 0x15, 0x0E}, // '@'
    {0x0E, 0x11, 0x11, 0x11, 0x1F, 0x11, 0x11}, // 'A'
    {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E}, // 'B'
    {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E}, // 'C'
    {0x1C, 0x12, 0x11, 0x11, 0x11, 0x12, 0x1C}, // 'D'
    {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F}, // 'E'
    {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10}, // 'F'
    {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F}, // 'G'
    {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}, // 'H'
    {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E}, // 'I'
    {0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C}, // 'J'
    {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}, // 'K'
    {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F}, // 'L'
    {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11}, // 'M'
    {0x11, 0x11, 0x19, 0x15, 0x13, 0x11, 0x11}, // 'N'
    {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}, // 'O'
    {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10}, // 'P'
    {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D}, // 'Q'
    {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11}, // 'R'
    {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E}, // 'S'
    {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}, // 'T'
    {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}, // 'U'
    {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04}, // 'V'
    {0x11, 0x11, 0x11, 0x15, 0x15, 0x1B, 0x11}, // 'W'
    {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11}, // 'X'
    {0x11, 0x11, 0x11, 0x0A, 0x04, 0x04, 0x04}, // 'Y'
    {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F}, // 'Z'
    {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E}, // '['
    {0x00, 0x10, 0x08, 0x04, 0x02, 0x01, 0x00}, // '\'
    {0x0E, 0x02, 0x02, 0x02, 0x02, 0x02, 0x0E}, // ']'
    {0x04, 0x0A, 0x11, 0x00, 0x00, 0x00, 0x00}, // '^'
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F}, // '_'
    {0x08, 0x04, 0x02, 0x00, 0x00, 0x00, 0x00}, // '`'
    {0x00, 0x00, 0x0E, 0x01, 0x0F, 0x11, 0x0F}, // 'a'
    {0x10, 0x10, 0x16, 0x19, 0x11, 0x11, 0x1E}, // 'b'
    {0x00, 0x00, 0x0E, 0x10, 0x10, 0x11, 0x0E}, // 'c'
    {0x01, 0x01, 0x0D, 0x13, 0x11, 0x11, 0x0F}, // 'd'
    {0x00, 0x00, 0x0E, 0x11, 0x1F, 0x10, 0x0E}, // 'e'
    {0x06, 0x09, 0x08, 0x1C, 0x08, 0x08, 0x08}, // 'f'
    {0x00, 0x00, 0x0F, 0x11, 0x11, 0x0F, 0x01}, // 'g'
    {0x10, 0x10, 0x16, 0x19, 0x11, 0x11, 0x11}, // 'h'
    {0x04, 0x00, 0x0C, 0x04, 0x04, 0x04, 0x0E}, // 'i'
    {0x02, 0x00, 0x06, 0x02, 0x02, 0x12, 0x0C}, // 'j'
    {0x10, 0x10, 0x12, 0x14, 0x18, 0x14, 0x12}, // 'k'
    {0x0C, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E}, // 'l'
    {0x00, 0x00, 0x1A, 0x15, 0x15, 0x11, 0x11}, // 'm'
    {0x00, 0x00, 0x16, 0x19, 0x11, 0x11, 0x11}, // 'n'
    {0x00, 0x00, 0x0E, 0x11, 0x11, 0x11, 0x0E}, // 'o'
    {0x00, 0x00, 0x1E, 0x11, 0x1E, 0x10, 0x10}, // 'p'
    {0x00, 0x00, 0x0D, 0x13, 0x0F, 0x01, 0x01}, // 'q'
    {0x00, 0x00, 0x16, 0x19, 0x10, 0x10, 0x10}, // 'r'
    {0x00, 0x00, 0x0E, 0x10, 0x0E, 0x01, 0x1E}, // 's'
    {0x08, 0x08, 0x1C, 0x08, 0x08, 0x09, 0x06}, // 't'
    {0x00, 0x00, 0x11, 0x11, 0x11, 0x13, 0x0D}, // 'u'
    {0x00, 0x00, 0x11, 0x11, 0x11, 0x0A, 0x04}, // 'v'
    {0x00, 0x00, 0x11, 0x11, 0x15, 0x15, 0x0A}, // 'w'
    {0x00, 0x00, 0x11, 0x0A, 0x04, 0x0A, 0x11}, // 'x'
    {0x00, 0x00, 0x11, 0x11, 0x0F, 0x01, 0x0E}, // 'y'
    {0x00, 0x00, 0x1F, 0x02, 0x04, 0x08, 0x1F}, // 'z'
};

/**
 * 简单文本绘制函数
 */
void draw_text_simple(uint16_t *buffer, int width, int height, int x, int y, 
                     const char *text, uint16_t color, uint16_t bg_color) {
    int char_width = 6;   // 字符宽度 (5像素 + 1像素间距)
    int char_height = 7;  // 字符高度
    int len = strlen(text);
    
    // 边界检查
    if (x < 0 || y < 0 || x >= width || y >= height) return;
    
    // 绘制背景矩形
    int text_width = len * char_width;
    int max_width = (x + text_width > width) ? width - x : text_width;
    int max_height = (y + char_height > height) ? height - y : char_height;
    
    for (int dy = 0; dy < max_height; dy++) {
        for (int dx = 0; dx < max_width; dx++) {
            int px = x + dx;
            int py = y + dy;
            if (px >= 0 && px < width && py >= 0 && py < height) {
                buffer[py * width + px] = bg_color;
            }
        }
    }
    
    // 绘制每个字符
    for (int i = 0; i < len && (x + i * char_width) < width; i++) {
        char c = text[i];
        int char_x = x + i * char_width;
        
        // 获取字符在字体表中的索引
        int font_index = -1;
        if (c >= 32 && c <= 122) { // 支持 ASCII 32-122 (空格到小写z)
            font_index = c - 32;
        }
        
        // 如果字符在支持范围内，绘制字符
        if (font_index >= 0 && font_index < (int)(sizeof(font_5x7) / sizeof(font_5x7[0]))) {
            const unsigned char *char_data = font_5x7[font_index];
            
            // 绘制字符的每一行
            for (int row = 0; row < char_height && (y + row) < height; row++) {
                unsigned char line_data = char_data[row];
                
                // 绘制字符的每一列
                for (int col = 0; col < 5 && (char_x + col) < width; col++) {
                    int px = char_x + col;
                    int py = y + row;
                    
                    if (px >= 0 && py >= 0 && px < width && py < height) {
                        // 检查当前位是否需要绘制
                        if (line_data & (1 << (4 - col))) {
                            buffer[py * width + px] = color;
                        }
                    }
                }
            }
        } else {
            // 不支持的字符，绘制一个方块
            for (int row = 0; row < char_height && (y + row) < height; row++) {
                for (int col = 0; col < 5 && (char_x + col) < width; col++) {
                    int px = char_x + col;
                    int py = y + row;
                    
                    if (px >= 0 && py >= 0 && px < width && py < height) {
                        if (row == 0 || row == char_height - 1 || col == 0 || col == 4) {
                            buffer[py * width + px] = color;
                        }
                    }
                }
            }
        }
    }
}

/**
 * 横屏文字绘制函数 - 将文字逆时针旋转90度以适配横屏显示
 */
void draw_text_landscape(uint16_t *buffer, int width, int height, int x, int y, 
                        const char *text, uint16_t color, uint16_t bg_color) {
    int char_width = 6;   // 字符宽度 (5像素 + 1像素间距)
    int char_height = 7;  // 字符高度
    int len = strlen(text);
    
    // 在横屏模式下，文字从右向左，从上到下绘制
    // 边界检查
    if (x < 0 || y < 0 || x >= width || y >= height) return;
    
    // 计算旋转后的文字区域
    int text_height = len * char_width;  // 横屏下文字的高度
    int text_width = char_height;       // 横屏下文字的宽度
    
    // 绘制背景矩形
    int max_width = (x + text_width > width) ? width - x : text_width;
    int max_height = (y + text_height > height) ? height - y : text_height;
    
    for (int dy = 0; dy < max_height; dy++) {
        for (int dx = 0; dx < max_width; dx++) {
            int px = x + dx;
            int py = y + dy;
            if (px >= 0 && px < width && py >= 0 && py < height) {
                buffer[py * width + px] = bg_color;
            }
        }
    }
    
    // 绘制每个字符（旋转90度）
    for (int i = 0; i < len && (y + i * char_width) < height; i++) {
        char c = text[i];
        int char_y = y + i * char_width;  // 横屏下字符的Y位置
        
        // 获取字符在字体表中的索引
        int font_index = -1;
        if (c >= 32 && c <= 122) { // 支持 ASCII 32-122
            font_index = c - 32;
        }
        
        // 如果字符在支持范围内，绘制字符
        if (font_index >= 0 && font_index < (int)(sizeof(font_5x7) / sizeof(font_5x7[0]))) {
            const unsigned char *char_data = font_5x7[font_index];
            
            // 绘制字符的每一行（旋转后变成列）
            for (int row = 0; row < char_height && (x + row) < width; row++) {
                unsigned char line_data = char_data[row];
                
                // 绘制字符的每一列（旋转后变成行）
                for (int col = 0; col < 5 && (char_y + (4 - col)) < height; col++) {
                    int px = x + row;
                    int py = char_y + (4 - col);  // 逆序排列以实现旋转
                    
                    if (px >= 0 && py >= 0 && px < width && py < height) {
                        // 检查当前位是否需要绘制
                        if (line_data & (1 << (4 - col))) {
                            buffer[py * width + px] = color;
                        }
                    }
                }
            }
        } else {
            // 不支持的字符，绘制一个方块
            for (int row = 0; row < char_height && (x + row) < width; row++) {
                for (int col = 0; col < 5 && (char_y + (4 - col)) < height; col++) {
                    int px = x + row;
                    int py = char_y + (4 - col);
                    
                    if (px >= 0 && py >= 0 && px < width && py < height) {
                        if (row == 0 || row == char_height - 1 || col == 0 || col == 4) {
                            buffer[py * width + px] = color;
                        }
                    }
                }
            }
        }
    }
}

/**
 * 显示FPS信息到屏幕
 */
void display_fps_info(fbtft_lcd_t *lcd, uint16_t *buffer, BenchmarkStats *stats) {
    char fps_text[64];
    char max_fps_text[64];
    char frames_text[64];
    char time_text[64];
    
    // 判断屏幕方向：如果宽度>高度为横屏，否则为竖屏
    int is_landscape = (lcd->width > lcd->height);
    
    if (is_landscape) {
        // 横屏模式：在右上角垂直显示信息
        int info_x = lcd->width - 50;  // 右边距50像素
        int info_y = 10;               // 上边距10像素
        int line_spacing = 60;         // 行间距
        
        // 清除信息显示区域
        fbtft_lcd_fill_rectangle(lcd, info_x - 10, info_y, lcd->width - 1, 
                                info_y + line_spacing * 4 + 30, FBTFT_WHITE);
        
        // 当前FPS
        snprintf(fps_text, sizeof(fps_text), "FPS: %.1f", stats->current_fps);
        draw_text_landscape(buffer, lcd->width, lcd->height, info_x, info_y, 
                           fps_text, FBTFT_RED, FBTFT_WHITE);
        
        // 最高FPS
        snprintf(max_fps_text, sizeof(max_fps_text), "MAX: %.1f", stats->max_fps);
        draw_text_landscape(buffer, lcd->width, lcd->height, info_x, info_y + line_spacing, 
                           max_fps_text, FBTFT_GREEN, FBTFT_WHITE);
        
        // 总帧数
        snprintf(frames_text, sizeof(frames_text), "Frames: %llu", stats->total_frames);
        draw_text_landscape(buffer, lcd->width, lcd->height, info_x, info_y + line_spacing * 2, 
                           frames_text, FBTFT_BLUE, FBTFT_WHITE);
        
        // 运行时间
        unsigned long long elapsed_time = stats->current_time_ms - stats->start_time_ms;
        snprintf(time_text, sizeof(time_text), "Time: %llu.%llu s", 
                elapsed_time / 1000, (elapsed_time % 1000) / 100);
        draw_text_landscape(buffer, lcd->width, lcd->height, info_x, info_y + line_spacing * 3, 
                           time_text, FBTFT_BLACK, FBTFT_WHITE);
    } else {
        // 竖屏模式：在左上角水平显示信息
        int info_width = lcd->width - 20;
        int info_height = 80;
        fbtft_lcd_fill_rectangle(lcd, 10, 10, 10 + info_width, 10 + info_height, FBTFT_WHITE);
        
        int info_x = 10;
        int info_y = 15;
        int line_height = 16;
        
        // 当前FPS
        snprintf(fps_text, sizeof(fps_text), "FPS: %.1f", stats->current_fps);
        draw_text_simple(buffer, lcd->width, lcd->height, info_x, info_y, 
                        fps_text, FBTFT_RED, FBTFT_WHITE);
        
        // 最高FPS
        snprintf(max_fps_text, sizeof(max_fps_text), "MAX: %.1f", stats->max_fps);
        draw_text_simple(buffer, lcd->width, lcd->height, info_x, info_y + line_height, 
                        max_fps_text, FBTFT_GREEN, FBTFT_WHITE);
        
        // 总帧数
        snprintf(frames_text, sizeof(frames_text), "Frames: %llu", stats->total_frames);
        draw_text_simple(buffer, lcd->width, lcd->height, info_x, info_y + line_height * 2, 
                        frames_text, FBTFT_BLUE, FBTFT_WHITE);
        
        // 运行时间
        unsigned long long elapsed_time = stats->current_time_ms - stats->start_time_ms;
        snprintf(time_text, sizeof(time_text), "Time: %llu.%llu s", 
                elapsed_time / 1000, (elapsed_time % 1000) / 100);
        draw_text_simple(buffer, lcd->width, lcd->height, info_x, info_y + line_height * 3, 
                        time_text, FBTFT_BLACK, FBTFT_WHITE);
    }
}

/**
 * 显示最终结果
 */
void display_final_results(fbtft_lcd_t *lcd, uint16_t *buffer, BenchmarkStats *stats, int image_count) {
    char result_text[64];
    
    // 清屏
    fbtft_lcd_clear(lcd, FBTFT_WHITE);
    
    int x = 20;
    int y = 30;
    int line_height = 25;
    
    // 标题
    draw_text_simple(buffer, lcd->width, lcd->height, x, y, 
                    "Benchmark Results", FBTFT_BLACK, FBTFT_WHITE);
    y += line_height * 2;
    
    // 总帧数
    snprintf(result_text, sizeof(result_text), "Total Frames: %llu", stats->total_frames);
    draw_text_simple(buffer, lcd->width, lcd->height, x, y, 
                    result_text, FBTFT_BLUE, FBTFT_WHITE);
    y += line_height;
    
    // 运行时间
    unsigned long long elapsed_time = stats->current_time_ms - stats->start_time_ms;
    snprintf(result_text, sizeof(result_text), "Time: %.1f sec", (double)elapsed_time / 1000.0);
    draw_text_simple(buffer, lcd->width, lcd->height, x, y, 
                    result_text, FBTFT_BLUE, FBTFT_WHITE);
    y += line_height;
    
    // 平均FPS
    snprintf(result_text, sizeof(result_text), "Avg FPS: %.1f", stats->average_fps);
    draw_text_simple(buffer, lcd->width, lcd->height, x, y, 
                    result_text, FBTFT_GREEN, FBTFT_WHITE);
    y += line_height;
    
    // 最大FPS
    snprintf(result_text, sizeof(result_text), "Max FPS: %.1f", stats->max_fps);
    draw_text_simple(buffer, lcd->width, lcd->height, x, y, 
                    result_text, FBTFT_RED, FBTFT_WHITE);
    y += line_height;
    
    // 图像数量
    snprintf(result_text, sizeof(result_text), "Images: %d", image_count);
    draw_text_simple(buffer, lcd->width, lcd->height, x, y, 
                    result_text, FBTFT_BLACK, FBTFT_WHITE);
    
    // 显示到LCD
    fbtft_lcd_display_buffer(lcd, buffer);
}

/**
 * 打印进度信息
 */
void print_progress(BenchmarkStats *stats) {
    if (stats->total_frames % 1000 == 0) {
        printf("Frames: %llu, Current FPS: %.1f, Max FPS: %.1f\n", 
               stats->total_frames, stats->current_fps, stats->max_fps);
    }
}

/**
 * 运行FBTFT LCD Benchmark
 */
void fbtft_benchmark_run(const display_config_t *config) {
    fbtft_lcd_t lcd;
    ImageInfo images[MAX_IMAGES];
    BenchmarkStats stats = {0};
    int image_count = 0;
    int current_image = 0;
    uint16_t *image_buffer = NULL;
    uint16_t *transform_buffer = NULL;
    BMPImage bmp_image;
    
    // 设置信号处理器
    signal(SIGINT, benchmark_signal_handler);
    
    printf("=== FBTFT LCD Benchmark Test ===\n");
    printf("Press Ctrl+C to stop the benchmark\n\n");
    
    // 扫描BMP文件
    image_count = scan_bmp_files(images, MAX_IMAGES);
    if (image_count == 0) {
        printf("Error: No BMP files found in ./pic/ directory\n");
        return;
    }
    
    // 初始化FBTFT LCD
    const char *fb_device = "/dev/fb1"; // 通常FBTFT设备是fb1
    if (fbtft_lcd_check_device(fb_device) == 0) {
        fb_device = "/dev/fb0"; // 如果fb1不存在，尝试fb0
        if (fbtft_lcd_check_device(fb_device) == 0) {
            printf("Error: No framebuffer device found\n");
            return;
        }
    }
    
    if (fbtft_lcd_init(&lcd, fb_device) != 0) {
        printf("Error: Failed to initialize FBTFT LCD\n");
        return;
    }
    
    // 打印LCD信息
    fbtft_lcd_print_info(&lcd);
    
    // 分配图像缓冲区
    size_t buffer_size = lcd.width * lcd.height * sizeof(uint16_t);
    image_buffer = (uint16_t *)malloc(buffer_size);
    if (!image_buffer) {
        printf("Error: Failed to allocate image buffer\n");
        fbtft_lcd_deinit(&lcd);
        return;
    }
    
    // 如果需要旋转或镜像，分配变换缓冲区
    if (config && (config->rotation != ROTATE_0 || config->mirror != MIRROR_NONE)) {
        transform_buffer = (uint16_t *)malloc(buffer_size);
        if (!transform_buffer) {
            printf("Error: Failed to allocate transform buffer\n");
            free(image_buffer);
            fbtft_lcd_deinit(&lcd);
            return;
        }
    }
    
    // 显示配置信息
    if (config) {
        printf("Display Configuration:\n");
        printf("  Rotation: %d degrees\n", config->rotation);
        const char *mirror_names[] = {"none", "horizontal", "vertical", "both"};
        printf("  Mirror: %s\n", mirror_names[config->mirror]);
        const char *fit_names[] = {"scale", "stretch", "auto"};
        printf("  Fit Mode: %s\n", fit_names[config->fit_mode]);
        printf("\n");
    }
    
    // 清屏并显示启动信息
    fbtft_lcd_clear(&lcd, FBTFT_WHITE);
    draw_text_simple(image_buffer, lcd.width, lcd.height, 50, 100, 
                    "FBTFT LCD Benchmark", FBTFT_BLACK, FBTFT_WHITE);
    draw_text_simple(image_buffer, lcd.width, lcd.height, 70, 130, 
                    "Starting...", FBTFT_RED, FBTFT_WHITE);
    fbtft_lcd_display_buffer(&lcd, image_buffer);
    sleep(2);
    
    printf("Starting benchmark...\n");
    stats.start_time_ms = get_current_time_ms();
    stats.running = 1;
    
    // 主benchmark循环
    while (benchmark_running && stats.running) {
        // 加载并显示当前图像
        if (images[current_image].valid) {
            // 清除缓冲区
            memset(image_buffer, 0, buffer_size);
            
            // 尝试加载BMP图像
            if (bmp_load(images[current_image].path, &bmp_image) == 0) {
                // 根据适配模式选择加载方式
                if (config && config->fit_mode == FIT_AUTO) {
                    // 自动旋转以最佳适配
                    bmp_convert_to_rgb565_smart_fit(&bmp_image, image_buffer, lcd.width, lcd.height, 1);
                } else if (config && config->fit_mode == FIT_STRETCH) {
                    // 拉伸填充整个屏幕
                    bmp_convert_to_rgb565_smart_fit(&bmp_image, image_buffer, lcd.width, lcd.height, 0);
                } else {
                    // 默认保持宽高比缩放
                    bmp_convert_to_rgb565(&bmp_image, image_buffer, lcd.width, lcd.height);
                }
                bmp_free(&bmp_image);
                
                // 应用图像变换（旋转和镜像）
                if (config && transform_buffer && 
                    (config->rotation != ROTATE_0 || config->mirror != MIRROR_NONE)) {
                    
                    // 将变换后的图像复制到变换缓冲区
                    fbtft_lcd_transform_buffer(image_buffer, transform_buffer, 
                                             lcd.width, lcd.height, 
                                             config->rotation, config->mirror);
                    
                    // 将变换缓冲区的内容复制回图像缓冲区
                    memcpy(image_buffer, transform_buffer, buffer_size);
                }
            } else {
                // BMP加载失败，显示错误信息
                fbtft_lcd_clear(&lcd, FBTFT_WHITE);
                draw_text_simple(image_buffer, lcd.width, lcd.height, 10, 50, 
                               "Failed to load image", FBTFT_RED, FBTFT_WHITE);
                // 截断文件名以适应屏幕
                char short_name[32];
                const char *filename = strrchr(images[current_image].path, '/');
                filename = filename ? filename + 1 : images[current_image].path;
                strncpy(short_name, filename, sizeof(short_name) - 1);
                short_name[sizeof(short_name) - 1] = '\0';
                draw_text_simple(image_buffer, lcd.width, lcd.height, 10, 70, 
                               short_name, FBTFT_RED, FBTFT_WHITE);
            }
            
            // 计算实时FPS
            stats.current_time_ms = get_current_time_ms();
            unsigned long long elapsed_time = stats.current_time_ms - stats.start_time_ms;
            
            if (stats.total_frames > 0 && elapsed_time > 0) {
                stats.current_fps = (double)stats.total_frames * 1000.0 / (double)elapsed_time;
                if (stats.current_fps > stats.max_fps) {
                    stats.max_fps = stats.current_fps;
                }
            }
            
            // 显示FPS信息（每FPS_UPDATE_INTERVAL帧更新一次以减少开销）
            if (stats.total_frames % FPS_UPDATE_INTERVAL == 0) {
                display_fps_info(&lcd, image_buffer, &stats);
            }
            
            // 显示到LCD
            fbtft_lcd_display_buffer(&lcd, image_buffer);
        }
        
        stats.total_frames++;
        
        // 切换到下一张图像
        current_image = (current_image + 1) % image_count;
        
        // 检查是否达到测试时间限制
        unsigned long long elapsed_time = stats.current_time_ms - stats.start_time_ms;
        if (elapsed_time / 1000 >= BENCHMARK_DURATION_SEC) {
            printf("Benchmark completed after %d seconds\n", BENCHMARK_DURATION_SEC);
            break;
        }
        
        // 打印进度信息
        print_progress(&stats);
    }
    
    // 计算最终统计
    stats.current_time_ms = get_current_time_ms();
    unsigned long long total_time = stats.current_time_ms - stats.start_time_ms;
    stats.average_fps = (double)stats.total_frames * 1000.0 / (double)total_time;
    
    // 显示最终结果
    display_final_results(&lcd, image_buffer, &stats, image_count);
    
    // 打印最终统计到控制台
    printf("\n=== FBTFT Benchmark Results ===\n");
    printf("Total Frames: %llu\n", stats.total_frames);
    printf("Total Time: %.1f seconds\n", (double)total_time / 1000.0);
    printf("Average FPS: %.1f\n", stats.average_fps);
    printf("Maximum FPS: %.1f\n", stats.max_fps);
    printf("Images tested: %d\n", image_count);
    printf("FB Device: %s\n", lcd.device_path);
    printf("Resolution: %dx%d\n", lcd.width, lcd.height);
    printf("===============================\n");
    
    // 等待5秒显示结果
    sleep(5);
    
    // 清理资源
    if (image_buffer) {
        free(image_buffer);
    }
    if (transform_buffer) {
        free(transform_buffer);
    }
    fbtft_lcd_deinit(&lcd);
    
    printf("FBTFT Benchmark completed successfully!\n");
}
