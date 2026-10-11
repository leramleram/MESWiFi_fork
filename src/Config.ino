
#include <FS.h>
#include <SPIFFS.h>

#include "Config.h"

t_cfg current_config;
bool config_valid = false;

void cfg_save()
{
    File file = SPIFFS.open("/config.dat", "w");
    if (!file || file.isDirectory())
    {
        return;
    }

    if (strlen(current_config.hostname) < 2)
    {
        strcpy(current_config.hostname, CONFIG_OTANAME);
    }

    file.write((uint8_t *)&current_config, sizeof(current_config));
    file.close();
}

void cfg_reset()
{
    memset(&current_config, 0x00, sizeof(current_config));

    current_config.magic = CONFIG_MAGIC;
    strcpy(current_config.hostname, CONFIG_OTANAME);
    strcpy(current_config.mqtt_server, "");
    current_config.mqtt_port = 11883;
    strcpy(current_config.mqtt_user, "");
    strcpy(current_config.mqtt_password, "");
    strcpy(current_config.mqtt_client, CONFIG_OTANAME);
    current_config.mqtt_publish = 0;


    strcpy(current_config.wifi_ssid, "(not set)");
    strcpy(current_config.wifi_password, "(not set)");
}

void cfg_read()
{
    File file = SPIFFS.open("/config.dat", "r");

    config_valid = false;

    if (!file || file.isDirectory())
    {
        cfg_reset();
    }
    else
    {
        file.read((uint8_t *)&current_config, sizeof(current_config));
        file.close();

        /* Einstellungen aus dem alten Format (MESWiFi defa4d2, Magic 0xE1AAFF09) übernehmen,
           sonst startet das Gerät nach dem Update ohne WLAN/MQTT im Einrichtungsmodus */
        if (current_config.magic == 0xE1AAFF09)
        {
            struct
            {
                uint32_t magic;
                uint32_t version;
                uint32_t verbose;
                uint32_t mqtt_publish;
                char hostname[32];
                char wifi_ssid[32];
                char wifi_password[32];
                char mqtt_server[32];
                int mqtt_port;
                char mqtt_user[32];
                char mqtt_password[32];
                char mqtt_client[32];
            } alt;
            static_assert(sizeof(alt) == sizeof(current_config), "Konfiguration: Größe alt/neu verschieden");
            memcpy(&alt, &current_config, sizeof(alt));

            cfg_reset();
            strncpy(current_config.hostname, alt.hostname, sizeof(current_config.hostname) - 1);
            strncpy(current_config.wifi_ssid, alt.wifi_ssid, sizeof(current_config.wifi_ssid) - 1);
            strncpy(current_config.wifi_password, alt.wifi_password, sizeof(current_config.wifi_password) - 1);
            strncpy(current_config.mqtt_server, alt.mqtt_server, sizeof(current_config.mqtt_server) - 1);
            current_config.mqtt_port = alt.mqtt_port;
            strncpy(current_config.mqtt_user, alt.mqtt_user, sizeof(current_config.mqtt_user) - 1);
            strncpy(current_config.mqtt_password, alt.mqtt_password, sizeof(current_config.mqtt_password) - 1);
            strncpy(current_config.mqtt_client, alt.mqtt_client, sizeof(current_config.mqtt_client) - 1);
            current_config.mqtt_publish = alt.mqtt_publish;
            cfg_save();
            config_valid = true;
            return;
        }

        if (current_config.magic != CONFIG_MAGIC)
        {
            /* on a minor version change, just keep wifi settings and hostname */
            if ((current_config.magic & ~0xF) == (CONFIG_MAGIC & ~0xF))
            {
                char hostname[32];
                char wifi_ssid[32];
                char wifi_password[32];

                strcpy(hostname, current_config.hostname);
                strcpy(wifi_ssid, current_config.wifi_ssid);
                strcpy(wifi_password, current_config.wifi_password);

                cfg_reset();

                strcpy(current_config.hostname, hostname);
                strcpy(current_config.wifi_ssid, wifi_ssid);
                strcpy(current_config.wifi_password, wifi_password);
                config_valid = true;
            }
            else
            {
                cfg_reset();
            }
            return;
        }
        config_valid = true;
    }
}
