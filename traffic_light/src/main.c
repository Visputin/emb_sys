#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

// ----------------------------
// LED state variables
// ----------------------------
int led_state = 0;          // 0=Red,1=Yellow,2=Green,3=Paused
int prev_led_state = 0;     // store state when paused
int led_direction = 1;      // 1=forward, -1=backward

// ----------------------------
// LED pins
// ----------------------------
static const struct gpio_dt_spec red   = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);

// ----------------------------
// Threads
// ----------------------------
#define STACKSIZE 500
#define PRIORITY 5

void red_led_task(void *, void *, void*);
void yellow_led_task(void *, void *, void*);
void green_led_task(void *, void *, void*);

// ----------------------------
// Condition variables & mutexes
// ----------------------------
K_MUTEX_DEFINE(red_mutex);
K_CONDVAR_DEFINE(red_signal);

K_MUTEX_DEFINE(yellow_mutex);
K_CONDVAR_DEFINE(yellow_signal);

K_MUTEX_DEFINE(green_mutex);
K_CONDVAR_DEFINE(green_signal);

// ----------------------------
// Button config
// ----------------------------
#define BUTTON_NODE DT_ALIAS(sw2)
static const struct gpio_dt_spec button_2 = GPIO_DT_SPEC_GET_OR(BUTTON_NODE, gpios, {0});
static struct gpio_callback button_2_cb;

// ----------------------------
// Button interrupt handler
// ----------------------------
void button_2_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    if (led_state != 3) {
        prev_led_state = led_state;
        led_state = 3;
        printk("Paused\n");
    } else {
        led_state = prev_led_state;
        printk("Resumed\n");

        // Broadcast the LED that was active when paused
        switch(prev_led_state) {
            case 0: k_condvar_broadcast(&red_signal); break;
            case 1: k_condvar_broadcast(&yellow_signal); break;
            case 2: k_condvar_broadcast(&green_signal); break;
        }
    }
}

// ----------------------------
// LED trigger helper
// ----------------------------
void trigger_next_led(int current) {
    if (led_state == 3) return; // paused

    switch(current) {
        case 0: k_condvar_broadcast(&yellow_signal); 
        break;
        case 1: 
            if (led_direction == 1)
                k_condvar_broadcast(&green_signal);
            else
                k_condvar_broadcast(&red_signal);
            break;
        case 2: k_condvar_broadcast(&yellow_signal); 
        break;
    }
}

// ----------------------------
// Initialize button & LEDs
// ----------------------------
int init_button() {
    int ret;

    if (!gpio_is_ready_dt(&button_2)) {
        printk("Error: button not ready\n");
        return -1;
    }

    ret = gpio_pin_configure_dt(&button_2, GPIO_INPUT);
    if (ret != 0) return -1;

    ret = gpio_pin_interrupt_configure_dt(&button_2, GPIO_INT_EDGE_TO_ACTIVE);
    if (ret != 0) return -1;

    gpio_init_callback(&button_2_cb, button_2_handler, BIT(button_2.pin));
    gpio_add_callback(button_2.port, &button_2_cb);

    printk("Button initialized\n");
    return 0;
}

int init_led() {
    if (gpio_pin_configure_dt(&red, GPIO_OUTPUT) < 0) return -1;
    if (gpio_pin_configure_dt(&green, GPIO_OUTPUT) < 0) return -1;

    gpio_pin_set_dt(&red, 0);
    gpio_pin_set_dt(&green, 0);

    printk("LEDs initialized\n");
    return 0;
}

// ----------------------------
// LED threads
// ----------------------------
void red_led_task(void *arg1, void *arg2, void *arg3) {
    while (1) {
        k_mutex_lock(&red_mutex, K_FOREVER);
        k_condvar_wait(&red_signal, &red_mutex, K_FOREVER);
        k_mutex_unlock(&red_mutex);

        if (led_state == 3) continue;

        led_state = 0;
        gpio_pin_set_dt(&red, 1);
        gpio_pin_set_dt(&green, 0);
        printk("Red ON\n");

        led_direction = 1;
        k_msleep(1000);

        trigger_next_led(0);
    }
}

void yellow_led_task(void *arg1, void *arg2, void *arg3) {
    while (1) {
        k_mutex_lock(&yellow_mutex, K_FOREVER);
        k_condvar_wait(&yellow_signal, &yellow_mutex, K_FOREVER);
        k_mutex_unlock(&yellow_mutex);

        if (led_state == 3) continue;

        led_state = 1;
        gpio_pin_set_dt(&red, 1);
        gpio_pin_set_dt(&green, 1); // Simulate Yellow
        printk("Yellow ON\n");

        k_msleep(1000);

        trigger_next_led(1);
    }
}

void green_led_task(void *arg1, void *arg2, void *arg3) {
    while (1) {
        k_mutex_lock(&green_mutex, K_FOREVER);
        k_condvar_wait(&green_signal, &green_mutex, K_FOREVER);
        k_mutex_unlock(&green_mutex);

        if (led_state == 3) continue;

        led_state = 2;
        gpio_pin_set_dt(&red, 0);
        gpio_pin_set_dt(&green, 1);
        printk("Green ON\n");

        led_direction = -1;
        k_msleep(1000);

        trigger_next_led(2);
    }
}

// ----------------------------
// Main
// ----------------------------
int main(void) {
    if (init_led() != 0) printk("LED init failed\n");
    if (init_button() != 0) printk("Button init failed\n");

    printk("Traffic light started. Press button to pause/resume.\n");

    // Small delay to allow threads to start
    k_msleep(10);

    // Start with Red LED
    k_condvar_broadcast(&red_signal);

    return 0;
}

// ----------------------------
// Threads
// ----------------------------
K_THREAD_DEFINE(red_thread, STACKSIZE, red_led_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(yellow_thread, STACKSIZE, yellow_led_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(green_thread, STACKSIZE, green_led_task, NULL, NULL, NULL, PRIORITY, 0, 0);
