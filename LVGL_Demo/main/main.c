#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_sntp.h"
#include "lvgl.h"
#include "lvgl_helpers.h"

#define TAG "LVGL_APP"

static lv_obj_t *label_time;

/* Update time every second */
static void update_time_task(void *arg) {
    while (1) {
        time_t now;
        struct tm timeinfo;
        char buf[32];

        time(&now);
        localtime_r(&now, &timeinfo);
        snprintf(buf, sizeof(buf), "%02d:%02d:%02d", 
                 timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        lv_label_set_text(label_time, buf);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* Initialize SNTP */
void initialize_sntp(void) {
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
}

/* Main app entry */
void app_main(void) {
    // Initialize NVS, SNTP, etc.
    initialize_sntp();

    // Initialize LVGL
    lv_init();
    lvgl_driver_init();

    static lv_color_t buf1[LV_HOR_RES_MAX * 40];
    static lv_disp_draw_buf_t draw_buf;
    lv_disp_draw_buf_init(&draw_buf, buf1, NULL, LV_HOR_RES_MAX * 40);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf = &draw_buf;
    disp_drv.flush_cb = disp_driver_flush;
    disp_drv.hor_res = 128;
    disp_drv.ver_res = 160;
    lv_disp_drv_register(&disp_drv);

    // Optional: backlight
    gpio_set_direction(15, GPIO_MODE_OUTPUT);
    gpio_set_level(15, 1);

    // Create UI
    lv_obj_t *label_title = lv_label_create(lv_scr_act());
    lv_label_set_text(label_title, "Lich nhac!");
    lv_obj_align(label_title, LV_ALIGN_TOP_MID, 0, 5);

    lv_obj_t *label_desc = lv_label_create(lv_scr_act());
    lv_label_set_text(label_desc, "Uong thuoc luc:");
    lv_obj_align(label_desc, LV_ALIGN_TOP_LEFT, 5, 30);

    label_time = lv_label_create(lv_scr_act());
    lv_label_set_text(label_time, "00:00:00");
    lv_obj_align(label_time, LV_ALIGN_TOP_LEFT, 5, 50);

    // Tick task
    const esp_timer_create_args_t lv_tick_timer_args = {
        .callback = &lv_tick_inc,
        .arg = (void *)1,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "lv_tick"
    };
    esp_timer_handle_t lv_tick_timer = NULL;
    esp_timer_create(&lv_tick_timer_args, &lv_tick_timer);
    esp_timer_start_periodic(lv_tick_timer, 1 * 1000); // 1ms

    // Create update task
    xTaskCreate(update_time_task, "update_time", 4096, NULL, 1, NULL);

    // Handle LVGL task
    while (1) {
        lv_task_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
