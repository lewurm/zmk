/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_sensor_rotate_super_tab

#include <device.h>
#include <drivers/behavior.h>
#include <logging/log.h>

#include <drivers/sensor.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

static int64_t last_timestamp = 0;
static struct k_delayed_work work;

void behavior_sensor_rotate_super_tab_work_handler(struct k_work *item) {
    LOG_DBG("hello world from work handler");
    LOG_DBG("uptime: 0x%08llx", k_uptime_get());
    uint32_t modkey = LGUI;
    ZMK_EVENT_RAISE(zmk_keycode_state_changed_from_encoded(modkey, false, k_uptime_get()));
    last_timestamp = 0;
}

static int behavior_sensor_rotate_super_tab_init(const struct device *dev) {
    static bool init_first_run = true;

    if (init_first_run) {
        k_delayed_work_init(&work, behavior_sensor_rotate_super_tab_work_handler);
    }
    init_first_run = false;
    return 0;
}

static int on_sensor_binding_triggered(struct zmk_behavior_binding *binding,
                                       const struct sensor_value value, int64_t timestamp) {
    uint32_t keycode;
    uint32_t timeout_ms = 350;
    LOG_DBG("inc keycode 0x%02X dec keycode 0x%02X", binding->param1, binding->param2);
    LOG_DBG("timestamp: 0x%08llx", timestamp);
    LOG_DBG("diff: %d", timestamp - last_timestamp);
    LOG_DBG("uptime: 0x%08llx", k_uptime_get());


    switch (value.val1) {
    case 1:
        keycode = binding->param1;
        break;
    case -1:
        keycode = binding->param2;
        break;
    default:
        return -ENOTSUP;
    }
    
    LOG_DBG("SEND %d", keycode);

    bool tabbing = false;
    if ((timestamp - last_timestamp) < timeout_ms) {
        tabbing = true;
        int todo = k_delayed_work_cancel(&work);
    } else {
        uint32_t modkey = LGUI;
        ZMK_EVENT_RAISE(zmk_keycode_state_changed_from_encoded(modkey, true, timestamp));
    }

    ZMK_EVENT_RAISE(zmk_keycode_state_changed_from_encoded(keycode, true, timestamp));

    last_timestamp = timestamp;
    k_delayed_work_submit(&work, K_MSEC(timeout_ms));

    // TODO: Better way to do this?
    k_msleep(5);

    return ZMK_EVENT_RAISE(zmk_keycode_state_changed_from_encoded(keycode, false, timestamp));
}

static const struct behavior_driver_api behavior_sensor_rotate_super_tab_driver_api = {
    .sensor_binding_triggered = on_sensor_binding_triggered};

#define KP_INST(n)                                                                                 \
    DEVICE_DT_INST_DEFINE(n, behavior_sensor_rotate_super_tab_init, device_pm_control_nop, NULL,   \
                          NULL, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                  \
                          &behavior_sensor_rotate_super_tab_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KP_INST)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
