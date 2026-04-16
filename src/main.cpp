#include <stdio.h>
#include <cstring>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "daas.hpp"
#include "daas_nodes.h"
#include "datatypes.h"

bool nodeConnected = false;
bool requestedLogs = false;
din_t foundDIN;

const TickType_t periodStatus = pdMS_TO_TICKS(10000);
TickType_t lastSend = 0;

class event_daas : public IDaasApiEvent {
public:
    void dinAccepted(din_t din) override {
        printf("[DIN Accepted] DIN: %d\n", (din<<44));
        foundDIN = din;
    }
    void ddoReceived(int payload_size, typeset_t typeset, din_t din) override {
        printf("DDO Received\n");
        if(typeset == 3410) requestedLogs = true;
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
#define LOG_TS 3400
uint8_t rand_range(uint8_t min, uint8_t max)
{
    if (min > max) {
        uint8_t tmp = min;
        min = max;
        max = tmp;
    }

    return (uint8_t)(min + (rand() % (max - min + 1)));
}

sensors_t generate_random_sensors() {
    sensors_t s;

    s.heat = (int8_t)rand_range(40, 50);

    s.climate_temp = rand_range(25, 30);

    s.climate_hum = (uint8_t)rand_range(30, 35);

    s.rumor = (uint8_t)rand_range(5,15);

    s.daylight = (uint8_t)rand_range(50,60);

    s.presence = (uint8_t)1;

    return s;
}

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

void sendLogData(log_record_t data){
    DDO out(LOG_TS);

    // uint8_t buffer[1] = {data};

    constexpr uint32_t packetSize = sizeof(log_record_t);

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
        // #ifdef NODEA
        if(!nodeConnected) {
            // daas.discovery();
            printf("Discovery \n");
        }
        // #endif
        TickType_t now = xTaskGetTickCount();
        if(now-lastSend >= periodStatus){
            if(nodeConnected){
                printf("Connected\n");
                sensors_t sensors = generate_random_sensors();
                dev_stat_t data;
                data.sensors = sensors;
                data.devicetime = 1776268445;
                data.source = 1;
                data.charge = 80;
                data.charge_status = 3;
                data.memory = 10;
                data.link = 3;
                    
                sendData(data);
                receiveData();
                lastSend = now;
            }
        }
        if(requestedLogs){
            for(int i=0; i<5; i++){
                log_record_t record;
                record.devicetime = 1776268445 + rand_range(10,20);
                record.id = rand_range(1,10);
                record.value = rand_range(1,10);
                sendLogData(record);
            }
            requestedLogs = false;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
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

    daas.addTypeset(3110, [](din_t din) {
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