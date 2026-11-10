#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

/* 获取设备树中的 3 个 LED 引脚 */
#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)
#define LED2_NODE DT_ALIAS(led2)

static const struct gpio_dt_spec leds[] = {
    GPIO_DT_SPEC_GET(LED0_NODE, gpios),
    GPIO_DT_SPEC_GET(LED1_NODE, gpios),
    GPIO_DT_SPEC_GET(LED2_NODE, gpios),
};

/* 全亮 */
void app_led_all_on(void)
{
    for (int i = 0; i < ARRAY_SIZE(leds); i++) {
        gpio_pin_set_dt(&leds[i], 1);
    }
}

/* 全灭 */
void app_led_all_off(void)
{
    for (int i = 0; i < ARRAY_SIZE(leds); i++) {
        gpio_pin_set_dt(&leds[i], 0);
    }
}

/* 闪烁 N 次 */
void app_led_flash(uint8_t times)
{
    for(uint8_t i = 0; i < times; i++) {
        k_msleep(500);
        app_led_all_on();
        k_msleep(500);
        app_led_all_off();
    }
}

/* 流水灯任务 */
void app_led_flow_task(void)
{
    /* LED1亮，LED3灭 */
    gpio_pin_set_dt(&leds[0], 1); 
    gpio_pin_set_dt(&leds[2], 0); 
    k_msleep(500);

    /* LED2亮，LED1灭 */
    gpio_pin_set_dt(&leds[1], 1); 
    gpio_pin_set_dt(&leds[0], 0); 
    k_msleep(500);

    /* LED3亮，LED2灭 */
    gpio_pin_set_dt(&leds[2], 1); 
    gpio_pin_set_dt(&leds[1], 0); 
    k_msleep(500);
}

int main(void)
{
    /* 初始化 GPIO */
    for (int i = 0; i < ARRAY_SIZE(leds); i++) {
        if (!gpio_is_ready_dt(&leds[i])) {
            return -1;
        }
        gpio_pin_configure_dt(&leds[i], GPIO_OUTPUT_INACTIVE); // 默认低电平（灭）
    }

    /* 循环执行流水灯 */
    while (1) {
        app_led_flow_task();
    }
    
    return 0;
}