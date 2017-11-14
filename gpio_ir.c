/***************************************************************************************

		Module to send on or off IR code through GPIO pin 22

***************************************************************************************/

#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/time.h>
#include <linux/errno.h>
#include <linux/timekeeping.h>
#include <linux/delay.h>

#define KEY_POWER 0x06F948B7

#ifndef MAX_UDELAY_MS
	#define MAX_UDELAY_US 5000
#else
	#define MAX_UDELAY_US (MAX_UDELAY_MS*1000)
#endif

#define ONE_PULSE	590
#define ONE_GAP		1653
#define ZERO_PULSE	590
#define ZERO_GAP	528
#define HEADER_PULSE	9036
#define HEADER_GAP	4449
#define STOP_PULSE	588
#define STOP_GAP	0


MODULE_AUTHOR("Chinmoy Mohapatra");
MODULE_DESCRIPTION("IR code transmitter through GPIO-22");
MODULE_LICENSE("GPL");


int ret;
static int gpio_out_pin=22; //gpio out pin defaulted to 22
static struct gpio_chip *gpiochip;

static bool sw_carrier = 1;


module_param(gpio_out_pin, int, 0);
MODULE_PARM_DESC(gpio_out_pin, "GPIO output pin");

static void gpio_setpin(int pin, int value)
{
	gpiochip->set(gpiochip, pin, value);
}

/***********************************IR functions**************************************/

static unsigned long read_current_time(void)
{
	struct timespec current_time;
	getnstimeofday(&current_time);
	return (current_time.tv_sec * 1000000) + (current_time.tv_nsec/1000);
}

static long send_sw_carrier(unsigned long length)
{
	unsigned long actual_t = 0, initial_t = 0;
	length *= 1000;
	initial_t = read_current_time();
	while(actual_t < length)
	{
		gpio_setpin(gpio_out_pin,1);
		udelay(8);
		gpio_setpin(gpio_out_pin,0);
		udelay(17);

		actual_t = (read_current_time() - initial_t) * 1000;
	}
	return 0;
}

static void my_delay(unsigned long u_secs)
{
	while (u_secs > MAX_UDELAY_US)
	{
		udelay(MAX_UDELAY_US);
		u_secs -= MAX_UDELAY_US;
	}
	udelay(u_secs);
}

static long send_pulse(unsigned long length)
{
	if (length <= 0)
		return 0;
	if (sw_carrier)
		return send_sw_carrier(length);
	else
	{
		gpio_setpin(gpio_out_pin,1);
		my_delay(length);
	}
	return 0;
}

static void send_space(long length)
{
	gpio_setpin(gpio_out_pin,0);
	if (length <= 0)
		return;
	my_delay(length);
}

static void sendIR(long data, int nbits)
{
	int i=0;
	//send header or start
	send_pulse(HEADER_PULSE);
	send_space(HEADER_GAP);
	//send data
	for(i=0;i<nbits;i++)
	{
		if (data & 0x80000000)
		{
			send_pulse(ONE_PULSE);
			send_space(ONE_GAP);
		}
		else
		{
			send_pulse(ZERO_PULSE);
			send_space(ZERO_GAP);
		}
		data = data<<1;
	}
	//send stop signal
	send_pulse(STOP_PULSE);
	send_space(STOP_GAP);
}

/***************************************IR end****************************************/


//define device
struct ir_dev {
	char *name;
	int button_power;
};
#define to_ir_dev(x) ((struct ir_dev *)((x)->platform_data))

//device attributes
/*attribute name*/
static ssize_t show_name(struct device *dev, struct device_attribute *attr, char *response);
static struct device_attribute attr_name = {
	.attr = {
		.name = "name", //name
		.mode = 0664, //permission
	},
	.show = show_name,
};
static ssize_t show_name(struct device *dev, struct device_attribute *attr, char *response)
{
	ssize_t result;
	struct platform_device *pdev = to_platform_device(dev);
	result = sprintf(response,"%s\n",pdev->name);
	return result;
}

/*attribute button_key*/
static ssize_t show_button(struct device *dev, struct device_attribute *attr, char *response);
static ssize_t store_button(struct device *dev, struct device_attribute *attr, const char *buf, size_t valsize);
static struct device_attribute attr_button_key = {
	.attr = {
		.name = "button_power", //name
		.mode = 0664, //permission
	},
	.show = show_button,
	.store = store_button,
};
static ssize_t show_button(struct device *dev, struct device_attribute *attr, char *response)
{
	ssize_t result;
	struct ir_dev *pdev = to_ir_dev(dev);
	result = sprintf(response,"%d\n",pdev->button_power);
	return result;
}
static ssize_t store_button(struct device *dev, struct device_attribute *attr, const char *buf,size_t valsize)
{
	long value;
	ssize_t result;
	result = sscanf(buf,"%ld",&value);
	if(result != 1)
		return -EINVAL;
	else
	{
		sendIR(KEY_POWER,32); //send KEY_POWER
		result = valsize;
		return result;
	}
}

//attribute group
static struct attribute *ir_attrs[] = {
	&attr_name.attr,
	&attr_button_key.attr,
	NULL
};
static struct attribute_group ir_attr_group = {
	.attrs = ir_attrs,
};
static const struct attribute_group *ir_attr_groups[] = {
	&ir_attr_group,
	NULL
};


//match function to find gpiochip
static int is_chip(struct gpio_chip *chip, void * data)
{
	if (strcmp(data,chip->label)==0)
		return 1;
	return 0;
}

//release function
static void ir_release(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	dev_alert(dev,"IR Module : Releasing gpio device %s\n", pdev->name);
}

//device structure
static struct ir_dev ir;
static struct platform_device ir_pdev = {
	.name	= "ir",
	.id	= 0,
	.dev 	= {
		.release	= ir_release,
		.groups		= ir_attr_groups,
	},
};


//module initialization
static int __init ir_startup(void)
{
	printk("KERNEL : IR Module : Loading module IR transmitter\n");
	ir_pdev.dev.platform_data = &ir;

	/*setting up GPIO pin*/
	if (!gpiochip)
		gpiochip = gpiochip_find("pinctrl-bcm2835",is_chip);
	if (!gpiochip)
		printk("IR Module : gpiochip not found\n");

	gpiochip->direction_output(gpiochip,gpio_out_pin,0);

	/*Registering IR device as platform device*/

	//struct ir_dev gdev;
	//gdev = (struct ir_dev *)ir_pdev.dev.platform_data;
	printk("KERNEL : IR Module : Registering ir device\n");
	ret = platform_device_register(&ir_pdev);
	if(ret)
		printk("IR Module : device failed to register\n");
	else
	{
		dev_alert(&ir_pdev.dev,"device registerd\n");
		printk("gpio_out_pin = %d\n",gpio_out_pin);
	}
	return ret;
}

// exit module
static void __exit ir_cleanup(void)
{
	printk("KERNEL : IR Module : Unloading IR module\n");
	dev_alert(&ir_pdev.dev, "unregistering device");
	platform_device_unregister(&ir_pdev);
}


module_init(ir_startup);
module_exit(ir_cleanup);
