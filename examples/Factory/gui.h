/**
 * @file      gui.h
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2023  Shenzhen Xinyuan Electronic Technology Co., Ltd
 * @date      2023-10-12
 *
 */

#pragma once
enum {
    LV_MSG_WIFI_GET_IP,
};

void initGUI();
void serialToScreen(lv_obj_t *parent, String string, bool result);
