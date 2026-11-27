/*
 * SPDX-License-Identifier: MIT
 * Fri3d Camp 2026 badge — start-up work and the communicator keyboard
 *
 * The coprocessor firmware (optional): the CH32X035 coprocessor (buttons,
 * joystick, battery, LoRa reset, 3V3_AUX) gets its firmware from ZephCore:
 * before main(), the bundled image (the badge module's
 * firmware/coprocessor_2.0.4.bin, CONFIG_MFD_FRI3D_EXPANDER_FIRMWARE) is
 * installed over RVSWD when the coprocessor runs an older version or does not
 * answer at all (no firmware), then the badge restarts: the new firmware
 * resets the display, the touch controller and the LoRa module. Debug builds
 * of the coprocessor firmware report 255.255.255 and are left alone. Only
 * with coprocessor_update.conf (it needs the zephyr-rvswd module, see there).
 *
 * The communicator add-on's keyboard (optional, with communicator.conf and
 * its overlay; without them this part is left out): its typing keys arrive as
 * FRI3D_TEXT_KEY(<character>) (communicator_keys.h, board.overlay) and are
 * reported again as typed characters (zephcore_input_ascii.h), with Shift and
 * Caps Lock (US layout, as the add-on's firmware) applied.
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/mfd/fri3d_expander.h>
#include <zephyr/init.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/reboot.h>

#include <helpers/input/zephcore_input_ascii.h>

#include "communicator_keys.h"

LOG_MODULE_REGISTER(fri3d_board_init, CONFIG_ZEPHCORE_BOARD_LOG_LEVEL);

#ifdef CONFIG_MFD_FRI3D_EXPANDER_UPDATE
static int coprocessor_update(void)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(expander));
	uint8_t running[3], bundled[3];
	int ret;

	if (mfd_fri3d_expander_get_bundled_version(bundled) != 0) {
		return 0;
	}

	if (device_is_ready(dev)) {
		mfd_fri3d_expander_get_version(dev, running);
		if (memcmp(running, bundled, sizeof(bundled)) >= 0) {
			return 0;
		}
		LOG_INF("coprocessor firmware v%u.%u.%u is older than v%u.%u.%u, updating",
			running[0], running[1], running[2], bundled[0], bundled[1], bundled[2]);
	} else {
		LOG_WRN("no coprocessor answer, installing firmware v%u.%u.%u", bundled[0],
			bundled[1], bundled[2]);
	}

	ret = mfd_fri3d_expander_update_bundled(dev, NULL, NULL);
	if (ret != 0) {
		LOG_ERR("coprocessor update failed: %d", ret);
		return 0;
	}

	LOG_INF("coprocessor updated, restarting");
	k_msleep(100);
	sys_reboot(SYS_REBOOT_COLD);

	return 0;
}

SYS_INIT(coprocessor_update, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
#endif /* CONFIG_MFD_FRI3D_EXPANDER_UPDATE */

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(communicator_input)) && defined(CONFIG_INPUT_FRI3D_COMMUNICATOR)
/* With Shift (US layout): the characters that change other than the letters */
static char shifted(char c)
{
	static const char plain[] = "1234567890-=[]\\;',./`";
	static const char shift[] = "!@#$%^&*()_+{}|:\"<>?~";
	const char *p;

	if (c >= 'a' && c <= 'z') {
		return c - 'a' + 'A';
	}
	p = strchr(plain, c);

	return p != NULL ? shift[p - plain] : c;
}

static void communicator_key(struct input_event *evt, void *user_data)
{
	static bool left_shift, right_shift, caps_lock;
	/* the character each typing key went down as: its release reports the same one */
	static char down[0x7f - 0x20];
	uint16_t code = evt->code;
	char c, *sent;

	ARG_UNUSED(user_data);

	if (evt->type != INPUT_EV_KEY) {
		return;
	}

	switch (code) {
	case INPUT_KEY_LEFTSHIFT:
		left_shift = evt->value != 0;
		return;
	case INPUT_KEY_RIGHTSHIFT:
		right_shift = evt->value != 0;
		return;
	case INPUT_KEY_CAPSLOCK:
		if (evt->value != 0) {
			caps_lock = !caps_lock;
		}
		return;
	default:
		break;
	}

	if (code < FRI3D_TEXT_KEY(0x20) || code > FRI3D_TEXT_KEY(0x7e)) {
		/* the keys with a meaning of their own, and the characters reported below */
		return;
	}
	c = (char)(code - FRI3D_TEXT_KEY_BASE);
	sent = &down[c - 0x20];

	if (evt->value != 0) {
		bool shift = left_shift || right_shift;

		/* Caps Lock: letters only; Shift makes them lower case again */
		if (c >= 'a' && c <= 'z') {
			shift = shift != caps_lock;
		}
		*sent = shift ? shifted(c) : c;
		input_report_key(evt->dev, ZEPHCORE_INPUT_ASCII_BASE + *sent, 1, true, K_NO_WAIT);
	} else if (*sent != 0) {
		input_report_key(evt->dev, ZEPHCORE_INPUT_ASCII_BASE + *sent, 0, true, K_NO_WAIT);
		*sent = 0;
	}
}

INPUT_CALLBACK_DEFINE(DEVICE_DT_GET(DT_NODELABEL(communicator_input)), communicator_key, NULL);
#endif /* communicator_input */
