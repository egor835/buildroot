#include <linux/module.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/of_device.h>

#define POLL_MS 20

struct fd650 {
	struct input_dev *input;
	struct delayed_work work;

	struct gpio_desc *clk;
	struct gpio_desc *dat;

	u8 prev_keys;
};

static inline int fd650_dat_read(struct fd650 *fd)
{
	return gpiod_get_value(fd->dat);
}

static inline void fd650_delay(void)
{
	udelay(5);
}

static inline void fd650_clk(struct fd650 *fd, int v)
{
	gpiod_set_value(fd->clk, v);
	fd650_delay();
}

static inline void fd650_dat(struct fd650 *fd, int v)
{
	gpiod_set_value(fd->dat, v);
	fd650_delay();
}

static inline void fd650_start(struct fd650 *fd)
{
	fd650_dat(fd, 1); fd650_clk(fd, 1);
	fd650_dat(fd, 0); fd650_clk(fd, 0);
}

static inline void fd650_stop(struct fd650 *fd)
{
	fd650_dat(fd, 0); fd650_clk(fd, 1);
	fd650_dat(fd, 1);
}

static void fd650_write_byte(struct fd650 *fd, u8 data)
{
	int i = 0;

	for (i = 0; i < 8; i++) {
		fd650_dat(fd, data & 0x80);
		fd650_clk(fd, 1);
		fd650_clk(fd, 0);
		data <<= 1;
	}

	fd650_dat(fd, 1);
	fd650_clk(fd, 1);
	fd650_clk(fd, 0);
}

static u8 fd650_read_byte(struct fd650 *fd)
{
	u8 v = 0;
	int i = 0;

	for (i = 0; i < 8; i++) {
		fd650_clk(fd, 1);
		v <<= 1;
		if (fd650_dat_read(fd))
			v |= 1;
		fd650_clk(fd, 0);
	}

	return v;
}

static u8 fd650_read_keys(struct fd650 *fd)
{

	u8 data = 0;

	fd650_start(fd);
	fd650_write_byte(fd, 0x4F);   // READ KEY CMD
	data = fd650_read_byte(fd);
	fd650_stop(fd);

	return data;
}

static void fd650_poll(struct work_struct *work)
{
	struct fd650 *fd =
		container_of(work, struct fd650, work.work);

	u8 cur = fd650_read_keys(fd);
	u8 changed = fd->prev_keys ^ cur;

	if (changed) {
		//pr_info("fd650 raw keys: 0x%02X\n", cur);
		switch (cur) {
        	    case 0x77:
                        input_event(fd->input, EV_MSC, MSC_SCAN, 0x10081);
                        input_report_key(fd->input, KEY_POWER, 1);
                        break;
                    case 0x5F:
                        input_event(fd->input, EV_MSC, MSC_SCAN, 0xc0040);
                        input_report_key(fd->input, KEY_MENU, 1);
                        break;
                    case 0x4F:
                        input_event(fd->input, EV_MSC, MSC_SCAN, 0xc00e9);
                        input_report_key(fd->input, KEY_VOLUMEUP, 1); 
                        break;
                    case 0x47:
                        input_event(fd->input, EV_MSC, MSC_SCAN, 0xc00ea);
                        input_report_key(fd->input, KEY_VOLUMEDOWN, 1); 
                        break;
                    case 0x37:
                        input_event(fd->input, EV_MSC, MSC_SCAN, 0x10081);
                        input_report_key(fd->input, KEY_POWER, 0);  
                        break;
                    case 0x1F:
                        input_event(fd->input, EV_MSC, MSC_SCAN, 0xc0040);
                        input_report_key(fd->input, KEY_MENU, 0);
                        break;
                    case 0x0F:
                        input_event(fd->input, EV_MSC, MSC_SCAN, 0xc00e9);
                        input_report_key(fd->input, KEY_VOLUMEUP, 0);
                        break;
                    case 0x07:
                        input_event(fd->input, EV_MSC, MSC_SCAN, 0xc00ea);
                        input_report_key(fd->input, KEY_VOLUMEDOWN, 0);
                        break;
        	}
		input_sync(fd->input);
	}

	fd->prev_keys = cur;
	schedule_delayed_work(&fd->work, msecs_to_jiffies(POLL_MS));
}

static const struct of_device_id fd650_of_match[] = {
	{ .compatible = "fd650,bitbang" },
	{ }
};
MODULE_DEVICE_TABLE(of, fd650_of_match);

static int fd650_probe(struct platform_device *pdev)
{
	struct fd650 *fd;
	struct input_dev *input;
	struct device *dev = &pdev->dev;
	int err;

	fd = devm_kzalloc(dev, sizeof(*fd), GFP_KERNEL);
	if (!fd)
		return -ENOMEM;

	input = devm_input_allocate_device(dev);
	if (!input)
		return -ENOMEM;

	fd->clk = devm_gpiod_get(dev, "clk", GPIOD_OUT_LOW);
	if (IS_ERR(fd->clk))
		return PTR_ERR(fd->clk);

	fd->dat = devm_gpiod_get(dev, "dat", GPIOD_OUT_LOW);
	if (IS_ERR(fd->dat))
		return PTR_ERR(fd->dat);

	input->name = "fd650-keys";
	input->phys = "fd650/input0";
	input->id.bustype = BUS_HOST;

	__set_bit(EV_MSC, input->evbit);
	__set_bit(MSC_SCAN, input->mscbit);
	__set_bit(EV_KEY, input->evbit);
	__set_bit(KEY_POWER, input->keybit);
	__set_bit(KEY_MENU, input->keybit);
	__set_bit(KEY_VOLUMEUP, input->keybit);
	__set_bit(KEY_VOLUMEDOWN, input->keybit);

	fd->input = input;
	fd->prev_keys = 0xFF;

	err = input_register_device(input);
	if (err)
		return err;

	platform_set_drvdata(pdev, fd);

	INIT_DELAYED_WORK(&fd->work, fd650_poll);
	schedule_delayed_work(&fd->work, msecs_to_jiffies(POLL_MS));

	dev_info(dev, "fd650 input driver probed\n");
	return 0;
}

static int fd650_remove(struct platform_device *pdev)
{
	struct fd650 *fd = platform_get_drvdata(pdev);

	cancel_delayed_work_sync(&fd->work);
	return 0;
}

static struct platform_driver fd650_driver = {
	.probe  = fd650_probe,
	.remove = fd650_remove,
	.driver = {
		.name = "fd650",
		.of_match_table = fd650_of_match,
	},
};

module_platform_driver(fd650_driver);
MODULE_LICENSE("GPL");

