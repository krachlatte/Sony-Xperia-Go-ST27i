#include <linux/platform_device.h>
#include <linux/gpio_event.h>
#include <linux/gpio.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <plat/pincfg.h>
#include "pins-db8500.h"
#include "pins.h"

#define ROW_PIN_I0	164
#define ROW_PIN_I1	163
#define ROW_PIN_I2	162
#define ROW_PIN_I3	161
#define ROW_PIN_I4	156
#define ROW_PIN_I5	155
#define ROW_PIN_I6	154
#define ROW_PIN_I7	153

#define COL_PIN_O0	168
#define COL_PIN_O1	167
#define COL_PIN_O2	166
#define COL_PIN_O3	165
#define COL_PIN_O4	160
#define COL_PIN_O5	159
#define COL_PIN_O6	158

static pin_cfg_t gpio_keypad_pins[] = {
	GPIO153_GPIO | PIN_INPUT_NOPULL,
	GPIO154_GPIO | PIN_INPUT_PULLUP,
	GPIO155_GPIO | PIN_INPUT_PULLUP,
	GPIO156_GPIO | PIN_INPUT_NOPULL,
	GPIO161_GPIO | PIN_INPUT_NOPULL,
	GPIO162_GPIO | PIN_INPUT_PULLUP,
	GPIO163_GPIO | PIN_INPUT_PULLUP,
	GPIO164_GPIO | PIN_INPUT_NOPULL,
	GPIO157_GPIO | PIN_INPUT_PULLUP,
	GPIO158_GPIO | PIN_OUTPUT_LOW,
	GPIO159_GPIO | PIN_INPUT_NOPULL,
	GPIO160_GPIO | PIN_INPUT_NOPULL,
	GPIO165_GPIO | PIN_INPUT_NOPULL,
	GPIO166_GPIO | PIN_INPUT_NOPULL,
	GPIO167_GPIO | PIN_INPUT_NOPULL,
	GPIO168_GPIO | PIN_INPUT_NOPULL,
};

static struct gpio_event_direct_entry gpio_keypad_map[] = {
	{ .dev = 0, .gpio = ROW_PIN_I1, .code = KEY_VOLUMEDOWN },
	{ .dev = 0, .gpio = ROW_PIN_I2, .code = KEY_VOLUMEUP },
	{ .dev = 0, .gpio = ROW_PIN_I5, .code = KEY_CAMERA },
	{ .dev = 0, .gpio = ROW_PIN_I6, .code = KEY_CAMERA_FOCUS },
};

static struct gpio_event_input_info gpio_keypad_info = {
	.info.func = gpio_event_input_func,
	.info.no_suspend = true,
	.type = EV_KEY,
	.keymap = gpio_keypad_map,
	.keymap_size = ARRAY_SIZE(gpio_keypad_map),
	.debounce_time.tv64 = 20 * NSEC_PER_MSEC,
};

static struct gpio_event_info *keypad_info[] = {
	&gpio_keypad_info.info,
};

static int keypad_power(const struct gpio_event_platform_data *pdata, bool on)
{
	if (on)
		nmk_config_pins(gpio_keypad_pins, ARRAY_SIZE(gpio_keypad_pins));
	return 0;
}

static struct gpio_event_platform_data keypad_data = {
	.info		= keypad_info,
	.info_count	= ARRAY_SIZE(keypad_info),
	.power          = keypad_power,
	.names = {
		"ux500-ske-keypad",
		NULL
	},
};

static struct platform_device gpio_keypad_device = {
	.name	= GPIO_EVENT_DEV_NAME,
	.id	= -1,
	.dev	= {
		.platform_data	= &keypad_data,
	},
};

static int __init gpio_keypad_device_init(void)
{
	return platform_device_register(&gpio_keypad_device);
}

static void __exit gpio_keypad_device_exit(void)
{
	platform_device_unregister(&gpio_keypad_device);
}

module_init(gpio_keypad_device_init);
module_exit(gpio_keypad_device_exit);
