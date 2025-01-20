/*
 * 
 * PSP Registry Editor
 * Copyright (c) 2024 ultros (redhate) http://github.com/redhate
 *
 */

#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspdebug.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <pspctrl.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspreg.h>
#include <intraFont.h>
#include <pspthreadman_kernel.h>

#define DELAY 250000

//#include "../common/callbacks.h"

PSP_MODULE_INFO("USERMODE", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER); //0 for kernel
PSP_HEAP_SIZE_MAX();

int real_psp = 0;
int top_ui_selector = 0;
char version_txt_buffer[512];

static int exitRequest = 0;

int running(){
	return !exitRequest;
}

int exitCallback(int arg1, int arg2, void *common){
	exitRequest = 1;
	return 0;
}

int callbackThread(SceSize args, void *argp){
	int cbid;
	cbid = sceKernelCreateCallback("Exit Callback", exitCallback, NULL);
	sceKernelRegisterExitCallback(cbid);
	sceKernelSleepThreadCB();
	return 0;
}

SceUID setupCallbacks(void){
	int thid = 0;
	thid = sceKernelCreateThread("callbackThread", callbackThread, 0x11, 0xFA0, 0, 0);
	if(thid >= 0){
		sceKernelStartThread(thid, 0, 0);
	}
	return thid;
}

// Colors
enum colors {
	/* ARGB */
	RED 		= 0xFF0000FF,
	GREEN 		= 0xFF00FF00,
	BLUE 		= 0xFFFF0000,
	WHITE 		= 0xFFFFFFFF,
	LITEGRAY 	= 0xFFBFBFBF,
	GRAY 		= 0xFF7F7F7F,
	DARKGRAY 	= 0xFF3F3F3F,
	BLACK 		= 0xFF000000,
};

//font globals
intraFont* ltn[16];  //latin fonts (large/small,with/without serif,regular/italic/bold/italic&bold)
intraFont* jpn0;

//yanked directly from the intrafont sample, why reinvent the wheel?
void loadfonts(void) {
 
 	#define FILE_PREFIX "flash0:/font/"
 
	// Init intraFont library
	intraFontInit();

	// Load latin fonts
	char file[40];
	int i;
	for (i = 0; i < 16; i++){
		sprintf(file, FILE_PREFIX "ltn%d.pgf", i);
		ltn[i] = intraFontLoad(file, 0);  //<- this is where the actual loading happens
	}

	jpn0 = intraFontLoad(FILE_PREFIX "jpn0.pgf", INTRAFONT_STRING_SJIS);  //japanese font with SJIS text string encoding
	intraFontSetStyle(jpn0, 0.8f, WHITE, DARKGRAY, 0.0f, 0);              //scale to 80%

	// Make sure the important fonts for this application are loaded
	if (!ltn[0] || !ltn[4] || !ltn[8])
	exit(1);

	// Set alternative fonts that are used in case a certain char does not exist in the main font
	intraFontSetAltFont(ltn[8], jpn0);  //japanese font is used for chars that don't exist in latin

}


//registry cdefs
#define REG_ARRAY_LEN 512 //how perfect right? wonder if that was by design or some stroke of luck

//registry globals
int registry_edit=0;
int loading_registry_screen=0;
int registry_item_counter=0;

//registry type
typedef struct registry_entry{
	
	char *dir;				  //registry paths
	char *name;				  //registry item's name
	unsigned int type;		  //it's type, int, str, bin
	unsigned int size;		  //and its size in bytes.
	unsigned char data[1024]; //this is casted to void*
	
}registry_entry;

