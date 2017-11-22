#include <linux/module.h>
#include <linux/timer.h>
#include <linux/gpio.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <soc/bcm2835/raspberrypi-firmware.h>


MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Event Poller");
MODULE_AUTHOR("Chinmoy Mohapatra");

static int interval = 1000;//100 ms
module_param(interval, int, 0);
MODULE_PARM_DESC(interval, "poll time delay");

static int gpio_pin = 46;
module_param(gpio_pin, int, 0);
MODULE_PARM_DESC(gpio_pin, "GPIO PIN");

static struct gpio_chip *gpiochip;
static struct timer_list polltimer;
static struct rpi_firmware *mb_fw;
struct device_node *mbox;


int repeat_0=0,repeat_1=0;
static char *envp[2];

static void digi_release(struct device *dev)
{
	dev_alert(dev, "HDMI Module : releasing gpio device\n");
}

static struct platform_device hdmi_dev = {
        .name = "hdmi",
        .id = -1,
        .dev = {
                .release = digi_release,
               },
};

u32 mb_reg;
struct edid_packet {
	u32 blkno;
	u32 status;
	unsigned char edid_data[128];
};
/*static struct clk_pack {
	u32 id;
	u32 val;
} clk;
*/


static void poll_handler(unsigned long dummy)
{
        char *event = NULL;
        int value,ret,i;
	struct edid_packet edid;
	u32 addr=0;
        event = kasprintf(GFP_KERNEL, "TRIGGER=%s","connect");
	envp[0] = "hello";
        envp[1] = NULL;
	value = gpiochip->get(gpiochip,gpio_pin);
        //printk("My Module : value = %d\n",value);

	if (value==0)
        {
                repeat_1=0;
                if (repeat_0==0)
                {
                        printk("HDMI Module : HDMI Plugged\n");
                        repeat_0=1;
        	        if(event)
			{
                	        envp[0] = event;
                        	envp[1] = NULL;
                       		kobject_uevent_env(&hdmi_dev.dev.kobj, KOBJ_ADD, envp);
                        	kfree(event);
			}

			mbox = of_find_compatible_node(NULL,NULL,"raspberrypi,bcm2835-firmware");
        		if (mbox)
		        {
                		mb_fw = rpi_firmware_get(NULL);
		                printk("HDMI Module : RPI Firmware node found mailbox %p\n",mb_fw);

				edid.blkno=0;
				edid.status=0;
				for(i=0;i<128;i++)
					edid.edid_data[i]='\0';

				ret = rpi_firmware_property(mb_fw,RPI_FIRMWARE_GET_EDID_BLOCK,&edid,sizeof(edid));

			/*	for(i=0;i<128;i++)
				{
					printk("EDID[%d] = %c\n",i,edid.edid_data[i]);
					addr=0;
				}
				printk("\n");*/
				printk("HDMI Module : HDMI Device Name = %c%c%c%c\n",edid.edid_data[113],edid.edid_data[114],edid.edid_data[115],edid.edid_data[116]);
			}
		}
	}
        else
        {
                repeat_0=0;
                if (repeat_1==0)
                {
                        printk("HDMI Module : HDMI Unplugged\n");
                        repeat_1=1;
                        if(event)
			{
                        	envp[0] = event;
                        	envp[1] = NULL;
                       		kobject_uevent_env(&hdmi_dev.dev.kobj, KOBJ_REMOVE, envp);
                        	kfree(event);
			}
		}
	}

 	ret = mod_timer(&polltimer, jiffies+msecs_to_jiffies(interval));
        if (ret)
        {
                printk("HDMI Module : mod timer error\n");
		platform_device_unregister(&hdmi_dev);
		del_timer(&polltimer);
		return;
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
        printk("HDMI Module : gpio_pin = %d\n",gpio_pin);

        /*setting up GPIO pin*/
        if (!gpiochip)
                gpiochip = gpiochip_find("pinctrl-bcm2835",is_chip);
        if (!gpiochip)
                printk("HDMI Module : gpiochip not found\n");


	platform_device_register(&hdmi_dev);
	setup_timer(&polltimer,poll_handler, 0);

        ret = mod_timer(&polltimer ,msecs_to_jiffies(interval));
        if (ret)
        {
                printk("HDMI Module : Mod timer error\n");
                del_timer(&polltimer);
		platform_device_unregister(&hdmi_dev);
		return -1;
        }

        printk("HDMI Module : Module loaded\n");

	return 0;
}
module_init(poll_load);

static void __exit poll_unload(void)
{
	platform_device_unregister(&hdmi_dev);
        del_timer(&polltimer);
        printk("HDMI Module : Module unloaded\n");
}
module_exit(poll_unload);

