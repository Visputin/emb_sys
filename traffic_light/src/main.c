#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>
#include <inttypes.h>

// Global variables
int led_state = 0;						// 0 = red, 1 = yellow 2 = green, 3 = pause
int prev_led_state = 0;					// store previous state when pausing
int led_direction = 1;					// 1 = forward in sequence, -1 = backwards in sequence

// Led pin configurations
static const struct gpio_dt_spec red = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);

// Configure buttons
#define BUTTON_2 DT_ALIAS(sw2)
static const struct gpio_dt_spec BUTTON_2 = GPIO_DT_SPEC_GET_OR(BUTTON_2, gpios, {0});
static struct gpio_callback BUTTON_2_data;

// Button interrupt handler
void BUTTON_2_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	if (led_state != 3) {				// if not paused, pause and store previous state
		prev_led_state = led_state;
		led_state = 3;
		printk("Button pressed, pausing sequence\n");
	}
	else {								// if paused, resume previous state
		led_state = prev_led_state;
		printk("Button pressed, resuming sequence\n");
	}
}

// Red led thread initialization
#define STACKSIZE 500
#define PRIORITY 5
void red_led_task(void *, void *, void*);
void green_led_task(void *, void *, void*);
void yellow_led_task(void *, void *, void*);
K_THREAD_DEFINE(red_thread,STACKSIZE,red_led_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(green_thread,STACKSIZE,green_led_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(yellow_thread,STACKSIZE,yellow_led_task,NULL,NULL,NULL,PRIORITY,0,0);

// Main program
int main(void)
{
    init_led();
	init_button();

	return 0;
}

// Initialize leds
int  init_led() {

    // Led pin initializations
	int ret0 = gpio_pin_configure_dt(&red, GPIO_OUTPUT_ACTIVE);
	if (ret0 < 0) {

		printk("Error: Red Led configure failed\n");		
		return ret0;
	}
	
	int ret2 = gpio_pin_configure_dt(&green, GPIO_OUTPUT_ACTIVE);
	if (ret2 < 0) {

		printk("Error: Green Led configure failed\n");		
		return ret2;
	}

	// set all leds off
	gpio_pin_set_dt(&red,0);
	gpio_pin_set_dt(&green,0);

	printk("Leds initialized ok\n");
	
	return 0;
}

// Button initialization
int init_button() {

	int ret;
	if (!gpio_is_ready_dt(&BUTTON_2)) {
		printk("Error: button 0 is not ready\n");
		return -1;
	}

	ret = gpio_pin_configure_dt(&BUTTON_2, GPIO_INPUT);
	if (ret != 0) {
		printk("Error: failed to configure pin\n");
		return -1;
	}

	ret = gpio_pin_interrupt_configure_dt(&BUTTON_2, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		printk("Error: failed to configure interrupt on pin\n");
		return -1;
	}

	gpio_init_callback(&BUTTON_2_data, BUTTON_2_handler, BIT(BUTTON_2.pin));
	gpio_add_callback(BUTTON_2.port, &BUTTON_2_data);
	printk("Set up button 0 ok\n");
	
	return 0;
}

// Task to handle red LED
void red_led_task(void *, void *, void*) {
    printk("Red led thread started\n");

    while (true) {
        // Pause check at the start
        if (led_state == 3) {
            k_msleep(50);
            continue;
        }

        if (led_state == 0) {
            gpio_pin_set_dt(&red, 1);
            gpio_pin_set_dt(&green, 0);

            for (int i = 0; i < 100; i++) {  // 100 x 10ms = 1 second
                if (led_state == 3) break;   // stop if paused
                k_msleep(10);
                k_yield();
            }

            if (led_state != 3) {
                led_direction = 1;  // forward
                led_state = 1;      // move to yellow
            }
        }

        k_msleep(50);
    }
}

// Task to handle yellow LED
void yellow_led_task(void *, void *, void*) {
    printk("Yellow led thread started\n");

    while (true) {
        // Pause check at the start
        if (led_state == 3) {
            k_msleep(50);
            continue;
        }

        if (led_state == 1) {
            // Yellow = red + green on
            gpio_pin_set_dt(&red, 1);
            gpio_pin_set_dt(&green, 1);

            for (int i = 0; i < 100; i++) {  // 50 x 10ms = 1 second
                if (led_state == 3) break;  // stop if paused
                k_msleep(10);
                k_yield();
            }

            if (led_state != 3) {
                if (led_direction == 1)
                    led_state = 2;  // go to green
                else
                    led_state = 0;  // go back to red
            }
        }

        k_msleep(50);
    }
}

// Task to handle green LED
void green_led_task(void *, void *, void*) {
    printk("Green led thread started\n");

    while (true) {
        // Pause check at the start
        if (led_state == 3) {
            k_msleep(50);
            continue;
        }

        if (led_state == 2) {
            gpio_pin_set_dt(&red, 0);
            gpio_pin_set_dt(&green, 1);

            for (int i = 0; i < 100; i++) {  // 100 x 10ms = 1 second
                if (led_state == 3) break;   // stop if paused
                k_msleep(10);
                k_yield();
            }

            if (led_state != 3) {
                led_direction = -1;  // backward
                led_state = 1;       // move to yellow
            }
        }

        k_msleep(50);
    }
}


