// Created by Zhigang An on Apr. 15, 2017.
// Case Western Reserve university
// 
// Compile: make
// Insert module: sudo insmod blink_km.ko
// See the info printed: sudo tail -f /var/log/kern.log
// Remove module: sudo rmmod blink_km.ko
//
// blink an LED with dip button.

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zhigang An");
MODULE_DESCRIPTION("A LED blink with button kernel module.");
MODULE_VERSION("1.0");

#define BUTTON_PIN	19
#define LED_PIN		20

#define HIGH	1
#define LOW		0

#define IRQF_TRIGGER_EDGE	(IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING)

static unsigned int irqNumber;

// GPIO interrupt Handler.
// Note that the same interrupt handler cannot be invoked concurrently,
// as the interrupt line is masked out until the function is complete.
// Input: 
//     irq    IRQ number.
//     dev_id device ID.
//     regs   hardware specific register values. (For debugging)
// Output:
//     Returns IRQ_HANDLED if successful, IRQ_NONE if failed.
static irq_handler_t interruptHandler(unsigned int irq, void *dev_id, struct pt_regs *regs)
{
	// Not necessary to resgister interrupt as edge triggered here.
	// Just to illustrate how to handle edge triggered interrupt.
	// (This way is useful for DHT11 sensor.)
	// Get value to determine rising/falling edge.
	if (gpio_get_value(BUTTON_PIN) == HIGH) {
		// Rising edge.
		gpio_set_value(LED_PIN, LOW);
	} else {
		// falling edge.
		gpio_set_value(LED_PIN, HIGH);
	}
	
	// Tell caller interrupt has been handled correctly.
	return (irq_handler_t)IRQ_HANDLED;
}

static int __init initialization(void)
{
	// Declare variable at the very beginning of a scope to silence warning.
	int result = 0;
	
	// Check whether the GPIO is valid.
	if (!gpio_is_valid(LED_PIN)){
		printk(KERN_INFO "blink_km: Initialization failed. Invalid LED GPIO.\n");
		return -ENODEV;
	}
	
	// Request GPIO pin.
	gpio_request(LED_PIN, "sysfs");
	// Set direction as output, and value is high voltage.
	gpio_direction_output(LED_PIN, HIGH);
	// Make LED GPIO visible in /sys/class/gpio. Prevent direction from being changed (false).
	gpio_export(LED_PIN, false);
	// Do the same for button GPIO pin.
	gpio_request(BUTTON_PIN, "sysfs");
	// Set direction as input.
	gpio_direction_input(BUTTON_PIN);
	// Debounce button witha delay of 200 ms.
	gpio_set_debounce(BUTTON_PIN, 200);
	// The same as LED pin.
	gpio_export(BUTTON_PIN, false);
	
	// Store IRQ number for later use.
	irqNumber = gpio_to_irq(BUTTON_PIN);
	
	// Request an interrupt and register handeler function.
	result = request_irq(irqNumber, (irq_handler_t)interruptHandler, IRQF_TRIGGER_EDGE, "blink_km", NULL);
	
	printk(KERN_INFO "blink_km: Initialization succeeded.");
	return result;
}

static void __exit finalization(void)
{
	// Free interrupt.
	free_irq(irqNumber, NULL);
	
	// Turn off LED.
	gpio_set_value(LED_PIN, 0);
	// Unexport LED GPIO.
	gpio_unexport(LED_PIN);
	// Unexport Button GPIO.
	gpio_unexport(BUTTON_PIN);
	// Free LED GPIO.
	gpio_free(LED_PIN);
	// Free Button GPIO.
	gpio_free(BUTTON_PIN);
	
	// Message shown in kern.log.
	printk(KERN_INFO "BLINK_km: Exit.\n");
}

// Specify initialization and finalization function.
module_init(initialization);
module_exit(finalization);
