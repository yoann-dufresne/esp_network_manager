#include <WiFi.h>
#include <esp_now.h>


#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

void formatMacAddress(const uint8_t *macAddr, char *buffer, int maxLength);
uint64_t mac2uint(const uint8_t * macAddr);


class WifiManager {
protected:
    bool verbose;
    uint8_t macaddress[6];

public:
    WifiManager();
    bool init();
    void setVerbose(bool verbose);

    void basic_callback(const uint8_t *macAddr, const uint8_t *data, int dataLen);
    void set_receive_callback(esp_now_recv_cb_t callback);
    bool broadcast(const String &message);
};

/** The aim of this class is to register all the boards on the local esp network and give them a parent to communicate with. The network
 * is conceived as a hierarchical network. This registry is central and can have 10 children. Each child can also have 10 children. Each
 * board on the network can only talk to its parent and can only be contacted by its children. It has to be the relay of the children
 * communication with this registry. This spider web is made to avoid the limitation of 20 esp maximum per direct communication. All other
 * messages are broadcasted.
*/
class ServerWifiManager : public WifiManager {
private:
    uint64_t server_uint_mac;

    uint8_t nb_boards;
    uint8_t max_boards;
    // all registered boards to that server
    uint64_t * registry;
    // assigned parent boards on first contact
    uint64_t * parents;
public:
    ServerWifiManager();
    ~ServerWifiManager();
    void callback(const uint8_t *macAddr, const uint8_t *data, int dataLen);
    uint64_t new_device(uint64_t mac);
};

#endif
