#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

/* 获取 LED 和 按键 的设备树描述 */
#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)
#define LED2_NODE DT_ALIAS(led2)

#define SW0_NODE DT_ALIAS(sw0)
#define SW1_NODE DT_ALIAS(sw1)
#define SW2_NODE DT_ALIAS(sw2)

static const struct gpio_dt_spec leds[] = {
    GPIO_DT_SPEC_GET(LED0_NODE, gpios),
    GPIO_DT_SPEC_GET(LED1_NODE, gpios),
    GPIO_DT_SPEC_GET(LED2_NODE, gpios),
};

static const struct gpio_dt_spec buttons[] = {
    GPIO_DT_SPEC_GET(SW0_NODE, gpios),
    GPIO_DT_SPEC_GET(SW1_NODE, gpios),
    GPIO_DT_SPEC_GET(SW2_NODE, gpios),
};

/* 定义按键中断回调函数 */
static struct gpio_callback button_cb_data;

void button_pressed_cb(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    /* 根据按下的引脚触发对应的 LED 翻转 */
    if (pins & BIT(buttons[0].pin)) {
        gpio_pin_toggle_dt(&leds[0]); // WAKEUP 翻 LED1
    }
    if (pins & BIT(buttons[1].pin)) {
        gpio_pin_toggle_dt(&leds[1]); // TAMPER 翻 LED2
    }
    if (pins & BIT(buttons[2].pin)) {
        gpio_pin_toggle_dt(&leds[2]); // USER 翻 LED3
    }
}

int main(void)
{
    /* 1. 初始化 LED (输出) */
    for (int i = 0; i < ARRAY_SIZE(leds); i++) {
        gpio_pin_configure_dt(&leds[i], GPIO_OUTPUT_INACTIVE);
    }

    /* 2. 初始化按键 (输入) 并绑定中断 */
    for (int i = 0; i < ARRAY_SIZE(buttons); i++) {
        if (!gpio_is_ready_dt(&buttons[i])) {
            return -1;
        }
        gpio_pin_configure_dt(&buttons[i], GPIO_INPUT);
        
        /* 配置为双边沿触发（按下和松开都触发），或者只配下降沿 */
        gpio_pin_interrupt_configure_dt(&buttons[i], GPIO_INT_EDGE_TO_ACTIVE);
    }

    /* 3. 初始化回调结构体，并把三个按键的引脚绑到同一个回调函数上 */
    gpio_init_callback(&button_cb_data, button_pressed_cb,
                       BIT(buttons[0].pin) | BIT(buttons[1].pin) | BIT(buttons[2].pin));
    
    /* 4. 把回调注册到 GPIO A 和 C, B 端口（Zephyr 支持跨端口注册，直接注册到对应端口） */
    gpio_add_callback(buttons[0].port, &button_cb_data);
    gpio_add_callback(buttons[1].port, &button_cb_data);
    gpio_add_callback(buttons[2].port, &button_cb_data);

    while (1) {
        /* 主循环空闲，什么都不用干，中断会自动处理按键 */
        k_msleep(1000); 
    }
    
    return 0;
}