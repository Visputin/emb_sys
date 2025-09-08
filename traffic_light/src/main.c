#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

// Led pin configurations
static const struct gpio_dt_spec red = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);

// Red led thread initialization
#define STACKSIZE 500
#define PRIORITY 5
void red_led_task(void *, void *, void*);
void green_led_task(void *, void *, void*);
void yellow_led_task(void *, void *, void*);
K_THREAD_DEFINE(red_thread,STACKSIZE,red_led_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(green_thread,STACKSIZE,green_led_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(yellow_thread,STACKSIZE,yellow_led_task,NULL,NULL,NULL,PRIORITY,0,0);

// Global variables
int led_state = 0;	// 0 = red, 1 = yellow 2 = green, 3 = pause
int led_direction = 1;	// 1 = forward in sequence, -1 = backwards in sequence

// Main program
int main(void)
{
        init_led();

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

// Task to handle red led
void red_led_task(void *, void *, void*) {
	
	printk("Red led thread started\n");
	while (true) {
		if (led_state == 0) {

			gpio_pin_set_dt(&red,1);	// turn red on
			gpio_pin_set_dt(&green,0);	// make sure green is off

			k_sleep(K_SECONDS(2));		// keep red on for 2 seconds

			led_direction = 1;			// change direction to forward
			led_state = 1;				// change state to yellow
		}
		k_msleep(50);
	}
}

// Task to handle yellow led
void yellow_led_task(void *, void *, void*) {
	
	printk("Yellow led thread started\n");
	while (true) {
		if (led_state == 1) {

			// turn green and red on to make yellow
			gpio_pin_set_dt(&green,1);
			gpio_pin_set_dt(&red,1);

			k_sleep(K_SECONDS(2));		// keep yellow on for 2 seconds

			// check wether to go forward or backwards in sequence
			if (led_direction == -1) {
				led_state = 0;
			}
			else {
				led_state = 2;
			}
		}
		k_msleep(50);
	}
}

// Task to handle green led
void green_led_task(void *, void *, void*) {
	
	printk("Green led thread started\n");
	while (true) {
		if (led_state == 2) {
			
			gpio_pin_set_dt(&green,1);		// turn green on
			gpio_pin_set_dt(&red,0);		// make sure red is off

			k_sleep(K_SECONDS(2));			// keep green on for 2 seconds

			// change direction to backwards
			led_direction = -1;				
			led_state = 1;
		}
		k_msleep(50);
	}
}

