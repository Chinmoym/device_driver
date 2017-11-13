/***************************************************************************************

		Module to send on or off IR code through GPIO pin 23

***************************************************************************************/

#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/time.h>
#include <linux/errno.h>


#define KEY_POWER 0x0000000


MODULE_AUTHOR("Chinmoy Mohapatra");
MODULE_DESCRIPTION("IR code transmitter through GPIO-23");
MODULE_LICENSE("GPL");


int ret;
static int gpio_out_pin=23; //gpio out pin defaulted to 23

static struct gpio_chip *gpiochip;


module_param(gpio_out_pin, int, 0);
MODULE_PARM_DESC(gpio_out_pin, "GPIO output pin");



/***********************************IR functions**************************************/
static void sendNEC(long data, int nbits)
{
	printk("%d\n",gpio_out_pin);
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
		sendNEC(KEY_POWER,40); //send KEY_POWER
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
