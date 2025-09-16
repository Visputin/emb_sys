// ##############################################################################################################
// Week 4 assignment currently finished for 1 points total
// ##############################################################################################################

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>
#include <inttypes.h>
#include <zephyr/drivers/uart.h>
#include <string.h>
#include <ctype.h>
#include <zephyr/timing/timing.h>

// Global variables
int led_state = 0;                      // 0 = red, 1 = yellow, 2 = green, 3 = pause
int prev_led_state = 0;                 // store previous state when pausing
int led_direction = 1;                  // 1 = forward, -1 = backward
enum { AUTO, MANUAL } mode = AUTO;      // operation mode, MANUAL(through UART input) or AUTO
static char custom_seq[20];
static int seq_index = 0;
static int seq_len = 0;
uint64_t red_timing_ns = 0;
uint64_t yellow_timing_ns = 0;
uint64_t green_timing_ns = 0;

// Led pin configurations
static const struct gpio_dt_spec red   = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);

// Thread initializations
#define STACKSIZE 500
#define PRIORITY 5
void red_led_task(void *, void *, void*);
void yellow_led_task(void *, void *, void*);
void green_led_task(void *, void *, void*);

// Timing thread initialization

// Dispatcher FIFO
K_FIFO_DEFINE(dispatcher_fifo);

// Configure buttons
#define BUTTON_2 DT_ALIAS(sw2)
static const struct gpio_dt_spec BUTTON_2 = GPIO_DT_SPEC_GET_OR(BUTTON_2, gpios, {0});
static struct gpio_callback BUTTON_2_data;

// Button interrupt handler
void BUTTON_2_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    if (led_state != 3) {
        prev_led_state = led_state;
        led_state = 3;
        printk("Button pressed, pausing sequence\n");
    } else {
        led_state = prev_led_state;
        printk("Button pressed, resuming sequence\n");
    }
}

// Button initialization
int init_button() {
    int ret;
    if (!gpio_is_ready_dt(&BUTTON_2)) {
        printk("Error: button not ready\n");
        return -1;
    }

    ret = gpio_pin_configure_dt(&BUTTON_2, GPIO_INPUT);
    if (ret != 0) return -1;

    ret = gpio_pin_interrupt_configure_dt(&BUTTON_2, GPIO_INT_EDGE_TO_ACTIVE);
    if (ret != 0) return -1;

    gpio_init_callback(&BUTTON_2_data, BUTTON_2_handler, BIT(BUTTON_2.pin));
    gpio_add_callback(BUTTON_2.port, &BUTTON_2_data);
    printk("Button initialized\n");
    return 0;
}

// Initialize leds
int init_led() {
    if (gpio_pin_configure_dt(&red, GPIO_OUTPUT_ACTIVE) < 0) return -1;
    if (gpio_pin_configure_dt(&green, GPIO_OUTPUT_ACTIVE) < 0) return -1;

    gpio_pin_set_dt(&red, 0);
    gpio_pin_set_dt(&green, 0);

    printk("LEDs initialized\n");
    return 0;
}

// UART initialization
#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

int init_uart(void) {
    if (!device_is_ready(uart_dev)) return 1;
    return 0;
}

// Data in FIFO
struct data_t {
    void *fifo_reserved;
    char msg[20];
};

// LED Tasks
// Red LED
void red_led_task(void *, void *, void*) {
    printk("Red LED thread started\n");

    timing_start();

    while (true) {
        if (led_state == 3) { k_msleep(50); continue; }

        // Manual mode
        if (mode == MANUAL) {
            if (seq_index < seq_len && custom_seq[seq_index] == 'R') {
                gpio_pin_set_dt(&red, 1);
                gpio_pin_set_dt(&green, 0);

                printk("Red LED ON\n");

                k_msleep(1000);
                seq_index++;
                if (seq_index >= seq_len) mode = AUTO;
            }
            k_msleep(50);
            continue;
        }

        // AUTO sequence
        if (led_state == 0) {
            timing_t red_start_time = timing_counter_get();

            gpio_pin_set_dt(&red, 1);
            gpio_pin_set_dt(&green, 0);

            printk("RED ON\n");

            for (int i = 0; i < 100; i++) {
                if (led_state == 3) break;
                k_msleep(10);
                k_yield();
            }
            if (led_state != 3) {
                led_direction = 1;
                led_state = 1;
            }

            timing_t red_end_time = timing_counter_get();
            //timing_stop();
            red_timing_ns = timing_cycles_to_ns(timing_cycles_get(&red_start_time, &red_end_time));
            printk("Red task: %lld microseconds\n", red_timing_ns / 1000);
        }

        k_msleep(50);
    }
}

