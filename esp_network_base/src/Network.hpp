#include <Arduino.h>

#include "config.h"
#include "WiFi.h"


#ifndef NETWORK_H
#define NETWORK_H

class Network
{
public:
    bool verbose;

    Network(bool verbose = true) : verbose(verbose) {};

    bool connect_network(int max_attempts = 0)
    {
        int config_idx = 0;

        while (WiFi.status() != WL_CONNECTED)
        {
            // Get the config to test
            config_t config = configs[config_idx];
            if (verbose)
                Serial.printf("Try connection to %s.", config.ssid, config.password);
            // Try to connect
            WiFi.begin(config.ssid, config.password);

            // Wait while init packet exchanges
            int wait_loops = 10;
            while (wait_loops > 0 and WiFi.status() != WL_CONNECTED)
            {
                delay(500);
                if (verbose)
                    Serial.print('.');
                wait_loops -= 1;
            }
            // If not connected
            if (WiFi.status() != WL_CONNECTED)
            {
                if (verbose)
                    Serial.println("");
                // Change the config to test
                config_idx = (config_idx + 1) % num_configs;
                // If all configs has been tested
                if (config_idx == 0)
                {
                    // Not checking max attempts on 0
                    if (max_attempts != 0)
                    {
                        // Return on connection failure
                        if (max_attempts == 1)
                            return false;
                        // Reduce the number of connection attempts.
                        max_attempts -= 1;
                    }
                }
            }
        }

        if (verbose)
        {
            Serial.print(" Connected on ");
            Serial.println( WiFi.localIP());
        }

        return true;
    }
};

#endif
