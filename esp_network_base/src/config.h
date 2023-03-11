
#ifndef CONFIG_H
#define CONFIG_H


struct config_s
{
	char* ssid;
	char* password;
	char* server_ip;
	int server_port;
};
typedef struct config_s config_t;


const config_t configs[2] = {
	{
		"Your Wifi network name 1",
		"Your password 1",
		"The IP server address 1",
		15555
	},
	{
		"Your Wifi network name 2",
		"Your password 2",
		"The IP server address 2",
		15556
	}
};

#endif