/*
 * SPDX-License-Identifier: MIT
 * Fri3d Camp 2026 badge — keep the coprocessor on the bundled firmware
 *
 * The CH32X035 coprocessor (buttons, joystick, battery, LoRa reset, 3V3_AUX)
 * gets its firmware from ZephCore: before main(), the bundled image (the badge
 * module's firmware/coprocessor_2.0.4.bin, CONFIG_MFD_FRI3D_EXPANDER_FIRMWARE)
 * is installed over RVSWD when the coprocessor runs an older version or does
 * not answer at all (no firmware), then the badge restarts: the new firmware
 * resets the display, the touch controller and the LoRa module. Debug builds
 * of the coprocessor firmware report 255.255.255 and are left alone.
 *
 * Optional: built with coprocessor_update.conf (it needs the zephyr-rvswd
 * module, see there); without it this file is empty.
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/mfd/fri3d_expander.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/reboot.h>

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
