#ifndef _FBTFT_BENCHMARK_H_
#define _FBTFT_BENCHMARK_H_

#include "fbtft_lcd.h"
#include "bmp_loader.h"
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <dirent.h>

// Benchmark配置
#define MAX_IMAGES              10
#define MAX_PATH_LEN            256
#define BENCHMARK_DURATION_SEC  30
#define FPS_UPDATE_INTERVAL     100  // 每100帧更新一次FPS显示

// 图像适配模式
typedef enum {
    FIT_SCALE = 0,      // 保持宽高比缩放（默认）
    FIT_STRETCH = 1,    // 拉伸填充屏幕
    FIT_AUTO = 2        // 自动旋转以最佳适配
} fit_mode_t;

// 全局显示配置
typedef struct {
    rotation_t rotation;        // 旋转角度
    mirror_t mirror;           // 镜像方式
    fit_mode_t fit_mode;       // 图像适配模式
} display_config_t;

// 图像信息结构
typedef struct {
    char path[MAX_PATH_LEN];
    int valid;
} ImageInfo;

// Benchmark统计信息
typedef struct {
    unsigned long long total_frames;
    unsigned long long start_time_ms;
    unsigned long long current_time_ms;
    double current_fps;
    double max_fps;
    double average_fps;
    int running;
} BenchmarkStats;

// 函数声明
void fbtft_benchmark_run(const display_config_t *config);
void benchmark_signal_handler(int sig);
unsigned long long get_current_time_ms(void);
int scan_bmp_files(ImageInfo images[], int max_images);
void display_fps_info(fbtft_lcd_t *lcd, uint16_t *buffer, BenchmarkStats *stats);
void display_final_results(fbtft_lcd_t *lcd, uint16_t *buffer, BenchmarkStats *stats, int image_count);
void print_progress(BenchmarkStats *stats);
void draw_text_simple(uint16_t *buffer, int width, int height, int x, int y, 
                     const char *text, uint16_t color, uint16_t bg_color);

#endif /* _FBTFT_BENCHMARK_H_ */
