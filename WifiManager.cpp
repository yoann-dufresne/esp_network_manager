#include "WifiManager.hpp"
#include <functional>


void formatMacAddress(const uint8_t *macAddr, char *buffer, int maxLength) {
    snprintf(buffer, maxLength, "%02x:%02x:%02x:%02x:%02x:%02x", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
};

uint64_t mac2uint(const uint8_t * macAddr) {
    uint64_t mac_uint = 0;
    for (size_t i=0 ; i<6 ; i++) {
        mac_uint <<= 8;
        mac_uint += macAddr[i];
    }
    return mac_uint;
}


WifiManager * currentManager;
ServerWifiManager * currentServer;


void WifiManager::basic_callback(const uint8_t *macAddr, const uint8_t *data, int dataLen) {
    // Prepare mesage
    char buffer[ESP_NOW_MAX_DATA_LEN + 1];
    int msgLen = min(ESP_NOW_MAX_DATA_LEN, dataLen);
    strncpy(buffer, (const char *)data, msgLen);
    buffer[msgLen] = '\0';

    // Format the MAC address
    char macStr[18];
    formatMacAddress(macAddr, macStr, 18);

    if (verbose)
        // Send Debug log message to the serial port
        Serial.printf("Received message from: %s - %s\n", macStr, buffer);
}

void static_basic_callback(const uint8_t *macAddr, const uint8_t *data, int dataLen) {
    currentManager->basic_callback(macAddr, data, dataLen);
}

WifiManager::WifiManager() {
    this->verbose = false;
    memset(this->macaddress, 0, 6);
}

bool WifiManager::init() {
    WiFi.mode(WIFI_STA);
    if (this->verbose)
        Serial.println("ESP-NOW Broadcast receiver");

    // Mac Address
    WiFi.macAddress(this->macaddress);
    this->mac_uint = mac2uint(this->macaddress);

    Serial.print("My MAC Address: ");
    Serial.println(WiFi.macAddress());

    // Disconnect from WiFi
    WiFi.disconnect();

    // Initialize ESP-NOW
    if (esp_now_init() == ESP_OK) {
        // Register callback
        Serial.println("ESP-NOW Init Success");
        currentManager = this;
        esp_now_register_recv_cb(static_basic_callback);

        // Send registration message
        for (size_t i=0 ; i<100 ; i++) {
            this->broadcast("C");
            delay(1000);
        }
    } else {
        Serial.println("ESP-NOW Init Failed");
        // delay(3000);
        // ESP.restart();
        return false;
    }
    
    return true;
};

void WifiManager::setVerbose(bool verbose) {
    this->verbose = verbose;
};


void WifiManager::set_receive_callback(esp_now_recv_cb_t callback) {
    esp_now_register_recv_cb(callback);
}


bool WifiManager::send(uint64_t destination, uint8_t * msg, size_t len) {
    return true;
}


bool WifiManager::broadcast(const String &message) {
    // Broadcast a message to every device in range
    uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    esp_now_peer_info_t peerInfo = {};
    memcpy(&peerInfo.peer_addr, broadcastAddress, 6);
    if (!esp_now_is_peer_exist(broadcastAddress)) {
        esp_now_add_peer(&peerInfo);
    }
    // Send message
    esp_err_t result = esp_now_send(broadcastAddress, (const uint8_t *)message.c_str(), message.length());

    if (this->verbose) {
        Serial.printf("Sending: %s\n", message);
        // Print results to serial monitor
        if (result = ESP_OK) {
            Serial.println("Broadcast message success");
        } else if (result == ESP_ERR_ESPNOW_NOT_INIT) {
            Serial.println("ESP-NOW not Init.");
        } else if (result == ESP_ERR_ESPNOW_ARG) {
            Serial.println("Invalid Argument");
        } else if (result == ESP_ERR_ESPNOW_INTERNAL) {
            Serial.println("Internal Error");
        } else if (result == ESP_ERR_ESPNOW_NO_MEM) {
            Serial.println("ESP_ERR_ESPNOW_NO_MEM");
        } else if (result == ESP_ERR_ESPNOW_NOT_FOUND) {
            Serial.println("Peer not found.");
        } else {
            Serial.println("Unknown error");
        }
    }

    return result == ESP_OK;
};




ServerWifiManager::ServerWifiManager() : WifiManager() {
    this->server_uint_mac = mac2uint(this->macaddress);
    this->nb_boards = 0;
    this->max_boards = 2;
    this->registry = new uint64_t[this->max_boards];
    this->parents = new uint64_t[this->max_boards];
};

ServerWifiManager::~ServerWifiManager() {
    delete[] this->registry;
    delete[] this->parents;
}

void ServerWifiManager::callback(const uint8_t *macAddr, const uint8_t *data, int dataLen) {
    // Empty message
    if (dataLen == 0)
        return;

    uint64_t uint_mac = mac2uint(macAddr);

    // Connection request
    if (data[0] == (uint8_t)'C') {
        uint64_t parent_mac = this->new_device(uint_mac);
        
        if (verbose) {
            Serial.printf("New device %s. Assigned parent %s", uint_mac, parent_mac);
        }

        // Send a direct connection request
        if (this->mac_uint == parent_mac) {
            uint8_t msg[1]; msg[0] = 'L';
            this->send(uint_mac, msg, 1);
        }
        // Send indirect connection request
        else {
            uint8_t msg[9]; msg[0] = 'T'; msg[1] = 'D'; msg[8] = 'L';
            for (size_t i=0 ; i<6 ; i++)
                msg[2+i] = macAddr[i];
            this->send(parent_mac, msg, 9);
        }
    } else {
        WifiManager::basic_callback(macAddr, data, dataLen);
    }
}


uint64_t ServerWifiManager::new_device(uint64_t mac) {
    // Verify not already registered
    bool registered = false;
    size_t i=0;
    for ( ; i<this->nb_boards ; i++) {
        if (this->registry[i] == mac) {
            registered = true;
        }
    }

    // Add the new device
    if (! registered) {
        // If not enought space
        if (i == this->max_boards) {
            // Registry transfer
            uint64_t * tmp_space = new uint64_t[2 * this->max_boards];
            memccpy(tmp_space, this->registry, sizeof(uint64_t), this->max_boards);
            delete[] this->registry; this->registry = tmp_space;
            // Parents transfer
            tmp_space = new uint64_t[2 * this->max_boards];
            memccpy(tmp_space, this->parents, sizeof(uint64_t), this->max_boards);
            delete[] this->parents; this->parents = tmp_space;

            this->max_boards *= 2;
        }

        // Define the parent;
        if (i < 10)
            this->parents[i] = this->server_uint_mac;
        else { // TODO : What for more than 110 perifericals ?
            this->parents[i] = i % 10;
        }

        // Register
        this->registry[i] = mac;
        this->nb_boards += 1;
    }

    // Return the parent value
    return this->parents[i];
};
