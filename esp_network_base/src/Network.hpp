#include <Arduino.h>
#include <WiFi.h>

#include "config.h"



#ifndef NETWORK_H
#define NETWORK_H

class Network
{
public:
    bool verbose;
    config_t config;
    WiFiClient socket;

    int msg_size;
    uint8_t * msg_buffer;

    Network(bool verbose = true) : verbose(verbose), msg_size(0), msg_buffer(new uint8_t[256]) {
        WiFi.mode(WIFI_STA);
        WiFi.disconnect();
    };

    ~Network()
    {
        delete[] msg_buffer;
    }

    bool is_connected_on_wifi()
    {
        return WiFi.status() == WL_CONNECTED;
    }

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

            if (WiFi.status() != WL_CONNECTED)
            {
                int n = WiFi.scanNetworks();
                Serial.println(" networks found");
                for (int i = 0; i < n; ++i) {
                    // Print SSID and RSSI for each network found
                    Serial.print(i + 1);
                    Serial.print(": ");
                    Serial.print(WiFi.SSID(i));
                    Serial.print(" (");
                    Serial.print(WiFi.RSSI(i));
                    Serial.print(")");
                    Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
                    delay(10);
                }
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
            Serial.println(WiFi.localIP());
        }

        return true;
    };


    bool is_connected_to_server()
    {
        return this->socket.connected();
    }

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
        Serial.println();
        // Wait for the connection to be fully operational
        delay(500);

        // Send the first message of the protocol
        uint8_t connec_msg[12] = {'E', 'S', 'P', ' ', 0, 0, 0, 0, 0, 0, 0, 0};
        WiFi.macAddress(connec_msg+4);
        return this->send(connec_msg, 12);
    };


    /** Read the next message on the socket and return the buffer containing it.
     * Maximum message size is 255. Stop waiting for the message after time to wait ms
    */
    uint8_t * read_message(const unsigned long time_to_wait = 100ul)
    {
        unsigned long last_contact = millis();
        bool msg_over = false;
        this->msg_size = 0;
        int idx = 0;

        // Construct the message
        while (not msg_over)
        {
            // Waiting for the next bytes to arrive
            while (this->socket.available() == 0)
            {
                if (millis() - last_contact > time_to_wait)
                {
                    this->msg_size = 0;
                    return nullptr;
                }
            }

            // Dertermine the message size
            if (this->msg_size == 0) this->msg_size = this->socket.read();

            // Read available bytes from the current message
            int bytes_to_read = min(this->msg_size, this->socket.available());
            for ( ; idx<bytes_to_read ; idx++)
            {
                this->msg_buffer[idx] = this->socket.read();
            }

            if (idx == this->msg_size)
                msg_over = true;
            last_contact = millis();
        }

        return this->msg_buffer;
    };

    /** Send a message through the TCP connection
     * @param msg Message to transfer. Max size is 255.
     * @param size Size of the message
    */
    bool send(uint8_t * msg, uint8_t size)
    {
        const size_t max_attempts = 1;
        const unsigned long time_to_ack = 50ul;
        //bool sent = false;
        size_t nb_attempts = 0;

        uint8_t * buffer = new uint8_t[size+1];
        buffer[0] = size;
        memcpy(buffer+1, msg, size);

        Serial.printf("Sending msg: (%d) ", size);
        for (int i(0) ; i<size+1 ; i++)
        {
            Serial.print(buffer[i]);
            Serial.print(' ');
        }
        Serial.println();

        size_t sent = this->socket.write(buffer, size+1);
        this->socket.flush(); 

        delete[] buffer;
        return sent == size+1;
    };
};

#endif