// Yellow LED
void yellow_led_task(void *, void *, void*) {
    printk("Yellow LED thread started\n");

    timing_start();

    while (true) {
        if (led_state == 3) { k_msleep(50); continue; }

        // Manual mode
        if (mode == MANUAL) {
            if (seq_index < seq_len && custom_seq[seq_index] == 'Y') {
                gpio_pin_set_dt(&red, 1);
                gpio_pin_set_dt(&green, 1);

                printk("Yellow LED ON\n");

                k_msleep(1000);
                seq_index++;
                if (seq_index >= seq_len) mode = AUTO;
            }
            k_msleep(50);
            continue;
        }

        // AUTO sequence
        if (led_state == 1) {
            timing_t yellow_start_time = timing_counter_get();

            gpio_pin_set_dt(&red, 1);
            gpio_pin_set_dt(&green, 1);
            printk("YELLOW ON\n");

            for (int i = 0; i < 100; i++) {
                if (led_state == 3) break;
                k_msleep(10);
                k_yield();
            }
            if (led_state != 3) {
                if (led_direction == 1)
                    led_state = 2;
                else
                    led_state = 0;
            }

            timing_t yellow_end_time = timing_counter_get();
            //timing_stop();
            yellow_timing_ns = timing_cycles_to_ns(timing_cycles_get(&yellow_start_time, &yellow_end_time));
            printk("Yellow task: %lld microseconds\n", yellow_timing_ns / 1000);
        }

        k_msleep(50);
    }
}

// Green LED
void green_led_task(void *, void *, void*) {
    printk("Green LED thread started\n");

    timing_start();

    while (true) {
        if (led_state == 3) { k_msleep(50); continue; }

        // Manual mode
        if (mode == MANUAL) {
            if (seq_index < seq_len && custom_seq[seq_index] == 'G') {
                gpio_pin_set_dt(&red, 0);
                gpio_pin_set_dt(&green, 1);

                printk("Green LED ON\n");

                k_msleep(1000);
                seq_index++;
                if (seq_index >= seq_len) mode = AUTO;
            }
            k_msleep(50);
            continue;
        }

        // AUTO sequence
        if (led_state == 2) {
            timing_t green_start_time = timing_counter_get();

            gpio_pin_set_dt(&red, 0);
            gpio_pin_set_dt(&green, 1);
            printk("GREEN ON\n");

            for (int i = 0; i < 100; i++) {
                if (led_state == 3) break;
                k_msleep(10);
                k_yield();
            }
            if (led_state != 3) {
                led_direction = -1;
                led_state = 1;
            }

            timing_t green_end_time = timing_counter_get();
            //timing_stop();
            green_timing_ns = timing_cycles_to_ns(timing_cycles_get(&green_start_time, &green_end_time));
            printk("Green task: %lld microseconds\n", green_timing_ns / 1000);
        }

        k_msleep(50);
    }
}

void calculate_timing() {
    uint64_t total_time = red_timing_ns + yellow_timing_ns + green_timing_ns;
    printk("Total LED cycle time: %lld microseconds WITH debug prints\n", total_time / 1000);
}

void monitor_timing() {
    calculate_timing();
    k_msleep(1000);
}

// UART Task
static void uart_task(void *unused1, void *unused2, void *unused3)
{
    char rc = 0;
    char uart_msg[20];
    memset(uart_msg, 0, sizeof(uart_msg));
    int uart_msg_cnt = 0;

    while (true) {
        if (uart_poll_in(uart_dev, &rc) == 0) {
            rc = toupper((unsigned char)rc);
            if (rc != '\r' && uart_msg_cnt < sizeof(uart_msg)-1) {
                uart_msg[uart_msg_cnt++] = rc;
            } else if (uart_msg_cnt > 0) {
                uart_msg[uart_msg_cnt] = '\0';

                struct data_t *buf = k_malloc(sizeof(struct data_t));
                if (buf) {
                    strncpy(buf->msg, uart_msg, sizeof(buf->msg)-1);
                    buf->msg[sizeof(buf->msg)-1] = '\0';
                    k_fifo_put(&dispatcher_fifo, buf);
                }

                uart_msg_cnt = 0;
                memset(uart_msg, 0, sizeof(uart_msg));
            }
        }
        k_msleep(10);
    }
}

// Dispatcher Task
static void dispatcher_task(void *unused1, void *unused2, void *unused3)
{
    while (true) {
        struct data_t *rec_item = k_fifo_get(&dispatcher_fifo, K_FOREVER);

        strncpy(custom_seq, rec_item->msg, sizeof(custom_seq)-1);
        custom_seq[sizeof(custom_seq)-1] = '\0';
        seq_len = strlen(custom_seq);
        seq_index = 0;
        mode = MANUAL;

        printk("Dispatcher received sequence: %s\n", custom_seq);
        
        k_free(rec_item);
    }
}

// Main
int main(void)
{
    init_led();
    init_button();
    timing_init();
    monitor_timing();

    if (init_uart() != 0) {
        printk("UART initialization failed!\n");
        return -1;
    }

    // Wait for everything to initialize and threads to start
    k_msleep(100);
    printk("Traffic light started. Type sequence via UART to override.\n");
    return 0;
}

// Threads
K_THREAD_DEFINE(uart_thread, STACKSIZE, uart_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(dis_thread, STACKSIZE, dispatcher_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(red_thread, STACKSIZE, red_led_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(yellow_thread, STACKSIZE, yellow_led_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(green_thread, STACKSIZE, green_led_task, NULL, NULL, NULL, PRIORITY, 0, 0);