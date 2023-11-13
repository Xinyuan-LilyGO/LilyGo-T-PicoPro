/**
 * @file      gui.cpp
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2023  Shenzhen Xin Yuan Electronic Technology Co., Ltd
 * @date      2023-10-16
 *
 */

#include <Arduino.h>
#include <lvgl.h>
#include <time.h>
#include <XPowersLib.h>
#include <WiFiEspAT.h>
#include "utilities.h"
#include "gui.h"
#include "esp_gpio.h"

LV_FONT_DECLARE(font_ali);
LV_IMG_DECLARE(wallpaper);
LV_IMG_DECLARE(wallpaper1);
LV_IMG_DECLARE(wallpaper2);
LV_IMG_DECLARE(wallpaper3);

LV_IMG_DECLARE(maintenance_128);
LV_IMG_DECLARE(music_128);
LV_IMG_DECLARE(settings_128);
LV_IMG_DECLARE(compass_128);
LV_IMG_DECLARE(gear_128);
LV_IMG_DECLARE(fatigue_128);
LV_IMG_DECLARE(management_128);
LV_IMG_DECLARE(performance_128);
LV_IMG_DECLARE(icon_back);

static lv_obj_t *hour_text;
static lv_obj_t *min_text;
static uint8_t pageId = 0;
static uint8_t randWallpaperIndex = 0;

extern String getModel();
extern String macAddress();
extern PowersSY6970 PMU;

const lv_img_dsc_t *wallpaper_ptr[4] = {&wallpaper, &wallpaper1, &wallpaper2, &wallpaper3};

#define UI_FONT_COLOR       lv_color_make(255, 255, 255)

extern void sensor_timer_callback(lv_timer_t *t);
static void make_clock_gui(lv_obj_t *parent);
static void make_menu_list_gui(lv_obj_t *parent);
static lv_obj_t *make_sensor_info_gui(lv_obj_t *parent);
static lv_obj_t *make_device_info_gui(lv_obj_t *parent);
static lv_obj_t *make_pmu_info_gui(lv_obj_t *parent);
static lv_obj_t *make_wifi_pins_gui(lv_obj_t *parent);

static void core_temp_timer_callback(lv_timer_t *t)
{
    lv_label_set_text_fmt((lv_obj_t * )t->user_data, "%2.1f°C",  analogReadTemp());
}

void serialToScreen(lv_obj_t *parent, String string,  bool result)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_scroll_dir(cont, LV_DIR_NONE);
    lv_obj_set_size(cont, LV_PCT(100), lv_font_get_line_height(&lv_font_montserrat_28) + 2 );

    lv_obj_t *label1 = lv_label_create(cont);
    lv_label_set_recolor(label1, true);
    lv_label_set_text(label1, string.c_str());
    lv_obj_align(label1, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_t *label = lv_label_create(cont);
    lv_label_set_recolor(label, true);
    lv_label_set_text(label, result ? "#FFFFFF [# #00ff00 PASS# #FFFFFF ]#" : "#FFFFFF [# #ff0000  FAIL# #FFFFFF ]#");
    lv_obj_align(label, LV_ALIGN_RIGHT_MID, 0, 0);

    lv_obj_scroll_to_y(parent, lv_disp_get_ver_res(NULL), LV_ANIM_ON);
    int i = 200;
    while (i--) {
        lv_task_handler();
        delay(1);
    }
}

static void updateDateTimeTask(lv_timer_t *args)
{
    static bool val;
    lv_obj_t *obj = (lv_obj_t *)args->user_data;
    val ?  lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN) : lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
    val ^= 1;

    struct tm timeinfo;
    time_t now;
    time(&now);
    localtime_r(&now, &timeinfo);
    lv_label_set_text_fmt(hour_text, "%02d", timeinfo.tm_hour);
    lv_label_set_text_fmt(min_text, "%02d", timeinfo.tm_min);
}

static void tileview_change_cb(lv_event_t *e)
{
    static uint8_t lastId = 0;
    static uint16_t lastPageID = 0;
    lv_obj_t *tileview = lv_event_get_target(e);

    pageId = lv_obj_get_index(lv_tileview_get_tile_act(tileview));
    lv_event_code_t c = lv_event_get_code(e);
    Serial.print("Code : ");
    Serial.print(c);
    uint32_t count =  lv_obj_get_child_cnt(tileview);
    Serial.print(" Count:");
    Serial.print(count);
    Serial.print(" pageId:");
    Serial.println(pageId);

    lastId = pageId;
}

