#include <linux/module.h>
#include <linux/timer.h>
#include <linux/gpio.h>
#include <linux/errno.h>
#include <linux/init.h>

MODULE_AUTHOR("Chinmoy Mohapatra");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Event Poller");


static int interval = 1000;//100 ms
module_param(interval, int, 0);
MODULE_PARM_DESC(interval, "poll time delay");

static int gpio_pin = 46;
module_param(gpio_pin, int, 0);
MODULE_PARM_DESC(gpio_pin, "GPIO PIN");

static struct gpio_chip *gpiochip;
static struct timer_list polltimer;

int repeat_0=0,repeat_1=0;

static void poll_handler(unsigned long dummy)
{
	int value,ret;
	value = gpiochip->get(gpiochip,gpio_pin);
	//printk("My Module : value = %d\n",value);
	if (value==0)
	{
		repeat_1=0;
		if (repeat_0==0)
		{
			printk("My Module : value = %d HDMI Plugged\n",value);
			repeat_0=1;
		}
		gpio_request(gpio_pin,"HDMI");
		gpio_export(gpio_pin, 0);
	}
	else
	{
		repeat_0=0;
		if (repeat_1==0)
		{
			printk("My Module : value = %d HDMI Unplugged\n",value);
			repeat_1=1;
                }
		gpio_unexport(gpio_pin);
		gpio_free(gpio_pin);
	}

	ret = mod_timer(&polltimer, jiffies+msecs_to_jiffies(interval));
	if (ret)
	{
		printk("mod timer error\n");
	}
	return;
}


//match function to find gpiochip
static int is_chip(struct gpio_chip *chip, void * data)
{
	if (strcmp(data,chip->label)==0)
		return 1;
	return 0;
}

static int __init poll_load(void)
{
	int ret;
	printk("My Module : gpio_pin = %d\n",gpio_pin);

	/*setting up GPIO pin*/
	if (!gpiochip)
		gpiochip = gpiochip_find("pinctrl-bcm2835",is_chip);
	if (!gpiochip)
		printk("IR Module : gpiochip not found\n");

	setup_timer(&polltimer,poll_handler, 0);
	ret = mod_timer(&polltimer ,msecs_to_jiffies(interval));
	if (ret)
	{
		printk("error\n");
		del_timer(&polltimer);
	}

	printk("My Module : Module loaded\n");
	return 0;
}
module_init(poll_load);

static void __exit poll_unload(void)
{
//	int ret;
	del_timer(&polltimer);
//	ret = gpio_export(gpio_pin,0);
        gpio_unexport(gpio_pin);
//        ret = gpio_request(gpio_pin,"HDMI");
	gpio_free(gpio_pin);

	printk("My Module : Module unloaded\n");
}
module_exit(poll_unload);