//i think this covers everything in the xmb and beyond. we got all the dev environment options, color space and resolution parameters unveiled...
registry_entry registry_array[REG_ARRAY_LEN]={
		
	{"/REGISTRY","category_version",0,0,{}},

	{"/SYSPROFILE","sound_reduction",0,0,{}},
	{"/SYSPROFILE/RESOLUTION","horizontal",0,0,{}},
	{"/SYSPROFILE/RESOLUTION","vertical",0,0,{}},

	{"/CONFIG/ALARM","alarm_0_time",0,0,{}},
	{"/CONFIG/ALARM","alarm_1_time",0,0,{}},
	{"/CONFIG/ALARM","alarm_2_time",0,0,{}},
	{"/CONFIG/ALARM","alarm_3_time",0,0,{}},
	{"/CONFIG/ALARM","alarm_4_time",0,0,{}},
	{"/CONFIG/ALARM","alarm_5_time",0,0,{}},
	{"/CONFIG/ALARM","alarm_6_time",0,0,{}},
	{"/CONFIG/ALARM","alarm_7_time",0,0,{}},
	{"/CONFIG/ALARM","alarm_8_time",0,0,{}},
	{"/CONFIG/ALARM","alarm_9_time",0,0,{}},
	{"/CONFIG/ALARM","alarm_0_property",0,0,{}},
	{"/CONFIG/ALARM","alarm_1_property",0,0,{}},
	{"/CONFIG/ALARM","alarm_2_property",0,0,{}},
	{"/CONFIG/ALARM","alarm_3_property",0,0,{}},
	{"/CONFIG/ALARM","alarm_4_property",0,0,{}},
	{"/CONFIG/ALARM","alarm_5_property",0,0,{}},
	{"/CONFIG/ALARM","alarm_6_property",0,0,{}},
	{"/CONFIG/ALARM","alarm_7_property",0,0,{}},
	{"/CONFIG/ALARM","alarm_8_property",0,0,{}},
	{"/CONFIG/ALARM","alarm_9_property",0,0,{}},

	{"/CONFIG/BROWSER","home_uri",0,0,{}},
	{"/CONFIG/BROWSER","cookie_mode",0,0,{}},
	{"/CONFIG/BROWSER","proxy_mode",0,0,{}},
	{"/CONFIG/BROWSER","proxy_address",0,0,{}},
	{"/CONFIG/BROWSER","proxy_port",0,0,{}},
	{"/CONFIG/BROWSER","picture",0,0,{}},
	{"/CONFIG/BROWSER","animation",0,0,{}},
	{"/CONFIG/BROWSER","javascript",0,0,{}},
	{"/CONFIG/BROWSER","cache_size",0,0,{}},
	{"/CONFIG/BROWSER","char_size",0,0,{}},
	{"/CONFIG/BROWSER","disp_mode",0,0,{}},
	{"/CONFIG/BROWSER","connect_mode",0,0,{}},
	{"/CONFIG/BROWSER","flash_activated",0,0,{}},
	{"/CONFIG/BROWSER","flash_play",0,0,{}},
	{"/CONFIG/BROWSER","proxy_protect",0,0,{}},
	{"/CONFIG/BROWSER","proxy_autoauth",0,0,{}},
	{"/CONFIG/BROWSER","proxy_user",0,0,{}},
	{"/CONFIG/BROWSER","proxy_password",0,0,{}},
	{"/CONFIG/BROWSER","webpage_quality",0,0,{}},

	{"/CONFIG/CAMERA","still_size",0,0,{}},
	{"/CONFIG/CAMERA","movie_size",0,0,{}},
	{"/CONFIG/CAMERA","still_quality",0,0,{}},
	{"/CONFIG/CAMERA","movie_quality",0,0,{}},
	{"/CONFIG/CAMERA","movie_fps",0,0,{}},
	{"/CONFIG/CAMERA","white_balance",0,0,{}},
	{"/CONFIG/CAMERA","exposure_bias",0,0,{}},
	{"/CONFIG/CAMERA","shutter_sound_mode",0,0,{}},
	{"/CONFIG/CAMERA","file_folder",0,0,{}},
	{"/CONFIG/CAMERA","file_number",0,0,{}},
	{"/CONFIG/CAMERA","msid",0,0,{}},
	{"/CONFIG/CAMERA","still_effect",0,0,{}},

	{"/CONFIG/DATE","time_format",0,0,{}},
	{"/CONFIG/DATE","date_format",0,0,{}},
	{"/CONFIG/DATE","summer_time",0,0,{}},
	{"/CONFIG/DATE","time_zone_offset",0,0,{}},
	{"/CONFIG/DATE","time_zone_area",0,0,{}},

	{"/CONFIG/DISPLAY","aspect_ratio",0,0,{}},
	{"/CONFIG/DISPLAY","scan_mode",0,0,{}},
	{"/CONFIG/DISPLAY","screensaver_start_time",0,0,{}},
	{"/CONFIG/DISPLAY","color_space_mode",0,0,{}},
	{"/CONFIG/DISPLAY","pi_blending_mode",0,0,{}},

	{"/CONFIG/LFTV","netav_domain_name",0,0,{}},
	{"/CONFIG/LFTV","netav_ip_address",0,0,{}},
	{"/CONFIG/LFTV","netav_port_no_home",0,0,{}},
	{"/CONFIG/LFTV","netav_port_no_away",0,0,{}},
	{"/CONFIG/LFTV","netav_nonce",0,0,{}},
	{"/CONFIG/LFTV","base_station_version",0,0,{}},
	{"/CONFIG/LFTV","base_station_region",0,0,{}},
	{"/CONFIG/LFTV","tuner_type",0,0,{}},
	{"/CONFIG/LFTV","input_line",0,0,{}},
	{"/CONFIG/LFTV","tv_channel",0,0,{}},
	{"/CONFIG/LFTV","bitrate_home",0,0,{}},
	{"/CONFIG/LFTV","bitrate_away",0,0,{}},
	{"/CONFIG/LFTV","channel_setting_jp",0,0,{}},
	{"/CONFIG/LFTV","channel_setting_us",0,0,{}},
	{"/CONFIG/LFTV","channel_setting_us_catv",0,0,{}},
	{"/CONFIG/LFTV","overwrite_netav_setting",0,0,{}},
	{"/CONFIG/LFTV","screen_mode",0,0,{}},
	{"/CONFIG/LFTV","remocon_setting_region",0,0,{}},
	{"/CONFIG/LFTV","remocon_setting",0,0,{}},
	{"/CONFIG/LFTV","remocon_setting_revision",0,0,{}},
	{"/CONFIG/LFTV","external_tuner_channel",0,0,{}},
	{"/CONFIG/LFTV","ssid",0,0,{}},
	{"/CONFIG/LFTV","audio_gain",0,0,{}},
	{"/CONFIG/LFTV","broadcast_standard_video1",0,0,{}},
	{"/CONFIG/LFTV","broadcast_standard_video2",0,0,{}},
	{"/CONFIG/LFTV","version",0,0,{}},
	{"/CONFIG/LFTV","tv_channel_range",0,0,{}},
	{"/CONFIG/LFTV","tuner_type_no",0,0,{}},
	{"/CONFIG/LFTV","input_line_no",0,0,{}},
	{"/CONFIG/LFTV","audio_channel",0,0,{}},
	{"/CONFIG/LFTV","shared_remocon_setting",0,0,{}},

	{"/CONFIG/MUSIC","wma_play",0,0,{}},
	{"/CONFIG/MUSIC","visualizer_mode",0,0,{}},
	{"/CONFIG/MUSIC","track_info_mode",0,0,{}},

	{"/CONFIG/NETWORK/ADHOC","channel",0,0,{}},
	{"/CONFIG/NETWORK/GO_MESSENGER","auth_name",0,0,{}},
	{"/CONFIG/NETWORK/GO_MESSENGER","auth_key",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE","latest_id",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE","eap_md5",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE","auto_setting",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE","wifisvc_setting",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/0","version",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/0","device",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/0","cnf_name",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/0","ssid",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/0","auth_proto",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/0","wep_key",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/0","ip_address",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/0","netmask",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/0","default_route",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/0","dns_flag",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/0","primary_dns",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/0","secondary_dns",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/0","auth_name",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/0","auth_key",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/0","http_proxy_flag",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/0","http_proxy_server",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/0","http_proxy_port",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/0","auth_8021x_type",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/0","auth_8021x_auth_name",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/0","auth_8021x_auth_key",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/0","wpa_key_type",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/0","wpa_key",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/0","wifisvc_config",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/0/SUB1","wifisvc_auth_name",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/0/SUB1","wifisvc_auth_key",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/0/SUB1","wifisvc_option",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/0/SUB1","last_leased_dhcp_addr",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/1","version",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/1","device",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/1","cnf_name",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/1","ssid",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/1","auth_proto",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/1","wep_key",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/1","ip_address",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/1","netmask",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/1","default_route",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/1","dns_flag",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/1","primary_dns",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/1","secondary_dns",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/1","auth_name",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/1","auth_key",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/1","http_proxy_flag",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/1","http_proxy_server",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/1","http_proxy_port",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/1","auth_8021x_type",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/1","auth_8021x_auth_name",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/1","auth_8021x_auth_key",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/1","wpa_key_type",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/1","wpa_key",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/1","wifisvc_config",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/1/SUB1","wifisvc_auth_name",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/1/SUB1","wifisvc_auth_key",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/1/SUB1","wifisvc_option",0,0,{}},
	{"/CONFIG/NETWORK/INFRASTRUCTURE/1/SUB1","last_leased_dhcp_addr",0,0,{}},

	{"/CONFIG/NP","env",0,0,{}},
	{"/CONFIG/NP","account_id",0,0,{}},
	{"/CONFIG/NP","password",0,0,{}},
	{"/CONFIG/NP","auto_sign_in_enable",0,0,{}},
	{"/CONFIG/NP","nav_only",0,0,{}},
	{"/CONFIG/NP","np_ad_clock_diff",0,0,{}},

	{"/CONFIG/ONESEG","schedule_data_key",0,0,{}},

	{"/CONFIG/PHOTO","slideshow_speed",0,0,{}},
	{"/CONFIG/PREMO","guide_page",0,0,{}},
	{"/CONFIG/PREMO","response",0,0,{}},
	{"/CONFIG/PREMO","ps3_name",0,0,{}},
	{"/CONFIG/PREMO","ps3_mac",0,0,{}},
	{"/CONFIG/PREMO","ps3_keytype",0,0,{}},
	{"/CONFIG/PREMO","ps3_key",0,0,{}},
	{"/CONFIG/PREMO","custom_video_bitrate1",0,0,{}},
	{"/CONFIG/PREMO","custom_video_bitrate2",0,0,{}},
	{"/CONFIG/PREMO","custom_video_buffer1",0,0,{}},
	{"/CONFIG/PREMO","custom_video_buffer2",0,0,{}},
	{"/CONFIG/PREMO","setting_internet",0,0,{}},
	{"/CONFIG/PREMO","button_assign",0,0,{}},
	{"/CONFIG/PREMO","flags",0,0,{}},
	{"/CONFIG/PREMO","account_id",0,0,{}},
	{"/CONFIG/PREMO","login_id",0,0,{}},
	{"/CONFIG/PREMO","password",0,0,{}},

	{"/CONFIG/RSS","download_items",0,0,{}},

	{"/CONFIG/SYSTEM","owner_name",0,0,{}},
	{"/CONFIG/SYSTEM","backlight_brightness",0,0,{}},
	{"/CONFIG/SYSTEM","umd_autoboot",0,0,{}},
	{"/CONFIG/SYSTEM","usb_charge",0,0,{}},
	{"/CONFIG/SYSTEM","umd_cache",0,0,{}},
	{"/CONFIG/SYSTEM","usb_auto_connect",0,0,{}},
	{"/CONFIG/SYSTEM/XMB","theme_type",0,0,{}},
	{"/CONFIG/SYSTEM/XMB","language",0,0,{}},
	{"/CONFIG/SYSTEM/XMB","button_assign",0,0,{}},
	{"/CONFIG/SYSTEM/XMB/THEME","color_mode",0,0,{}},
	{"/CONFIG/SYSTEM/XMB/THEME","wallpaper_mode",0,0,{}},
	{"/CONFIG/SYSTEM/XMB/THEME","system_color",0,0,{}},
	{"/CONFIG/SYSTEM/XMB/THEME","custom_theme_mode",0,0,{}},
	{"/CONFIG/SYSTEM/SOUND","main_volume",0,0,{}},
	{"/CONFIG/SYSTEM/SOUND","mute",0,0,{}},
	{"/CONFIG/SYSTEM/SOUND","avls",0,0,{}},
	{"/CONFIG/SYSTEM/SOUND","equalizer_mode",0,0,{}},
	{"/CONFIG/SYSTEM/SOUND","operation_sound_mode",0,0,{}},
	{"/CONFIG/SYSTEM/SOUND","dynamic_normalizer",0,0,{}},
	{"/CONFIG/SYSTEM/POWER_SAVING","suspend_interval",0,0,{}},
	{"/CONFIG/SYSTEM/POWER_SAVING","backlight_off_interval",0,0,{}},
	{"/CONFIG/SYSTEM/POWER_SAVING","wlan_mode",0,0,{}},
	{"/CONFIG/SYSTEM/POWER_SAVING","active_backlight_mode",0,0,{}},
	{"/CONFIG/SYSTEM/LOCK","password",0,0,{}},
	{"/CONFIG/SYSTEM/LOCK","parental_level",0,0,{}},
	{"/CONFIG/SYSTEM/LOCK","browser_start",0,0,{}},
	{"/CONFIG/SYSTEM/CHARACTER_SET","oem",0,0,{}},
	{"/CONFIG/SYSTEM/CHARACTER_SET","ansi",0,0,{}},

	{"/CONFIG/VIDEO","menu_language",0,0,{}},
	{"/CONFIG/VIDEO","sound_language",0,0,{}},
	{"/CONFIG/VIDEO","subtitle_language",0,0,{}},
	{"/CONFIG/VIDEO","appended_volume",0,0,{}},
	{"/CONFIG/VIDEO","lr_button_enable",0,0,{}},
	{"/CONFIG/VIDEO","list_play_mode",0,0,{}},
	{"/CONFIG/VIDEO","title_display_mode",0,0,{}},
	{"/CONFIG/VIDEO","output_ext_menu",0,0,{}},
	{"/CONFIG/VIDEO","output_ext_func",0,0,{}},

	{"/DATA/FONT","path_name",0,0,{}},
	{"/DATA/FONT","num_fonts",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO0","h_size",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO0","v_size",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO0","h_resolution",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO0","v_resolution",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO0","extra_attributes",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO0","weight",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO0","family_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO0","style",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO0","sub_style",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO0","language_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO0","region_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO0","country_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO0","font_name",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO0","file_name",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO0","expire_date",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO0","shadow_option",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO1","h_size",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO1","v_size",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO1","h_resolution",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO1","v_resolution",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO1","extra_attributes",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO1","weight",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO1","family_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO1","style",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO1","sub_style",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO1","language_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO1","region_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO1","country_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO1","font_name",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO1","file_name",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO1","expire_date",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO1","shadow_option",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO2","h_size",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO2","v_size",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO2","h_resolution",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO2","v_resolution",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO2","extra_attributes",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO2","weight",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO2","family_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO2","style",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO2","sub_style",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO2","language_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO2","region_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO2","country_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO2","font_name",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO2","file_name",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO2","expire_date",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO2","shadow_option",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO3","h_size",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO3","v_size",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO3","h_resolution",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO3","v_resolution",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO3","extra_attributes",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO3","weight",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO3","family_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO3","style",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO3","sub_style",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO3","language_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO3","region_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO3","country_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO3","font_name",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO3","file_name",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO3","expire_date",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO3","shadow_option",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO4","h_size",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO4","v_size",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO4","h_resolution",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO4","v_resolution",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO4","extra_attributes",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO4","weight",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO4","family_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO4","style",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO4","sub_style",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO4","language_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO4","region_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO4","country_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO4","font_name",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO4","file_name",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO4","expire_date",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO4","shadow_option",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO5","h_size",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO5","v_size",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO5","h_resolution",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO5","v_resolution",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO5","extra_attributes",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO5","weight",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO5","family_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO5","style",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO5","sub_style",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO5","language_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO5","region_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO5","country_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO5","font_name",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO5","file_name",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO5","expire_date",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO5","shadow_option",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO6","h_size",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO6","v_size",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO6","h_resolution",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO6","v_resolution",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO6","extra_attributes",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO6","weight",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO6","family_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO6","style",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO6","sub_style",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO6","language_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO6","region_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO6","country_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO6","font_name",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO6","file_name",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO6","expire_date",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO6","shadow_option",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO7","h_size",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO7","v_size",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO7","h_resolution",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO7","v_resolution",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO7","extra_attributes",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO7","weight",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO7","family_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO7","style",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO7","sub_style",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO7","language_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO7","region_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO7","country_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO7","font_name",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO7","file_name",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO7","expire_date",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO7","shadow_option",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO8","h_size",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO8","v_size",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO8","h_resolution",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO8","v_resolution",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO8","extra_attributes",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO8","weight",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO8","family_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO8","style",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO8","sub_style",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO8","language_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO8","region_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO8","country_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO8","font_name",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO8","file_name",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO8","expire_date",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO8","shadow_option",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO9","h_size",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO9","v_size",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO9","h_resolution",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO9","v_resolution",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO9","extra_attributes",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO9","weight",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO9","family_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO9","style",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO9","sub_style",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO9","language_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO9","region_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO9","country_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO9","font_name",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO9","file_name",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO9","expire_date",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO9","shadow_option",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO10","h_size",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO10","v_size",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO10","h_resolution",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO10","v_resolution",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO10","extra_attributes",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO10","weight",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO10","family_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO10","style",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO10","sub_style",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO10","language_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO10","region_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO10","country_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO10","font_name",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO10","file_name",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO10","expire_date",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO10","shadow_option",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO11","h_size",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO11","v_size",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO11","h_resolution",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO11","v_resolution",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO11","extra_attributes",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO11","weight",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO11","family_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO11","style",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO11","sub_style",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO11","language_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO11","region_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO11","country_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO11","font_name",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO11","file_name",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO11","expire_date",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO11","shadow_option",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO12","h_size",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO12","v_size",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO12","h_resolution",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO12","v_resolution",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO12","extra_attributes",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO12","weight",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO12","family_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO12","style",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO12","sub_style",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO12","language_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO12","region_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO12","country_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO12","font_name",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO12","file_name",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO12","expire_date",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO12","shadow_option",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO13","h_size",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO13","v_size",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO13","h_resolution",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO13","v_resolution",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO13","extra_attributes",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO13","weight",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO13","family_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO13","style",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO13","sub_style",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO13","language_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO13","region_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO13","country_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO13","font_name",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO13","file_name",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO13","expire_date",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO13","shadow_option",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO14","h_size",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO14","v_size",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO14","h_resolution",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO14","v_resolution",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO14","extra_attributes",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO14","weight",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO14","family_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO14","style",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO14","sub_style",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO14","language_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO14","region_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO14","country_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO14","font_name",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO14","file_name",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO14","expire_date",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO14","shadow_option",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO15","h_size",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO15","v_size",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO15","h_resolution",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO15","v_resolution",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO15","extra_attributes",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO15","weight",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO15","family_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO15","style",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO15","sub_style",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO15","language_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO15","region_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO15","country_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO15","font_name",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO15","file_name",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO15","expire_date",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO15","shadow_option",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO16","h_size",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO16","v_size",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO16","h_resolution",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO16","v_resolution",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO16","extra_attributes",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO16","weight",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO16","family_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO16","style",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO16","sub_style",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO16","language_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO16","region_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO16","country_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO16","font_name",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO16","file_name",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO16","expire_date",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO16","shadow_option",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO17","h_size",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO17","v_size",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO17","h_resolution",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO17","v_resolution",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO17","extra_attributes",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO17","weight",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO17","family_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO17","style",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO17","sub_style",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO17","language_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO17","region_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO17","country_code",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO17","font_name",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO17","file_name",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO17","expire_date",0,0,{}},
	{"/DATA/FONT/PROPERTY/INFO17","shadow_option",0,0,{}},

};

int set_registry_value(registry_entry *reg_item){
	
	
	int ret = 0;
	struct RegParam reg;
	REGHANDLE h;

	memset(&reg, 0, sizeof(reg));
	reg.regtype = 1;
	reg.namelen = strlen("/system");
	reg.unk2 = 1;
	reg.unk3 = 1;
	strcpy(reg.name, "/system");
	
	if(sceRegOpenRegistry(&reg, 2, &h) == 0){
		REGHANDLE hd;
		if(!sceRegOpenCategory(h, reg_item->dir, 2, &hd)){
			
			switch(reg_item->type){
				case REG_TYPE_INT:{
					if(!sceRegSetKeyValue(hd, reg_item->name, &reg_item->data, reg_item->size)){
						printf("Set registry value\n");
						ret = 1;
						sceRegFlushCategory(hd);
					}
				}break;
				case REG_TYPE_STR:{
					if(!sceRegSetKeyValue(hd, reg_item->name, (char*)reg_item->data, reg_item->size)){
						printf("Set registry value\n");
						ret = 1;
						sceRegFlushCategory(hd);
					}
				}
				case REG_TYPE_BIN:{
					if(!sceRegSetKeyValue(hd, reg_item->name, (unsigned char*)reg_item->data, reg_item->size)){
						printf("Set registry value\n");
						ret = 1;
						sceRegFlushCategory(hd);
					}
				}
			}
			sceRegCloseCategory(hd);
		}
		sceRegFlushRegistry(h);
		sceRegCloseRegistry(h);
	}

	return ret;
	
}

int get_registry_value(registry_entry *reg_item){
	
	int ret = 0;
	
	struct RegParam reg;
	memset(&reg, 0, sizeof(reg));
	reg.regtype = 1;
	reg.namelen = strlen("/system");
	reg.unk2 = 1;
	reg.unk3 = 1;
	strcpy(reg.name, "/system");
	
	REGHANDLE h;
	if(sceRegOpenRegistry(&reg, 2, &h) == 0){
		REGHANDLE hd;
		if(!sceRegOpenCategory(h, reg_item->dir, 2, &hd)){
			REGHANDLE hk;
			if(!sceRegGetKeyInfo(hd, reg_item->name, &hk, &reg_item->type, &reg_item->size)){

			 // if(!sceRegGetKeyValue(hd, hk, 0, a_size)){
			 // 	val=malloc(sizeof(char)*a_size);
			 // }
				if(!sceRegGetKeyValue(hd, hk, reg_item->data, reg_item->size)){
					ret = 1;
					sceRegFlushCategory(hd);
				}

			}
			sceRegCloseCategory(hd);
		}
		sceRegFlushRegistry(h);
		sceRegCloseRegistry(h);
	}

	return ret;
	
}

void update_registry_array(){
	registry_item_counter=0;
	int x;
	for(x=0;x<REG_ARRAY_LEN;x++){
		get_registry_value(&registry_array[x]);
		registry_item_counter++;
	}
}

unsigned int load_registry_bin_from_ms(registry_entry array[]){
	
	signed int fd = sceIoOpen("ms0:/reg.bin", PSP_O_RDONLY, 0777);
	if(fd > 0){
		sceIoRead(fd, array, sizeof(registry_entry)*REG_ARRAY_LEN);
		sceIoClose(fd);
		return 1;
	}
	else{
		//Return error
		return 0;
	}

}

unsigned int save_registry_bin_to_ms(registry_entry array[]){
	
	signed int fd = sceIoOpen("ms0:/reg.bin", PSP_O_CREAT | PSP_O_WRONLY, 0777);
	if(fd > 0){
		sceIoWrite(fd, array, sizeof(registry_entry)*REG_ARRAY_LEN);
		sceIoClose(fd);
		return 1;
	}
	else{
		//Return error
		return 0;
	}

}

//thread cdefs
#define MAX_THREAD 64

//thread globals
static SceUID thread_buf_now[MAX_THREAD];
static SceUID selected_thid;
static int thread_count_now;
int thread_edit=0;
int threadSelected=0;
int threadinfo_status=0;

SceKernelThreadInfo* fetchThreadInfo(){
	
	sceKernelGetThreadmanIdList(SCE_KERNEL_TMID_Thread, thread_buf_now, MAX_THREAD, &thread_count_now);
	SceKernelThreadInfo *status=malloc(sizeof(SceKernelThreadInfo)*MAX_THREAD);
	
	int x;
	for(x = 0; x < thread_count_now; x++){
		SceUID tmp_thid = thread_buf_now[x];
		//int thid;
		int ret;
		memset(&status[x], 0, sizeof(SceKernelThreadInfo));
		status[x].size = sizeof(SceKernelThreadInfo);
		ret = sceKernelReferThreadStatus(tmp_thid, &status[x]); 
	}
	
	//we want this to return the address of the local variable for fuck sake 
	//unless it gets cleared which it does not. bitch bitch bitch from gcc
	return status;
}

unsigned int getVersionText(){
	
	signed int fd = sceIoOpen("flash0:/vsh/etc/version.txt", PSP_O_RDONLY, 0777);
	if(fd > 0){
		unsigned int fileSize = sceIoLseek(fd, 0, SEEK_END); sceIoLseek(fd, 0, SEEK_SET);
		sceIoRead(fd, version_txt_buffer, fileSize);
		sceIoClose(fd);
		return 1;
	}
	else{
		//Return error
		return 0;
	}

}

//keyboard cus i'm a hacker and the osk from the samples is dated and is not qwerty
static char CharTable[2][4][13] = {
	{
	  { '`', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=' },
	  { 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\\' },
	  { 0x20, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', 0x20 },
	  { 0x20, 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0x20, 0x20 },
	},
	{
	  { '~', '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+' },
	  { 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', '|' }, // { }
	  { 0x20, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', 0x20 },
	  { 0x20, 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0x20, 0x20 },
	}
};

#define KEYBOARD_BUFFER_SIZE 1024
char keyboard_buffer[KEYBOARD_BUFFER_SIZE];
int  keyboard_x_select=2,
     keyboard_y_select=5,
     keyboard_shift=0,
     keyboard_enabled=0,
     keyboard_input_strokes=0; //we strokin' (STROKIN')

void keyboard_ui(){
	
	int screen_offset_x=100, 
		screen_offset_y=25;
	
	intraFontSetStyle(ltn[8], 1.5f, WHITE, 0, 0.0f, 0);
	
	intraFontPrint(ltn[8], screen_offset_y, screen_offset_x-35, keyboard_buffer);
	
	int x, y;
	for(x=0;x<4;x++){ //row
		for(y=0;y<13;y++){ //column
			char buffer[1];
			sprintf(buffer, "%c",  *((char*)&CharTable[keyboard_shift][x][y]));
			
			//if selected highlight the selection in red.
			if((keyboard_x_select == x) && (keyboard_y_select == y))
				intraFontSetStyle(ltn[8], 1.5f, RED, 0, 0.0f, 0);
			else 
				intraFontSetStyle(ltn[8], 1.5f, WHITE, 0, 0.0f, 0);
				
			intraFontPrint(ltn[8], screen_offset_y+y*35, screen_offset_x+x*35, buffer); //yes intra font is that fucking stupid to go y before x. assholes... may have to remedy that in the source.
		}
	}
	
}

int keyboard_controls(char *buffer){

	sceKernelDelayThread(DELAY);

	SceCtrlData input;
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
    
    //pad.Buttons = 0;
    
	while(1){
	
		sceCtrlReadBufferPositive(&input, 1);

		/*
		#define DEAD_SPACE 	10
		#define CENTER 		128
		
		//analog directional control over the keyboard with a little dead space
		if(input.Ly > CENTER+DEAD_SPACE){
			if(keyboard_x_select < 3) keyboard_x_select++;
			sceKernelDelayThread(DELAY);
		}
		if(input.Ly < CENTER-DEAD_SPACE){
			if(keyboard_x_select > 0) keyboard_x_select--;
			sceKernelDelayThread(DELAY);
		}
		if(input.Lx > CENTER+DEAD_SPACE){
			if(keyboard_y_select < 12) keyboard_y_select++;
			sceKernelDelayThread(DELAY);
		}
		if(input.Lx < CENTER-DEAD_SPACE){
			if(keyboard_y_select > 0) keyboard_y_select--;
			sceKernelDelayThread(DELAY);
		}
		*/
		
		//directional control over the keyboard
		if(input.Buttons & PSP_CTRL_UP){
			if(keyboard_x_select > 0) keyboard_x_select--;
			sceKernelDelayThread(DELAY);
		}
		if(input.Buttons & PSP_CTRL_DOWN){
			if(keyboard_x_select < 3) keyboard_x_select++;
			sceKernelDelayThread(DELAY);
		}
		if(input.Buttons & PSP_CTRL_LEFT){
			if(keyboard_y_select > 0) keyboard_y_select--;
			sceKernelDelayThread(DELAY);
		}
		if(input.Buttons & PSP_CTRL_RIGHT){
			if(keyboard_y_select < 12) keyboard_y_select++;
			sceKernelDelayThread(DELAY);
		}
		
		//reserved for close keyboard
		if(input.Buttons & PSP_CTRL_CIRCLE){
			keyboard_x_select=2,
			keyboard_y_select=5,
			keyboard_shift=0,
			keyboard_enabled=0,
			keyboard_input_strokes=0;
			sceKernelDelayThread(DELAY);
			break;
		}
		
		//backspace
		if(input.Buttons & PSP_CTRL_SQUARE){
			if(keyboard_input_strokes > 0) keyboard_input_strokes--;
			keyboard_buffer[keyboard_input_strokes] = 0x00;
			sceKernelDelayThread(DELAY);
		}
		
		//input the key into buffer
		if(input.Buttons & PSP_CTRL_CROSS){
			keyboard_buffer[keyboard_input_strokes] = CharTable[keyboard_shift][keyboard_x_select][keyboard_y_select];
			keyboard_buffer[keyboard_input_strokes+1] = 0x00;
			keyboard_input_strokes++;
			sceKernelDelayThread(DELAY);
		}
		
		//shift the keyboard
		if(input.Buttons & PSP_CTRL_SELECT){
			if(!keyboard_shift) keyboard_shift = 1;
			else keyboard_shift = 0;
			sceKernelDelayThread(DELAY);
		}
		
		//insert the keyboard buffer into the destination buffer
		if(input.Buttons & PSP_CTRL_START){
			strcpy(buffer, keyboard_buffer);
			//clear the buffer and close keyboard
			keyboard_x_select=2,
			keyboard_y_select=5,
			keyboard_shift=0,
			keyboard_enabled=0,
			keyboard_input_strokes=0;
			sceKernelDelayThread(DELAY);
			return 1;
		}
	}
	
	return 0;
	
}

#define DISPLAY_LINES 15
#define BROWSE_PITCH 16

int reg_selector = 0, 
	reg_scroll = 0;
	
int thread_scroll = 0;

int bin_x_select=0, 
	bin_y_select=0;

unsigned int hex_to_u32(const char* hex_str){
	
	unsigned int length = (char) strlen(hex_str);
	unsigned int tmp = 0;
	unsigned int result = 0;
	char c;

	while (length--){
	
		c = *hex_str++;
		
		     if((c >= '0') && (c <= '9')) tmp = c - '0';
		else if((c >= 'a') && (c <= 'f')) tmp = c - 'a' + 10;
		else if((c >= 'A') && (c <= 'F')) tmp = c - 'A' + 10;
		else tmp = 0;
		
		result |= (tmp << (length * 4));
	
	}

	return result;
	
}

void draw_ui(){

	if(!thread_edit && !registry_edit){
		intraFontSetStyle(ltn[0], 0.50f, (top_ui_selector == 0? WHITE: GRAY), 0, 0.0f, 0);			   	intraFontPrint(ltn[0], 60,  260, " [THREADS]");
		intraFontSetStyle(ltn[0], 0.50f, (top_ui_selector == 1? WHITE: GRAY), 0, 0.0f, 0);			   	intraFontPrint(ltn[0], 160, 260, " [REGISTRY]");
		intraFontSetStyle(ltn[0], 0.50f, (top_ui_selector == 2? WHITE: GRAY), 0, 0.0f, 0);			   	intraFontPrint(ltn[0], 260, 260, " [KERNEL]");
		intraFontSetStyle(ltn[0], 0.50f, (top_ui_selector == 3? WHITE: GRAY), 0, 0.0f, 0);			   	intraFontPrint(ltn[0], 360, 260, " [BUILD]");
	}
	
	if(keyboard_enabled){
		keyboard_ui();
	}
	else{
		//threads
		if(top_ui_selector == 0){
			if(thread_edit){
				
				intraFontSetStyle(ltn[0], 0.50f, WHITE, 0, 0.0f, 0);	
				
				//int thid;
				SceKernelThreadInfo status;
				int ret;

				char buffer[128];
				
				//thid = sceKernelGetThreadId();
				memset(&status, 0, sizeof(SceKernelThreadInfo));
				sprintf(buffer, "Thread ID: %08X\n", selected_thid); pspDebugScreenPuts(buffer);
				status.size = sizeof(SceKernelThreadInfo);
				ret = sceKernelReferThreadStatus(selected_thid, &status);
				sprintf(buffer, "Get Thread Status: %08X\n", ret); pspDebugScreenPuts(buffer);
				if(ret == 0){
				
					char buffer[128];
					
					intraFontPrint(ltn[0], 10, 10, "size");
					sprintf(buffer, "%d",   status.size);			   	
					intraFontPrint(ltn[0], 400, 10, buffer);
					
					intraFontPrint(ltn[0], 10, 25, "name");
					sprintf(buffer, "%s",   status.name);			   	
					intraFontPrint(ltn[0], 400, 25, buffer);
					
					intraFontPrint(ltn[0], 10, 40, "attr");
					sprintf(buffer, "%x",   status.attr);			   	
					intraFontPrint(ltn[0], 400, 40, buffer);
					
					intraFontPrint(ltn[0], 10, 55, "status");
					sprintf(buffer, "%d",   status.status);			   	
					intraFontPrint(ltn[0], 400, 55, buffer);
					
					intraFontPrint(ltn[0], 10, 70, "entry");
					sprintf(buffer, "%X",   (unsigned int)status.entry);			   	
					intraFontPrint(ltn[0], 400, 70, buffer);
					
					intraFontPrint(ltn[0], 10, 85, "stack");
					sprintf(buffer, "%X",   (unsigned int)status.stack);			   	
					intraFontPrint(ltn[0], 400, 85, buffer);
					
					intraFontPrint(ltn[0], 10, 100, "stackSize");
					sprintf(buffer, "%d",   status.stackSize);			
					intraFontPrint(ltn[0], 400, 100, buffer);
					
					intraFontPrint(ltn[0], 10, 115, "gpReg");
					sprintf(buffer, "%X",   (unsigned int)status.gpReg);			   	
					intraFontPrint(ltn[0], 400, 115, buffer);
					
					intraFontPrint(ltn[0], 10, 130, "initPriority");
					sprintf(buffer, "%d",   status.initPriority);	    
					intraFontPrint(ltn[0], 400, 130, buffer);
					
					intraFontPrint(ltn[0], 10, 145, "currentPriority");
					sprintf(buffer, "%d",   status.currentPriority);	
					intraFontPrint(ltn[0], 400, 145, buffer);
					
					intraFontPrint(ltn[0], 10, 160, "waitType");
					sprintf(buffer, "%d",   status.waitType);			
					intraFontPrint(ltn[0], 400, 160, buffer);
					
					intraFontPrint(ltn[0], 10, 175, "waitId");
					sprintf(buffer, "%X",   (unsigned int)status.waitId);			   	
					intraFontPrint(ltn[0], 400, 175, buffer);
					
					intraFontPrint(ltn[0], 10, 190, "wakeupCount");
					sprintf(buffer, "%d",   status.wakeupCount);		
					intraFontPrint(ltn[0], 400, 190, buffer);
					
					intraFontPrint(ltn[0], 10, 205, "exitStatus");
					sprintf(buffer, "%d",   status.exitStatus);			
					intraFontPrint(ltn[0], 400, 205, buffer);
					
					intraFontPrint(ltn[0], 10, 220, "runClocks.low");
					sprintf(buffer, "%d",   status.runClocks.low);			
					intraFontPrint(ltn[0], 400, 220, buffer);
					
					intraFontPrint(ltn[0], 10, 235, "runClocks.hi");
					sprintf(buffer, "%d",   status.runClocks.hi);			
					intraFontPrint(ltn[0], 400, 235, buffer);
					
					intraFontPrint(ltn[0], 10, 250, "intrPreemptCount");
					sprintf(buffer, "%d",   status.intrPreemptCount);	
					intraFontPrint(ltn[0], 400, 250, buffer);
					
					intraFontPrint(ltn[0], 10, 265, "threadPreemptCount");
					sprintf(buffer, "%d",   status.threadPreemptCount);	
					intraFontPrint(ltn[0], 400, 265, buffer);
					
					intraFontPrint(ltn[0], 10, 280, "releaseCount");
					sprintf(buffer, "%d",   status.releaseCount);		
					intraFontPrint(ltn[0], 400, 280, buffer);


					/* Size of the structure 
						SceSize     size;     
						char    	name[32];
						SceUInt     attr;
						int     	status;
						SceKernelThreadEntry    entry;
						void *  	stack;
						int     	stackSize;
						void *  	gpReg;
						int     	initPriority;
						int     	currentPriority;
						int     	waitType;
						SceUID  	waitId;
						int     	wakeupCount;
						int     	exitStatus;
						SceKernelSysClock   runClocks;
						SceUInt     intrPreemptCount;
						SceUInt     threadPreemptCount;
						SceUInt     releaseCount;
					*/

				}

			}
			else{
				
				intraFontSetStyle(ltn[0], 0.50f, RED, 0, 0.0f, 0);
				intraFontPrint(ltn[0], 10, 15, " UID                NAME                                       ENTRY            STACK             GPREG             PRI\n");
				intraFontSetStyle(ltn[0], 0.50f, WHITE, 0, 0.0f, 0);			
				
				SceKernelThreadInfo *threadInfo = fetchThreadInfo();
				if(threadInfo != NULL){
					sceKernelGetThreadmanIdList(SCE_KERNEL_TMID_Thread, thread_buf_now, MAX_THREAD, &thread_count_now);
					char buffer[128];
					int x;
					for(x = 0; x < thread_count_now; x++){
						
						if(threadSelected == x){ 
							intraFontSetStyle(ltn[0], 0.50f, GRAY, BLACK, 0.0f, 0); 
						}
						else{ 
							intraFontSetStyle(ltn[0], 0.50f, WHITE, BLACK, 0.0f, 0);
						}
						
						if(threadSelected == x){  }
						else{ }
						sprintf(buffer, " %d",   thread_buf_now[x+thread_scroll]);		       	intraFontPrint(ltn[0], 10,  30+(x*15), buffer);
						sprintf(buffer, " %s",   threadInfo[x+thread_scroll].name);			   	intraFontPrint(ltn[0], 75,  30+(x*15), buffer);
						sprintf(buffer, "0x%08X", (unsigned int)threadInfo[x+thread_scroll].entry); 		   		intraFontPrint(ltn[0], 225, 30+(x*15), buffer);
						sprintf(buffer, "0x%08X", (unsigned int)threadInfo[x+thread_scroll].stack);	   			intraFontPrint(ltn[0], 300, 30+(x*15), buffer);
						sprintf(buffer, "0x%08X", (unsigned int)threadInfo[x+thread_scroll].gpReg);	   			intraFontPrint(ltn[0], 375, 30+(x*15), buffer);
						sprintf(buffer, " %d\n", threadInfo[x+thread_scroll].currentPriority); 	intraFontPrint(ltn[0], 450, 30+(x*15), buffer);
					}
				}
			}
		}
		//registry
		//really lags the shit out of the gui
		else if(top_ui_selector == 1){
			if(loading_registry_screen){
				char buffer[128];
				intraFontSetStyle(ltn[0], 0.75f, WHITE, 0, 0.0f, 0);
				sprintf(buffer, "Please wait loading entry %d of 512", registry_item_counter);
				intraFontPrint(ltn[0], 100, 135, buffer);
			}
			else if(registry_edit){
					
					intraFontSetStyle(ltn[0], 0.75f, WHITE, 0, 0.0f, 0);	
					
					switch(registry_array[reg_selector+reg_scroll].type){
						
						case REG_TYPE_INT:{
							
							char buffer[128];
							
							//sprintf(buffer, "%s/%s", registry_array[reg_selector+reg_scroll].dir, registry_array[reg_selector+reg_scroll].name);
							//intraFontPrint(ltn[0], 20,  20, buffer);
							
							intraFontPrint(ltn[0], 10,  20, "Int: ");
							intraFontSetStyle(ltn[0], 0.75f, RED, 0, 0.0f, 0);
							sprintf(buffer, "%d", *((int*)&registry_array[reg_selector+reg_scroll].data));
							intraFontPrint(ltn[0], 10,  40, buffer);
							
						}
						break;

						case REG_TYPE_STR:{
							
							//we dont need to print anything in this case as the screen is taken over by the keyboard.
							
							/*
							char buffer[128];
							sprintf(buffer, "%s/%s", registry_array[reg_selector+reg_scroll].dir, registry_array[reg_selector+reg_scroll].name);
							intraFontPrint(ltn[0], 20,  20, buffer);
							
							sprintf(buffer, "Size: %d", registry_array[reg_selector+reg_scroll].size);
							intraFontPrint(ltn[0], 20,  35, buffer);
							
							sprintf(buffer, "String: %s", ((char*)&registry_array[reg_selector+reg_scroll].data));
							intraFontPrint(ltn[0], 20,  50, buffer);
							*/
							
						}
						break;

						case REG_TYPE_BIN:{
							
							char buffer[128];
							intraFontPrint(ltn[0], 10,  20, "Binary:");
							//first pass in hex
							int i, x=0, y=0;
							for(i=0; i<registry_array[reg_selector+reg_scroll].size; i++){
								if(i == y*BROWSE_PITCH){ y++; x=0;}
								else{ x++; }
								
								//if selected highlight the selection in red.
								if((bin_x_select == x) && (bin_y_select == y-1))
									intraFontSetStyle(ltn[0], 0.50f, RED, 0, 0.0f, 0);
								else 
									intraFontSetStyle(ltn[0], 0.50f, WHITE, 0, 0.0f, 0);
								
								sprintf(buffer, "%02hX", *((unsigned char*)&registry_array[reg_selector+reg_scroll].data[i]));
								intraFontPrint(ltn[0], 10+(x*20), 20+(y*20), buffer);
							}
							//second pass print the character
							x=0, y=0;
							for(i=0; i<registry_array[reg_selector+reg_scroll].size; i++){
								if(i == y*BROWSE_PITCH){ y++; x=0;}
								else{ x++; }
								
								//if selected highlight the selection in red.
								if((bin_x_select == x) && (bin_y_select == y-1))
									intraFontSetStyle(ltn[0], 0.50f, RED, 0, 0.0f, 0);
								else 
									intraFontSetStyle(ltn[0], 0.50f, WHITE, 0, 0.0f, 0);
								
								sprintf(buffer, "%c", *((unsigned char*)&registry_array[reg_selector+reg_scroll].data[i]));
								intraFontPrint(ltn[0], 330+(x*9), 20+(y*20), buffer);
							}
							
						}
						break;
						
					};
				
				}
			else{

				intraFontSetStyle(ltn[0], 0.50f, RED, 0, 0.0f, 0);
				intraFontPrint(ltn[0], 10, 15, " SYSTEM REGISTRY\n");
				intraFontSetStyle(ltn[0], 0.50f, WHITE, 0, 0.0f, 0);			
				
				int x;
				for(x=0;x<DISPLAY_LINES;x++){
					
					if(reg_selector == x){ 
						intraFontSetStyle(ltn[0], 0.50f, GRAY, BLACK, 0.0f, 0); 
					}
					else{ 
						intraFontSetStyle(ltn[0], 0.50f, WHITE, BLACK, 0.0f, 0);
					}

					char buffer[128];
					sprintf(buffer, "%s/%s", registry_array[x+reg_scroll].dir, registry_array[x+reg_scroll].name);
					intraFontPrint(ltn[0], 10,  30+(x*15), buffer);
					
					//yes we do need this
					
					switch(registry_array[x+reg_scroll].type){
						
						case REG_TYPE_INT:{
							sprintf(buffer, "%d", *((int*)&registry_array[x+reg_scroll].data));
							intraFontPrint(ltn[0], 400,  30+(x*15), buffer);
						}
						break;

						case REG_TYPE_STR:{
							sprintf(buffer, "%s", ((char*)&registry_array[x+reg_scroll].data));
							intraFontPrint(ltn[0], 400,  30+(x*15), buffer);
						}
						break;

						case REG_TYPE_BIN:{
							sprintf(buffer, "%s", ((char*)&registry_array[x+reg_scroll].data));
							intraFontPrint(ltn[0], 400,  30+(x*15), buffer);
						}
						break;
						
					};

				}
			
			}
		}
		//kernel
		else if(top_ui_selector == 2){
			char buffer[128];
			intraFontSetStyle(ltn[0], 0.75f, WHITE, 0, 0.0f, 0);
			sprintf(buffer, "0x%08X", hex_to_u32("0xDEADBEEF"));
			intraFontPrint(ltn[0], 100, 135, buffer);
		}
		//version page
		else if(top_ui_selector == 3){
			intraFontSetStyle(ltn[0], 0.65f, WHITE, 0, 0.0f, 0);
			if(real_psp){
				if(version_txt_buffer[0] != 0){		   	
					intraFontPrint(ltn[0], 35, 100, version_txt_buffer);
				}
			}
			else{
				intraFontPrint(ltn[0], 180, 130, "      PPSSPP\nNot Supported");
			}
			intraFontSetStyle(ltn[0], 0.5f, WHITE, 0, 0.0f, 0);
			intraFontPrint(ltn[0], 250, 200, "Version: Beta 2");
			intraFontPrint(ltn[0], 250, 215, "2024 - RedHate - github.com/redhate");
		}
	}
	
}

int controlThread(){
	

	/*
		//set_registry_value("/CONFIG/NP", "env", "C1-NP");
		//set_registry_value("/CONFIG/NP", "env", "D1-NP"); //developer
		//set_registry_value("/CONFIG/NP", "env", "D2-NP"); //developer
		//set_registry_value("/CONFIG/NP", "env", "D3-NP"); //developer
		//set_registry_value("/CONFIG/NP", "env", "D1-PMGT"); //developer
		//set_registry_value("/CONFIG/NP", "env", "D2-PMGT"); //developer
		//set_registry_value("/CONFIG/NP", "env", "D3-PMGT"); //developer
		//set_registry_value("/CONFIG/NP", "env", "D1-PQA"); //developer
		//set_registry_value("/CONFIG/NP", "env", "D2-PQA"); //developer
		//set_registry_value("/CONFIG/NP", "env", "D3-PQA"); //developer
		//set_registry_value("/CONFIG/NP", "env", "D1-SPINT"); //developer
		//set_registry_value("/CONFIG/NP", "env", "D2-SPINT"); //developer
		//set_registry_value("/CONFIG/NP", "env", "D3-SPINT"); //developer
		//set_registry_value("/CONFIG/NP", "env", "D-NP");  //developer
		//set_registry_value("/CONFIG/NP", "env", "D-PMGT");  //developer
		//set_registry_value("/CONFIG/NP", "env", "D-DPQA");  //developer
		//set_registry_value("/CONFIG/NP", "env", "D-SPINT");  //developer
		//set_registry_value("/CONFIG/NP", "env", "E1-NP");
		//set_registry_value("/CONFIG/NP", "env", "E1-PMGT");
		//set_registry_value("/CONFIG/NP", "env", "E1-PQA");
		//set_registry_value("/CONFIG/NP", "env", "E1-SPINT");
		//set_registry_value("/CONFIG/NP", "env", "HF-NP");
		//set_registry_value("/CONFIG/NP", "env", "HF-PMGT");
		//set_registry_value("/CONFIG/NP", "env", "HF-PQA");
		//set_registry_value("/CONFIG/NP", "env", "HF-SPINT");
		//set_registry_value("/CONFIG/NP", "env", "H-NP");
		//set_registry_value("/CONFIG/NP", "env", "H-PMGT");
		//set_registry_value("/CONFIG/NP", "env", "H-PQA");
		//set_registry_value("/CONFIG/NP", "env", "H-SPINT");
		//set_registry_value("/CONFIG/NP", "env", "MGMT");
		//set_registry_value("/CONFIG/NP", "env", "NP");
		//set_registry_value("/CONFIG/NP", "env", "PMGT");
		//set_registry_value("/CONFIG/NP", "env", "PQA");
		//set_registry_value("/CONFIG/NP", "env", "PROD-QA");
		//set_registry_value("/CONFIG/NP", "env", "Q1");
		//set_registry_value("/CONFIG/NP", "env", "Q2");
		//set_registry_value("/CONFIG/NP", "env", "Q1-NP");
		//set_registry_value("/CONFIG/NP", "env", "Q2-NP");
		//set_registry_value("/CONFIG/NP", "env", "Q1-PGMT");
		//set_registry_value("/CONFIG/NP", "env", "Q2-PGMT");
		//set_registry_value("/CONFIG/NP", "env", "Q1-PQA");
		//set_registry_value("/CONFIG/NP", "env", "Q2-PQA");
		//set_registry_value("/CONFIG/NP", "env", "Q1-SPINT");
		//set_registry_value("/CONFIG/NP", "env", "Q2-SPINT");
		//set_registry_value("/CONFIG/NP", "env", "Q-NP");
		//set_registry_value("/CONFIG/NP", "env", "Q-PGMT");
		//set_registry_value("/CONFIG/NP", "env", "Q-PQA");
		//set_registry_value("/CONFIG/NP", "env", "Q-SPINT");
		//set_registry_value("/CONFIG/NP", "env", "RC");
		//set_registry_value("/CONFIG/NP", "env", "RC-NP");
		//set_registry_value("/CONFIG/NP", "env", "R-NP");
		//set_registry_value("/CONFIG/NP", "env", "R-PMGT");
		//set_registry_value("/CONFIG/NP", "env", "R-PQA");
		//set_registry_value("/CONFIG/NP", "env", "R-SPINT");
		//set_registry_value("/CONFIG/NP", "env", "SP-INT"); //developer
	*/
	
	SceCtrlData pad;
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
	
	int doonce=0;
	
	while(running()){

		sceCtrlReadBufferPositive(&pad, 1);
		
		//left a tab
		if(pad.Buttons & PSP_CTRL_LTRIGGER){
			if(top_ui_selector > 0){ 
				top_ui_selector--;
				thread_edit=0;
				registry_edit=0;
			}
			else{ top_ui_selector = 3;}
			sceKernelDelayThread(DELAY);
		}
		//right a tab
		if(pad.Buttons & PSP_CTRL_RTRIGGER){
			if(top_ui_selector < 3){ 
				top_ui_selector++;
				thread_edit=0;
				registry_edit=0;
			}
			else{ top_ui_selector = 0;}
			sceKernelDelayThread(DELAY);
		}
		
		if(top_ui_selector == 0){
			//up down
			if(pad.Buttons & PSP_CTRL_UP){
				if(threadSelected > 0){
					threadSelected--;
				}
				else if(threadSelected == 0){
					if(thread_scroll > DISPLAY_LINES){
						thread_scroll--; 
					}
				}
				sceKernelDelayThread(DELAY);
			}
			if(pad.Buttons & PSP_CTRL_DOWN){
				if(threadSelected < thread_count_now-1){
					threadSelected++;
				}
				else if(threadSelected == DISPLAY_LINES-1){ 
					if(thread_scroll < thread_count_now-DISPLAY_LINES-1){
						thread_scroll++; 
					}
				}
				sceKernelDelayThread(DELAY);
			}

			//speedy up down
			if(pad.Buttons & PSP_CTRL_LEFT){
				if(threadSelected > 0){
					threadSelected--;
				}
				sceKernelDelayThread(DELAY/10);
			}
			if(pad.Buttons & PSP_CTRL_RIGHT){
				if(threadSelected < thread_count_now-1){
					threadSelected++;
				}
				sceKernelDelayThread(DELAY/10);
			}
			
			//select and open/close info menu
			if(pad.Buttons & PSP_CTRL_CIRCLE){
				if(thread_edit){
					thread_edit=0;
				}
				sceKernelDelayThread(DELAY/10);
			}
			//select and open/close info menu
			if(pad.Buttons & PSP_CTRL_CROSS){
				if(!thread_edit){
					thread_edit=1;
					sceKernelGetThreadmanIdList(SCE_KERNEL_TMID_Thread, thread_buf_now, MAX_THREAD, &thread_count_now);
					int x;
					for(x = 0; x < thread_count_now; x++){
						if(x == threadSelected){
							selected_thid = thread_buf_now[x];
						}
					}
				}
				sceKernelDelayThread(DELAY/10);
			}
		}
		else if(top_ui_selector == 1){
			//registry edit
			if(registry_edit){
				//close any sub-menu's and reset their vars
				if(pad.Buttons & PSP_CTRL_CIRCLE){
					//keyboard
					keyboard_x_select=2,
					keyboard_y_select=5,
					keyboard_shift=0,
					keyboard_enabled=0,
					keyboard_input_strokes=0;
					//registryview
					registry_edit=0;
					//binary selectors
					bin_y_select=0;
					bin_x_select=0;
					sceKernelDelayThread(DELAY);
				}
				//edit mode controls
				switch(registry_array[reg_selector+reg_scroll].type){
					case REG_TYPE_INT:{
						//up down
						if(pad.Buttons & PSP_CTRL_UP){
							*((int*)(registry_array[reg_selector+reg_scroll].data))=*((int*)(registry_array[reg_selector+reg_scroll].data))+1;
							sceKernelDelayThread(DELAY);
						}
						if(pad.Buttons & PSP_CTRL_DOWN){
							*((int*)(registry_array[reg_selector+reg_scroll].data))=*((int*)(registry_array[reg_selector+reg_scroll].data))-1;
							sceKernelDelayThread(DELAY);
						}
						//up down
						if(pad.Buttons & PSP_CTRL_LEFT){
							*((int*)(registry_array[reg_selector+reg_scroll].data))=*((int*)(registry_array[reg_selector+reg_scroll].data))-1;
							sceKernelDelayThread(DELAY/10);
						}
						if(pad.Buttons & PSP_CTRL_RIGHT){
							*((int*)(registry_array[reg_selector+reg_scroll].data))=*((int*)(registry_array[reg_selector+reg_scroll].data))+1;
							sceKernelDelayThread(DELAY/10);
						}
						if(pad.Buttons & PSP_CTRL_CROSS){
							set_registry_value(&registry_array[reg_selector+reg_scroll]);
							sceKernelDelayThread(DELAY);
						}
					}
					break;
					case REG_TYPE_STR:{
						//not needed, button input in the registry view menu takes us to the keyboard instead
					}
					break;
					case REG_TYPE_BIN:{
						//edit holding x for now
						if(pad.Buttons & PSP_CTRL_CROSS){
							//saniks nitePR style up down editing of the bytes
							//up down
							if(pad.Buttons & PSP_CTRL_UP){
								*((unsigned char*)&registry_array[reg_selector+reg_scroll].data[bin_x_select+(bin_y_select*BROWSE_PITCH)])=*((unsigned char*)&registry_array[reg_selector+reg_scroll].data[bin_x_select+(bin_y_select*BROWSE_PITCH)])+0x10;
								sceKernelDelayThread(DELAY);
							}
							if(pad.Buttons & PSP_CTRL_DOWN){
								*((unsigned char*)&registry_array[reg_selector+reg_scroll].data[bin_x_select+(bin_y_select*BROWSE_PITCH)])=*((unsigned char*)&registry_array[reg_selector+reg_scroll].data[bin_x_select+(bin_y_select*BROWSE_PITCH)])-0x10;
								sceKernelDelayThread(DELAY);
							}
							//left right
							if(pad.Buttons & PSP_CTRL_LEFT){
								*((unsigned char*)&registry_array[reg_selector+reg_scroll].data[bin_x_select+(bin_y_select*BROWSE_PITCH)])=*((unsigned char*)&registry_array[reg_selector+reg_scroll].data[bin_x_select+(bin_y_select*BROWSE_PITCH)])-0x01;
								sceKernelDelayThread(DELAY);
							}
							if(pad.Buttons & PSP_CTRL_RIGHT){
								*((unsigned char*)&registry_array[reg_selector+reg_scroll].data[bin_x_select+(bin_y_select*BROWSE_PITCH)])=*((unsigned char*)&registry_array[reg_selector+reg_scroll].data[bin_x_select+(bin_y_select*BROWSE_PITCH)])+0x01;
								sceKernelDelayThread(DELAY);
							}
						}
						else{
							//regular byte browsing
							//up down
							if(pad.Buttons & PSP_CTRL_UP){
								if(bin_y_select > 0){
									bin_y_select--;
								}
								sceKernelDelayThread(DELAY);
							}
							if(pad.Buttons & PSP_CTRL_DOWN){
								if(bin_y_select < registry_array[reg_selector+reg_scroll].size/(BROWSE_PITCH-1)){
									bin_y_select++;
								}
								sceKernelDelayThread(DELAY);
							}
							//left right
							if(pad.Buttons & PSP_CTRL_LEFT){
								if(bin_x_select > 0){
									bin_x_select--;
								}
								sceKernelDelayThread(DELAY);
							}
							if(pad.Buttons & PSP_CTRL_RIGHT){
								if(bin_x_select < BROWSE_PITCH-1){
									bin_x_select++;
								}
								sceKernelDelayThread(DELAY);
							}
						}
					}
					break;
				}
			}
			//registry view
			else{
				//up down
				if(pad.Buttons & PSP_CTRL_UP){
					if(reg_selector > 0){
						reg_selector--;
					}
					else if(reg_selector == 0){
						if(reg_scroll > 0){
							reg_scroll--; 
						}
					}
					sceKernelDelayThread(DELAY);
				}
				if(pad.Buttons & PSP_CTRL_DOWN){
					if(reg_selector < DISPLAY_LINES-1){
						reg_selector++;
					}
					else if(reg_selector == DISPLAY_LINES-1){ 
						if(reg_scroll < REG_ARRAY_LEN-DISPLAY_LINES){
							reg_scroll++; 
						}
					}
					sceKernelDelayThread(DELAY);
				}
				//up down
				if(pad.Buttons & PSP_CTRL_LEFT){
					if(reg_selector > 0){
						reg_selector--;
					}
					else if(reg_selector == 0){
						if(reg_scroll > 0){
							reg_scroll--; 
						}
					}
					sceKernelDelayThread(DELAY/10);
				}
				if(pad.Buttons & PSP_CTRL_RIGHT){
					if(reg_selector < DISPLAY_LINES-1){
						reg_selector++;
					}
					else if(reg_selector == DISPLAY_LINES-1){ 
						if(reg_scroll < REG_ARRAY_LEN-DISPLAY_LINES){
							reg_scroll++; 
						}
					}
					sceKernelDelayThread(DELAY/10);
				}
				//pop up the registry view
				if(pad.Buttons & PSP_CTRL_CROSS){
					switch(registry_array[reg_selector+reg_scroll].type){
						case REG_TYPE_STR:{
							keyboard_enabled=1;
							strcpy(keyboard_buffer, (char*)registry_array[reg_selector+reg_scroll].data);
							//the keyboard control sub-loop
							int ret = keyboard_controls((char*)registry_array[reg_selector+reg_scroll].data);
							if(ret){
								//get the size of the buffer
								registry_array[reg_selector+reg_scroll].size = strlen(keyboard_buffer);
								//set the registry entry
								set_registry_value(&registry_array[reg_selector+reg_scroll]);
								//re-grab the registry entry to update the index
								get_registry_value(&registry_array[reg_selector+reg_scroll]);
								//clear the keyboard buffer for next use
								int x;
								for(x=0;x<KEYBOARD_BUFFER_SIZE;x++){
									keyboard_buffer[x]=0x00;
								}
							}
						}break;
						default:{
							registry_edit=1;
						}break;
					}
					sceKernelDelayThread(DELAY);
				}
				//reserved for close keyboard
			}
		
			if(real_psp && !doonce){
				loading_registry_screen=1;
				update_registry_array();
				loading_registry_screen=0;
				//save_registry_bin(registry_array);
				//load_registry_bin(registry_array);
				doonce=1;
			}
		}
		else if(top_ui_selector == 2){
			if(pad.Buttons & PSP_CTRL_CROSS){
			/*
				const char program[]="ms0:/PSP/GAME/TEST/EBOOT.PBP";
				struct SceKernelLoadExecParam param;
				param.size = sizeof(param);
				param.args = strlen(program)+1;
				param.argp = &program;
				param.key = "game"; //updater/game/vsh
				//sceKernelSuspendAllUserThreads();
				sceKernelLoadExec(program, &param);
				//sceKernelLoadExec(PROGRAM, 0);
			*/

				//moduleLoadStart("ms0:/test.prx", 0, NULL);
				//SceUID uid = sceKernelLoadModuleMs("ms0:/test.prx", 0, NULL);
				// if(uid){
				//	 sceKernelStartModule(uid, 0, 0, 0, NULL);
				// }
			}
		}
	
	}
	
	return 0;
	
}

SceUID setup_input(){
	//pspDebugScreenPuts("Starting Control Thread...\n");
	//Create the control thread
	SceUID thid;
	thid=sceKernelCreateThread("controlThread", &controlThread, 0x18, 1024, 0, NULL);
	//Start thread
	if(thid >= 0) {
		sceKernelStartThread(thid, 0, NULL);
	}
	return thid;
}

int main(int argc, char* argv[]) {

	static unsigned int __attribute__((aligned(16))) list[262144];
	extern unsigned char lightmap_start[];

	typedef struct Vx_v32f_n32f {
		float normal[3];
		float vertex[3];
	} Vx_v32f_n32f;

	typedef struct Vx_v32f {
		float vertex[3];
	} Vx_v32f;

	#define BUF_WIDTH (512)
	#define SCR_WIDTH (480)
	#define SCR_HEIGHT (272)
	#define PIXEL_SIZE (4) /* change this if you change to another screenmode */
	#define FRAME_SIZE (BUF_WIDTH * SCR_HEIGHT * PIXEL_SIZE)
	#define ZBUF_SIZE (BUF_WIDTH SCR_HEIGHT * 2) /* zbuffer seems to be 16-bit? */

	#define TORUS_SLICES 12 // numc
	#define TORUS_ROWS 12 // numt
	#define TORUS_RADIUS 1.0f
	#define TORUS_THICKNESS 0.25f
	#define LIGHT_DISTANCE 3.0f

	Vx_v32f_n32f   __attribute__((aligned(16))) torus_v0[TORUS_SLICES * TORUS_ROWS];
	Vx_v32f        __attribute__((aligned(16))) torus_v1[TORUS_SLICES * TORUS_ROWS];
	unsigned short __attribute__((aligned(16))) torus_in[TORUS_SLICES * TORUS_ROWS * 6];

	/* Generate Torus */
	unsigned int i,j;
	for (j = 0; j < TORUS_SLICES; ++j) {
		for (i = 0; i < TORUS_ROWS; ++i) {
			struct Vx_v32f_n32f* v0 = &torus_v0[i + j*TORUS_ROWS];
			struct Vx_v32f*      v1 = &torus_v1[i + j*TORUS_ROWS];
			
			float s = i + 0.5f;
			float t = j;
			float cs,ct,ss,st;

			cs = cosf(s * (2*GU_PI)/TORUS_SLICES);
			ct = cosf(t * (2*GU_PI)/TORUS_ROWS);
			ss = sinf(s * (2*GU_PI)/TORUS_SLICES);
			st = sinf(t * (2*GU_PI)/TORUS_ROWS);

			v0->normal[0] = cs * ct;
			v0->normal[1] = cs * st;
			v0->normal[2] = ss;

			v0->vertex[0] = (TORUS_RADIUS + TORUS_THICKNESS * cs) * ct;
			v0->vertex[1] = (TORUS_RADIUS + TORUS_THICKNESS * cs) * st;
			v0->vertex[2] = TORUS_THICKNESS * ss;
			
			v1->vertex[0] = v0->vertex[0] + v0->normal[0] * 0.1f;
			v1->vertex[1] = v0->vertex[1] + v0->normal[1] * 0.1f;
			v1->vertex[2] = v0->vertex[2] + v0->normal[2] * 0.1f;
		}
	}
	for (j = 0; j < TORUS_SLICES; ++j) {
		for (i = 0; i < TORUS_ROWS; ++i) {
			unsigned short* in = &torus_in[(i+(j*TORUS_ROWS))*6];
			unsigned int i1 = (i+1)%TORUS_ROWS, j1 = (j+1)%TORUS_SLICES;

			*in++ = i + j  * TORUS_ROWS;
			*in++ = i1 + j * TORUS_ROWS;
			*in++ = i + j1 * TORUS_ROWS;

			*in++ = i1 + j  * TORUS_ROWS;
			*in++ = i1 + j1 * TORUS_ROWS;
			*in++ = i  + j1 * TORUS_ROWS;
		}
	}

	sceKernelDcacheWritebackAll();

	SceUID callback_thid = setupCallbacks();
	SceUID input_thid     = setup_input();

	//pspDebugScreenPuts("Fetching information from registry...\n");
	real_psp = getVersionText();

	// else{
		//load_registry_bin(registry_array);
	// }

	//pspDebugScreenPuts("Initializing GU...\n");
	// setup GU
	sceGuInit();
	sceGuStart(GU_DIRECT,list);
	sceGuDrawBuffer(GU_PSM_8888,(void*)0,BUF_WIDTH);
	sceGuDispBuffer(SCR_WIDTH,SCR_HEIGHT,(void*)0x88000,BUF_WIDTH);
	sceGuDepthBuffer((void*)0x110000,BUF_WIDTH);
	sceGuOffset(2048 - (SCR_WIDTH/2),2048 - (SCR_HEIGHT/2));
	sceGuViewport(2048,2048,SCR_WIDTH,SCR_HEIGHT);
	sceGuDepthRange(0xc350,0x2710);
	sceGuScissor(0,0,SCR_WIDTH,SCR_HEIGHT);
	sceGuEnable(GU_SCISSOR_TEST);
	sceGuDepthFunc(GU_GEQUAL);
	sceGuEnable(GU_DEPTH_TEST);
	sceGuFrontFace(GU_CW);
	sceGuShadeModel(GU_SMOOTH);
	sceGuEnable(GU_CULL_FACE);
	sceGuEnable(GU_CLIP_PLANES);
	sceGuFinish();
	sceGuSync(0,0);
	sceDisplayWaitVblankStart();
	sceGuDisplay(GU_TRUE);

	loadfonts();

	float rx = 0.0f, ry = 0.0f, rz = 0.0f;
	
	while(running()){
		
		sceGuStart(GU_DIRECT,list);

		sceGuClearColor(0xFF444444);
		sceGuClearDepth(0);
		sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT);

		sceGumMatrixMode(GU_PROJECTION);
		sceGumLoadIdentity();
		sceGumPerspective(75.0f,16.0/9.0f,0.1f,1000.0f);

		sceGumMatrixMode(GU_VIEW);
		sceGumLoadIdentity();
		
		sceGumMatrixMode(GU_MODEL);
		sceGumLoadIdentity();
		ScePspFVector3 pos = { 0.0f, 0.0f, -2.5f };
		ScePspFVector3 rot = { ry * (GU_PI/180.0f), rx * (GU_PI/180.0f), rz * (GU_PI/180.0f) };
		sceGumTranslate(&pos);
		sceGumRotateXYZ(&rot);

		sceGuTexMode(GU_PSM_8888,0,0,0);
		sceGuTexImage(0,64,64,64, lightmap_start);
		sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGB);
		sceGuTexFilter(GU_LINEAR,GU_LINEAR);
		
		sceGuColor(0xFFCCCCCC);
	
		ScePspFVector3 envmapMatrixColumns[2] = {
			{ 1.0f, 0.0f, 0.0f },
			{ 0.0f, 1.0f, 0.0f }
		};
		sceGuLight( 0, GU_DIRECTIONAL, GU_DIFFUSE, &envmapMatrixColumns[0] );
		sceGuLight( 1, GU_DIRECTIONAL, GU_DIFFUSE, &envmapMatrixColumns[1] );

		sceGuTexMapMode(GU_ENVIRONMENT_MAP,	0, 1);
		
		sceGumDrawArray(GU_TRIANGLES,
			GU_NORMAL_32BITF|GU_VERTEX_32BITF|GU_INDEX_16BIT|GU_TRANSFORM_3D,
			sizeof(torus_in)/sizeof(unsigned short), torus_in, torus_v0
		);

		sceGuEnable(GU_BLEND);
  		sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);

		draw_ui();
		
    	sceGuDisable(GU_BLEND);

		sceGuFinish();
		sceGuSync(0,0);

		rx +=1.5f;
		ry -=0.5f;
		//rz -=1.2f;


		sceDisplayWaitVblankStart();
		sceGuSwapBuffers();
	}

	sceGuTerm();


	// Unload our latin fonts
	for (i = 0; i < 16; i++) {
		intraFontUnload(ltn[i]);
	}
	// Unload our japanese font
	intraFontUnload(jpn0);


	// Shutdown font library
	intraFontShutdown();
	sceKernelTerminateThread(input_thid);
	sceKernelTerminateThread(callback_thid);
	sceKernelExitGame();
	
	return 0;
	
}
