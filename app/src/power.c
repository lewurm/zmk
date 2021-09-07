/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr.h>
#include <kernel.h>
#include <power/power.h>

#include <logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/usb.h>
#include <zmk/activity.h>

#include <drivers/ext_power.h>

bool is_usb_power_present() {
#ifdef CONFIG_USB
    return zmk_usb_is_powered();
#else
    return false;
#endif /* CONFIG_USB */
}

#if IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW_EXT_POWER)
static const struct device *ext_power;
#endif

struct pm_state_info pm_policy_next_state(int32_t ticks) {
    if (zmk_activity_get_state() == ZMK_ACTIVITY_SLEEP && !is_usb_power_present()) {
#if IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW_EXT_POWER)
        if (ext_power == NULL) {
            ext_power = device_get_binding("EXT_POWER");
            if (ext_power == NULL) {
                LOG_ERR("Unable to retrieve ext_power device: EXT_POWER");
            }
        }
        if (ext_power != NULL) {
            int rc = ext_power_disable(ext_power);
            if (rc != 0) {
                LOG_ERR("Unable to disable EXT_POWER: %d", rc);
            }
        }
#endif
        return (struct pm_state_info){PM_STATE_SOFT_OFF, 0, 0};
    }

    return (struct pm_state_info){PM_STATE_ACTIVE, 0, 0};
}

__weak bool pm_policy_low_power_devices(enum pm_state state) { return pm_is_sleep_state(state); }