void initGUI()
{
    randomSeed(analogRead(29));
    randWallpaperIndex = random(0, 4);
    lv_obj_set_style_bg_img_src(lv_scr_act(), wallpaper_ptr[randWallpaperIndex], LV_PART_MAIN);

    lv_obj_t *tileview = lv_tileview_create(lv_scr_act());
    lv_obj_align(tileview, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_size(tileview, LV_PCT(100), LV_PCT(100));
    lv_obj_remove_style(tileview, 0, LV_PART_SCROLLBAR);
    lv_obj_add_event_cb(tileview, tileview_change_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *tv1 = lv_tileview_add_tile(tileview, 0, 0, LV_DIR_VER);
    lv_obj_t *tv2 = lv_tileview_add_tile(tileview, 0, 1, LV_DIR_VER);

    lv_obj_set_style_bg_opa(tileview, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(tv1, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(tv2, LV_OPA_TRANSP, LV_PART_MAIN);

    make_clock_gui(tv1);
    make_menu_list_gui(tv2);

}

static void lv_device_switch_event_handler(lv_event_t *e)
{
    lv_event_code_t event = lv_event_get_code(e);
    if (event != LV_EVENT_VALUE_CHANGED )return;

    lv_obj_t *sw = lv_event_get_target(e);
    lv_state_t  state = lv_obj_get_state(sw);

    Serial.print("STATE:");
    Serial.println(state, HEX);
    switch (*(uint8_t *)lv_event_get_user_data(e)) {
    // "OTG Power"
    case 0:
        if (state == LV_STATE_FOCUSED) {
            Serial.println("OTG DISABLE");
            PMU.disableOTG();
        } else {
            if (!PMU.enableOTG()) {
                Serial.println("Enable OTG Failed!");
                lv_obj_clear_state(sw, LV_STATE_CHECKED);
            } else {
                Serial.println("OTG ENABLE");
            }
        }
        break;
    // "State LED Enable"
    case 1:
        if (state == LV_STATE_FOCUSED) {
            Serial.println("LED DISABLE");
            PMU.disableStatLed();
        } else {
            PMU.enableStatLed();
            Serial.println("LED ENABLE");
        }
        break;
    case 2:
        if (state == LV_STATE_FOCUSED) {
            Serial.println("CHARGE DISABLE");
            PMU.disableCharge();
        } else {
            PMU.enableCharge();
            Serial.println("CHARGE ENABLE");
        }
        break;
    default:
        break;
    }
}

static bool get_pmu_otg_enable()
{
    return PMU.isOTG();
}

static bool get_pmu_state_led_enable()
{
    return PMU.isEnableStatLed();
}

static bool get_pmu_charge_enable()
{
    return PMU.isEnableCharge();
}

static const char *get_wifi_mac()
{
    return macAddress().c_str();
}

static const char *get_loacl_ip()
{
    if (WiFi.localIP().toString() == "") {
        return "None";
    }
    return WiFi.localIP().toString().c_str();
}

static const char *get_wifi_version()
{
    return WiFi.firmwareVersion();
}

static const char *get_wifi_model()
{
    return getModel().c_str();
}


static lv_obj_t  *make_menu_page_cont(lv_obj_t *parent, lv_obj_t **page, const  char *title)
{
    *page = lv_menu_page_create(parent, (char *)title);
    lv_obj_set_style_bg_opa(parent, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_t *cont = lv_menu_cont_create(*page);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_radius(cont, 0, 0);
    lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_center(cont);
    return cont;
}



static const char *btnm_map[] = {"0", "1", "2", "3", "4", "5", "6", "\n",
                                 "7", "8", "9", "10", "TX", "RX",  "\n",
                                 "ALL", "CLEAR", ""
                                };

static void btnm_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    uint16_t id = lv_btnmatrix_get_selected_btn(obj);
    bool checked =   lv_btnmatrix_get_one_checked(obj);
    Serial.printf("ID:%d checked:%d \n", id, checked);

    if (id > 14) {
        return;
    }
    if (id == 13) {
        // Set all
        for (int i = 0; i < EIO_MAX; ++i) {
            esp_digital_write(i, HIGH);
            if (esp_digital_read(i) == HIGH) {
                lv_btnmatrix_set_btn_ctrl(obj, i, LV_BTNMATRIX_CTRL_CHECKED);
            }
        }
    } else if (id == 14) {
        // Clean
        for (int i = 0; i < EIO_MAX; ++i) {
            esp_digital_write(i, LOW);
            if (esp_digital_read(i) == LOW) {
                lv_btnmatrix_clear_btn_ctrl(obj, i,  LV_BTNMATRIX_CTRL_CHECKED );
            }
        }
    }
}

static lv_obj_t *make_wifi_pins_gui(lv_obj_t *parent)
{
    lv_obj_t *page;
    lv_obj_t *cont = make_menu_page_cont(parent, &page, "WiFi Pins");

    lv_obj_t *btnm1 = lv_btnmatrix_create(cont);
    lv_btnmatrix_set_map(btnm1, btnm_map);
    // for (int i = 0; i < 14; ++i) {
    //     lv_btnmatrix_set_btn_width(btnm1, i, 4);
    //     lv_btnmatrix_set_btn_ctrl(btnm1, i, LV_BTNMATRIX_CTRL_CHECKABLE);
    //     lv_btnmatrix_set_btn_ctrl(btnm1, i, LV_BTNMATRIX_CTRL_CHECKED);
    // }
    lv_obj_align(btnm1, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(btnm1, btnm_event_cb, LV_EVENT_CLICKED, NULL);

    return page;
}

static lv_obj_t *make_sensor_info_gui(lv_obj_t *parent)
{
    lv_obj_t *page;
    lv_obj_t *cont = make_menu_page_cont(parent, &page, "Sensor Info");

    //SOC
    lv_obj_t *obj = lv_obj_create(cont);
    lv_obj_set_scroll_dir(obj, LV_DIR_NONE);
    lv_obj_set_size(obj, LV_PCT(100), LV_PCT(30));
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_50, LV_PART_MAIN);

    lv_obj_t *label = lv_label_create(obj);
    lv_label_set_text(label, "SOC");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 10, 0);

    label = lv_label_create(obj);
    lv_label_set_text_fmt(label, "%2.1f",  analogReadTemp());
    lv_obj_set_style_text_font(label, &lv_font_montserrat_18, 0);
    lv_obj_set_width(label, 100);
    lv_obj_align(label, LV_ALIGN_RIGHT_MID, -20, 0);
    lv_timer_create(core_temp_timer_callback, 1000, label);

    //Sensor
    obj = lv_obj_create(cont);
    lv_obj_set_scroll_dir(obj, LV_DIR_NONE);
    lv_obj_set_size(obj, LV_PCT(100), LV_PCT(30));
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_50, LV_PART_MAIN);

    label = lv_label_create(obj);
    lv_label_set_text(label, "Proximity:");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 10, 0);

    label = lv_label_create(obj);
    lv_label_set_text_fmt(label, "%u", 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);
    lv_obj_set_width(label, 80);
    lv_obj_align(label, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_timer_create(sensor_timer_callback, 1000, label);

    return page;
}

static lv_obj_t *make_pmu_info_gui(lv_obj_t *parent)
{
    lv_obj_t *sw;
    lv_obj_t *label;
    lv_obj_t *page;
    lv_obj_t *cont = make_menu_page_cont(parent, &page, "PMU Info");

    const char *swName[] = {"OTG Enable", "State LED Enable", "Charge Enable"};
    static uint8_t index[] = {0, 1, 2};

    typedef bool (*getStatue)();
    getStatue func[] = {get_pmu_otg_enable, get_pmu_state_led_enable, get_pmu_charge_enable};
    for (int i = 0; i < sizeof(swName) / sizeof(swName[0]); ++i) {

        lv_obj_t *obj = lv_obj_create(cont);
        lv_obj_set_scroll_dir(obj, LV_DIR_NONE);
        lv_obj_set_size(obj, LV_PCT(100), LV_PCT(15));
        lv_obj_set_style_border_width(obj, 0, 0);
        lv_obj_set_style_bg_opa(obj, LV_OPA_50, LV_PART_MAIN);

        sw = lv_switch_create(obj);
        lv_obj_add_state(sw, func[i]() ? LV_STATE_CHECKED : LV_STATE_DEFAULT );
        lv_obj_set_size(sw, 80, 40);
        lv_obj_align(sw, LV_ALIGN_RIGHT_MID, -20, 0);

        lv_obj_add_event_cb(sw, lv_device_switch_event_handler, LV_EVENT_VALUE_CHANGED, &index[i]);

        label = lv_label_create(obj);
        lv_label_set_text(label, swName[i]);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);
        lv_obj_align(label, LV_ALIGN_LEFT_MID, 10, 0);
    }

    const char *labelName[] = {"CHG State", "Battery"};
    for (int i = 0; i < sizeof(labelName) / sizeof(labelName[0]); ++i) {
        lv_obj_t *obj = lv_obj_create(cont);
        lv_obj_set_scroll_dir(obj, LV_DIR_NONE);
        lv_obj_set_size(obj, LV_PCT(100), LV_PCT(15));
        lv_obj_set_style_border_width(obj, 0, 0);
        lv_obj_set_style_bg_opa(obj, LV_OPA_50, LV_PART_MAIN);

        label = lv_label_create(obj);
        lv_label_set_text(label, labelName[i]);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);
        lv_obj_align(label, LV_ALIGN_LEFT_MID, 10, 0);

        label = lv_label_create(obj);
        lv_label_set_text(label, "00");
        lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_18, 0);
        lv_obj_align(label, LV_ALIGN_RIGHT_MID, -30, 0);
        lv_obj_set_user_data(label, &index[i]);

        lv_timer_create([](lv_timer_t *t) {
            lv_obj_t *label = (lv_obj_t *)t->user_data;
            uint8_t *index =  (uint8_t *)lv_obj_get_user_data(label);
            switch (*index) {
            case 0:
                lv_label_set_text_fmt(label, "%s", PMU.getChargeStatusString());
                break;
            case 1:
                lv_label_set_text_fmt(label, "%u mv", PMU.getBattVoltage());
                break;
            default:
                break;
            }
            lv_obj_align(label, LV_ALIGN_RIGHT_MID, -30, 0);
        }, 1000, label);
    }

    return page;
}

