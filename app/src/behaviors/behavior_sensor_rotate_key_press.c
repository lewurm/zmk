/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_sensor_rotate_key_press

#include <device.h>
#include <drivers/behavior.h>
#include <logging/log.h>

#include <drivers/sensor.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

struct behavior_sensor_rotate_key_press_config {
    int32_t modifier_key;
    int32_t mod_timeout_ms;
};

// TODO:
// That does not work if multiple encoders with timeouts are used at the _same_
// time. Do we care?
static int64_t last_timestamp = 0;
static struct k_delayed_work work;

void behavior_sensor_rotate_key_press_work_handler(struct k_work *item) {
    const struct device *dev = device_get_binding(DT_INST_LABEL(0));
    const struct behavior_sensor_rotate_key_press_config *cfg = dev->config;
    LOG_DBG("hello world from work handler");
    LOG_DBG("uptime: 0x%08llx", k_uptime_get());
    /* timeout expired, release modifier */
    ZMK_EVENT_RAISE(
        zmk_keycode_state_changed_from_encoded(cfg->modifier_key, false, k_uptime_get()));
    last_timestamp = 0;
}

static int behavior_sensor_rotate_key_press_init(const struct device *dev) {
    static bool init_first_run = true;

    if (init_first_run) {
        init_first_run = false;
        k_delayed_work_init(&work, behavior_sensor_rotate_key_press_work_handler);
    }
    return 0;
}

static int on_sensor_binding_triggered(struct zmk_behavior_binding *binding,
                                       const struct sensor_value value, int64_t timestamp) {
    const struct device *dev = device_get_binding(binding->behavior_dev);
    const struct behavior_sensor_rotate_key_press_config *cfg = dev->config;

    uint32_t keycode;
    LOG_DBG("inc keycode 0x%02X dec keycode 0x%02X", binding->param1, binding->param2);

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

    bool mod_timeout_enabled = cfg->mod_timeout_ms != -1;

    if (mod_timeout_enabled) {
        if ((timestamp - last_timestamp) < cfg->mod_timeout_ms) {
            /* another input, restart timeout */
            k_delayed_work_cancel(&work);
        } else {
            /* first time, activate modifier key */
            ZMK_EVENT_RAISE(
                zmk_keycode_state_changed_from_encoded(cfg->modifier_key, true, timestamp));
        }
    }

    ZMK_EVENT_RAISE(zmk_keycode_state_changed_from_encoded(keycode, true, timestamp));

    if (mod_timeout_enabled) {
        last_timestamp = timestamp;
        k_delayed_work_submit(&work, K_MSEC(cfg->mod_timeout_ms));
    }

    // TODO: Better way to do this?
    k_msleep(5);

    return ZMK_EVENT_RAISE(zmk_keycode_state_changed_from_encoded(keycode, false, timestamp));
}

static const struct behavior_driver_api behavior_sensor_rotate_key_press_driver_api = {
    .sensor_binding_triggered = on_sensor_binding_triggered};

#define KP_INST(n)                                                                                 \
    const struct behavior_sensor_rotate_key_press_config                                           \
        behavior_sensor_rotate_key_press_config_##n = {                                            \
            .modifier_key = DT_INST_PROP(n, modifier_key),                                         \
            .mod_timeout_ms = DT_INST_PROP(n, mod_timeout_ms),                                     \
    };                                                                                             \
    DEVICE_DT_INST_DEFINE(n, behavior_sensor_rotate_key_press_init, device_pm_control_nop, NULL,   \
                          NULL, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                  \
                          &behavior_sensor_rotate_key_press_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KP_INST)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
