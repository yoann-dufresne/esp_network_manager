#include <Arduino.h>

#include "config.h"
#include "WiFi.h"


#ifndef NETWORK_H
#define NETWORK_H

class Network
{
public:
    bool verbose;
    config_t config;
    WiFiClient socket;

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

        // Connection ok
        this->config = configs[config_idx];
        if (verbose)
        {
            Serial.print(" Connected on ");
            Serial.println( WiFi.localIP());
        }

        return true;
    };

    bool connect_server(int max_attempts = 0)
    {
        if (this->verbose)
            Serial.printf("Try to reach server %s:%i ", this->config.server_ip, this->config.server_port);
        while (!this->socket.connect(this->config.server_ip, this->config.server_port)) {
            if (this->verbose)
                Serial.print('.');
            
            if (max_attempts == 1)
            {
                if (this->verbose)
                    Serial.println("\nServer not found. Abort");
                return false;
            }
            else if (max_attempts != 0)
            {
                max_attempts -= 1;
                delay(200);
            }
        }

        // Send the first message of the protocol
        char * connec_msg = "ESP real\n";
        uint8_t * msg = (uint8_t *)connec_msg;
        this->socket.write(connec_msg, 9);
        this->socket.flush();
        // Read acknowledgment
        while (this->socket.isconnected() and this->socket.available() == 0)
        { delay(10); }

        return true;
    };
};

#endif