static void make_clock_gui(lv_obj_t *parent)
{
    lv_obj_t *main_cout = lv_obj_create(parent);
    lv_obj_set_size(main_cout, LV_PCT(100), LV_PCT(100));
    lv_obj_clear_flag(main_cout, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(main_cout, 0, 0);
    lv_obj_set_style_bg_opa(main_cout, LV_OPA_TRANSP, LV_PART_MAIN);

    lv_obj_t *hour_cout = lv_obj_create(main_cout);
    lv_obj_set_size(hour_cout, LV_PCT(40), LV_PCT(90));
    lv_obj_align(hour_cout, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_set_style_bg_opa(hour_cout, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_opa(hour_cout, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_clear_flag(hour_cout, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *min_cout = lv_obj_create(main_cout);
    lv_obj_set_size(min_cout, LV_PCT(40), LV_PCT(90));
    lv_obj_align(min_cout, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_set_style_bg_opa(min_cout, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_opa(min_cout, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_clear_flag(min_cout, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *seg_text = lv_label_create(main_cout);
    lv_obj_align(seg_text, LV_ALIGN_CENTER, 0, -10);
    lv_obj_set_style_text_font(seg_text, &font_ali, 0);
    lv_label_set_text(seg_text, ":");
    lv_obj_set_style_text_color(seg_text, UI_FONT_COLOR, 0);

    hour_text = lv_label_create(hour_cout);
    lv_obj_center(hour_text);
    lv_obj_set_style_text_font(hour_text, &font_ali, 0);
    lv_label_set_text(hour_text, "12");
    lv_obj_set_style_text_color(hour_text, UI_FONT_COLOR, 0);


    min_text = lv_label_create(min_cout);
    lv_obj_center(min_text);
    lv_obj_set_style_text_font(min_text, &font_ali, 0);
    lv_label_set_text(min_text, "34");
    lv_obj_set_style_text_color(min_text, UI_FONT_COLOR, 0);

    lv_timer_create(updateDateTimeTask, 500, seg_text);
}

static void wifi_event_callback(lv_event_t *e)
{
    lv_obj_t *label = lv_event_get_target(e);
    lv_msg_t *m = lv_event_get_msg(e);
    const char *fmt = (const char *)lv_msg_get_user_data(m);
    const char *payload = (const char *)lv_msg_get_payload(m);
    lv_label_set_text_fmt(label, fmt, payload);
}

static lv_obj_t *make_device_info_gui(lv_obj_t *parent)
{
    lv_obj_t *page;
    lv_obj_t *cont = make_menu_page_cont(parent, &page, "Device Info");

    struct __wifi_if {
        const char *title;
        const char *(*func)();
    } wifi_if[] {
        {"IP:", get_loacl_ip},
        {"MAC:", get_wifi_mac},
        {"Version:", get_wifi_version},
        {"Model:", get_wifi_model},
    };

    uint32_t height = 50;
    height *= sizeof(wifi_if) / sizeof(wifi_if[0]);
    lv_obj_set_height(cont, height);

    const lv_font_t *font = &lv_font_montserrat_20;
    for (int i = 0; i < sizeof(wifi_if) / sizeof(wifi_if[0]); ++i) {

        lv_obj_t *obj = lv_obj_create(cont);
        lv_obj_set_scroll_dir(obj, LV_DIR_NONE);
        lv_obj_set_size(obj, LV_PCT(100), LV_PCT(22));
        lv_obj_set_style_border_width(obj, 0, 0);
        lv_obj_set_style_bg_opa(obj, LV_OPA_50, LV_PART_MAIN);

        lv_obj_t *label = lv_label_create(obj);
        lv_label_set_text(label, wifi_if[i].func());
        lv_obj_set_style_text_font(label, font, 0);
        lv_obj_set_width(label, 200);
        lv_obj_align(label, LV_ALIGN_RIGHT_MID, -60, 0);

        if (i == 0) {
            lv_obj_add_event_cb(label, wifi_event_callback, LV_EVENT_MSG_RECEIVED, NULL);
            lv_msg_subsribe_obj(LV_MSG_WIFI_GET_IP, label, (void *)"IP:%s");
        }

        label = lv_label_create(obj);
        lv_label_set_text(label, wifi_if[i].title);
        lv_obj_set_style_text_font(label, font, 0);
        lv_obj_align(label, LV_ALIGN_LEFT_MID, 10, 0);
    }
    return page;
}

static void make_menu_list_gui(lv_obj_t *parent)
{
    lv_obj_t *cont, *label, *image_btn;
    lv_obj_t *menu = lv_menu_create(parent);
    lv_obj_set_size(menu, lv_disp_get_hor_res(NULL), lv_disp_get_ver_res(NULL));
    lv_obj_set_style_bg_opa(menu, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_center(menu);

    // Set menu header title font size
    lv_menu_t menu8 = *(lv_menu_t *)menu;
    lv_obj_set_style_text_font(menu8.main_header_title, &lv_font_montserrat_24, 0);

    // Set menu header iamge
    lv_obj_t *btn = lv_menu_get_main_header_back_btn(menu);
    lv_obj_t *main_header_back_icon =  lv_obj_get_child(btn, 0);
    lv_img_set_src(main_header_back_icon, &icon_back);
    lv_obj_set_size(btn, icon_back.header.w, icon_back.header.h);
    lv_obj_center(main_header_back_icon);

    /*Create a main page*/
    lv_obj_t *main_page = lv_menu_page_create(menu, NULL);
    lv_obj_set_flex_flow(main_page, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(main_page, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    int w = LV_PCT(34);
    int h = LV_PCT(90);

    // lv_obj_set_scroll_dir(main_page, LV_DIR_BOTTOM);

    lv_obj_t *sub_page1 = make_pmu_info_gui(menu);
    lv_obj_t *sub_page2 = make_device_info_gui(menu);
    lv_obj_t *sub_page3 = make_sensor_info_gui(menu);
    lv_obj_t *sub_page4 = make_wifi_pins_gui(menu);


    struct __menu {
        const lv_img_dsc_t *img;
        const char *name;
        lv_obj_t *sub_page;

    } menuVal[] = {
        {&maintenance_128, "PMU Info", sub_page1},
        {&music_128, "Device Info", sub_page2},
        {&settings_128, "Sensor Info", sub_page3},
        {&compass_128, "WiFi Pins", sub_page4},
        // {&gear_128, "Item 4", NULL},
        // {&fatigue_128, "Item 5", NULL},
        // {&management_128, "Item 6", NULL},
        // {&performance_128, "Item 7", NULL}
    };

    for (int i = 0; i < sizeof(menuVal) / sizeof(menuVal[0]); ++i) {
        cont = lv_menu_cont_create(main_page);
        image_btn = lv_img_create(cont);
        lv_img_set_src(image_btn, menuVal[i].img);
        label = lv_label_create(cont);
        lv_label_set_text(label, menuVal[i].name);
        lv_obj_set_size(cont, w, h);
        lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
        /*Create sub page*/
        if (menuVal[i].sub_page) {
            lv_menu_set_load_page_event(menu, cont, menuVal[i].sub_page);
        }
    }
    lv_menu_set_page(menu, main_page);

}



