#include <stdio.h>
#include <cstring>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "daas.hpp"
#include "daas_nodes.h"
#include "datatypes.h"

bool nodeConnected = false;
din_t foundDIN;

class event_daas : public IDaasApiEvent {
public:
    void dinAccepted(din_t din) override {
        printf("[DIN Accepted] DIN: %d\n", (din<<44));
        foundDIN = din;
    }
    void ddoReceived(int payload_size, typeset_t typeset, din_t din) override {
        printf("DDO Received\n");
    }
    void frisbeeReceived(din_t) override {}
    void nodeStateReceived(din_t) override {}
    void atsSyncCompleted(din_t) override {}
    void frisbeeDperfCompleted(din_t, uint32_t packets_sent, uint32_t block_size) override {}
    void nodeDiscovered(din_t din, link_t link) override {
        printf("[Discovery] DIN: %d\n", (din>>44));
        printf("-------------------");
    }
    void nodeConnectedToNetwork(din_t sid, din_t din) override {
        printf("[nodeConnected] DIN: %d\n", (din<<44));
        nodeConnected = true;
        
    }
    virtual void streamInfoReceived(din_t din, stream_type pkt_type, uint32_t stream_id) override {}

};
event_daas *event = new event_daas();
DaasAPI daas(event);
// DaasAPI daas;

dev_stat_t status;
#define STATUS_TS 3110

void sendData(dev_stat_t data){
    DDO out(STATUS_TS);

    // uint8_t buffer[1] = {data};

    constexpr uint32_t packetSize = sizeof(dev_stat_t);

    uint8_t buffer[packetSize];
    memset(buffer, 0, packetSize);          // buffer pulito
    memcpy(buffer, &data, packetSize);     // copia sicura

    out.allocatePayload(packetSize);
    out.setPayload(buffer, packetSize);

    daas_error_t res = daas.push(foundDIN, &out);
    if(res == ERROR_NONE) printf("[PUSH] Sent data");
    else printf("[PUSH] Error %d",res);
}


bool receiveData(void){
    DDO *pk;

    if(daas.pull(foundDIN, &pk) != ERROR_NONE) return false;

    uint8_t *payload = pk->getPayloadPtr();

    delete pk;

    printf("[PULL] Received: %d \n", payload[0]);
    return true;
}

void periodicTask(void *parameter){
    for(;;){
        #ifdef NODEA
        if(!nodeConnected) {
            daas.discovery();
            printf("Discovery \n");
        }
        #endif
        if(nodeConnected){
            printf("Connected\n");
        sendData(status);
        receiveData();}
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

// In ESP-IDF non esistono setup() e loop(). Esiste solo app_main()
extern "C" void app_main() {
    printf("--- Boot Heltec V3 (ESP-IDF) ---\n");

    // 1. Inizializza NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        ret = nvs_flash_init();
    }

    // 2. Accendi il Bluetooth (ora esp-nimble-cpp è a casa sua e non crasherà)
    daas_error_t err = daas.enableDriver(_LINK_BLE, local_uri); 
    if (err != ERROR_NONE) {
        printf("Failed to enable driver: %d\n", err);
    } else {
        printf("BLE Driver enabled successfully.\n");
    }

    // 3. Inizializza il resto del nodo Daas
    daas.setATSMaxError(1000);
    daas.doInit(100, localDIN); 

    daas.setDiscoveryState(discovery_full); // Abilita la discovery completa

    daas.addTypeset(1, [](din_t din) {
        printf("Typeset 1 function called for DIN: %lu\n", din);
    });

    xTaskCreate(periodicTask, "DaaS Task", 4096, NULL, 1, NULL);
    // print
    printf("DaaS API Version: %s\n", daas.getVersion());
    printf("DaaS API Build Info: %s\n", daas.getBuildInfo());
    printf("Available Drivers: %s\n", daas.listAvailableDrivers());
    
    // err = daas.map(remoteDIN, _LINK_BLE, remote_uri);
    // if(err == ERROR_NONE) printf("Remote DIN mapped\n");
    // err = daas.locate(remoteDIN);
    // if(err == ERROR_NONE) printf("Remote DIN located\n");

    // Al posto del loop(), crei un task o metti un delay infinito
    while (1) {
      daas.doPerform(PERFORM_CORE_NO_THREAD); // Esegui il core in modalità real-time
      vTaskDelay(pdMS_TO_TICKS(10));
    }
}