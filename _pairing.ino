/*
 * Companion (client) device could obtain some configs (ssid/psk/hostname/ip address) from the host/server
 * Steps:
 * 1- client start up and broadcast to all wifi channels
 * 2- Manual action: the server/host starts listning and receives the pairing request
 * 3- host add client to peer list and send back the data
 * 4- client recieves and proces the data 
 * 5- client send ack back when everthing is okay
 * 6- server closed the service
 * 
 * based on; https://randomnerdtutorials.com/esp-now-auto-pairing-esp32-esp8266/
 * 
 */

#ifdef DEV_PAIRING

#ifndef __PAIRING //only once
#define __PAIRING

#include "./../../_secrets/pairing.h"

esp_now_peer_info_t slave;
enum MessageType { PAIRING, DATA, CONFIRMED, } messageType;

int chan;

typedef struct struct_pairing {
    uint8_t msgType;
    char    ssid[32];    //max 32
    char    pw[63];      //max 63
    char    host[30];    //max 30
    uint8_t ipAddr[4];  //max 4
} struct_pairing;

struct_pairing pairingData, recvdata;

// ---------------------------- esp_ now -------------------------
void printMAC(const uint8_t * mac_addr){
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Debug(macStr);
}

bool addPeer(const uint8_t *peer_addr) {      // add pairing
  memset(&slave, 0, sizeof(slave));
  const esp_now_peer_info_t *peer = &slave;
  memcpy(slave.peer_addr, peer_addr, 6);
  
  slave.channel = chan; // pick a channel
  slave.encrypt = 0; // no encryption
  // check if the peer exists
  bool exists = esp_now_is_peer_exist(slave.peer_addr);
  if (exists) {
    // Slave already paired.
    Debugln("Already Paired");
    return true;
  }
  else {
    esp_err_t addStatus = esp_now_add_peer(peer);
    if (addStatus == ESP_OK) {
      // Pair success
      Debugln("Pair success");
      return true;
    }
    else 
    {
      Debugln("Pair failed");
      return false;
    }
  }
} 

void StopPairing(){
    Debugln("Stop Pairing");
    esp_now_deinit(); //stop service
    WiFi.mode(WIFI_STA); //set wifi to STA  
}


void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) { 
  Debugf("%i bytes of data received from : ",len);printMAC(mac_addr);Debugln();
    switch ( (uint8_t)incomingData[0] ){
      case CONFIRMED:
        Debugln("Confirmed");
        StopPairing();
        break;
      case PAIRING:
        memcpy(&recvdata, incomingData, sizeof(recvdata));
        Debug("pw: ");Debugln(recvdata.pw);
        if ( strcmp(recvdata.pw, CONFIGPW ) == 0 ){
        Debug("msgType: ");Debugln(pairingData.msgType);
  //    Debugln(pairingData.ssid);
  //    Debugln(pairingData.host);
  //    Debugln("send response");
        addPeer(mac_addr);
        esp_err_t result = esp_now_send(mac_addr, (uint8_t *) &pairingData, sizeof(pairingData));
      }  else Debugln("Incorrect pw");
        break;
      case DATA:
        //komt hier niet
        break;  
    }  
}

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Debug("Last Packet Send Status: ");
  Debug(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success to " : "Delivery Fail to ");
  printMAC(mac_addr);
  Debugln();
}

void HandlePairing() {
  if ( httpServer.arg("action") == "stop" ) {
    StopPairing();
    httpServer.send(200, "text/plain", "Pairing Stoped" );
  }
  if ( httpServer.arg("action") == "start" ) {
    StartPairing();
    httpServer.send(200, "text/plain", "Pairing Started" );
  }
}

void StartPairing() {
  
  WiFiManager manageWiFi;
  
  Debug("Server MAC Address:  ");Debugln(WiFi.macAddress());

  Debug("Server SOFT AP MAC Address:  "); Debugln(WiFi.softAPmacAddress());

  chan = WiFi.channel();
  Debug("Station IP Address: "); Debugln(WiFi.localIP());
  Debug("Wi-Fi Channel: ");  Debugln(WiFi.channel());
  
  //prepair the hostdata
  for ( int i=0 ; i<4 ; i++ ) pairingData.ipAddr[i] = WiFi.localIP()[i];
  strcpy( pairingData.ssid, WiFi.SSID().c_str() );
  strcpy( pairingData.pw, manageWiFi.getWiFiPass().c_str() );
  strcpy( pairingData.host, settingHostname );
  pairingData.msgType = DATA;

//wifi manager zo doen:
//    String        manageWiFi.getWiFiPass();
//    String        manageWiFi.getWiFiSSID();

  // Set the device as a Station and Soft Access Point simultaneously
  WiFi.mode(WIFI_AP_STA);

  if (esp_now_init() != ESP_OK) {
      Debugln("Error initializing ESP-NOW");
      return;
    }
    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(OnDataRecv);
    
    Debugln("Pairing started");
}

#endif //_PAIRING
#endif //PAIRING
