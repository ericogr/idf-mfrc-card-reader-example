#include <esp_log.h>
#include <driver/spi_master.h>
#include <freertos/task.h>
#include <esp_heap_caps.h>
#include "MFRC522.h"

static const char* TAG = "cardreader";

SPI _SPI(VSPI_HOST);
MFRC522 _MFRC522(&_SPI, 22);

void searchNewCard(void * pvParameters)
{
    ESP_LOGI(TAG, "Searching card...");

    for( ;; )
    {
        bool haveCard = _MFRC522.PICC_IsNewCardPresent();

        if (haveCard && _MFRC522.PICC_ReadCardSerial()) {
            _MFRC522.PICC_DumpDetailsToSerial(&_MFRC522.uid);

            if (_MFRC522.uid.uidByte[0] == 214 && _MFRC522.uid.uidByte[1] == 173 && _MFRC522.uid.uidByte[2] == 167 && _MFRC522.uid.uidByte[3] == 197) {
                gpio_set_level((gpio_num_t)0, 0);
                gpio_set_level((gpio_num_t)2, 1);
                gpio_set_level((gpio_num_t)4, 0);
            }
            else {
                gpio_set_level((gpio_num_t)0, 1);
                gpio_set_level((gpio_num_t)2, 0);
                gpio_set_level((gpio_num_t)4, 0);
            }

            uint8_t block = 1;
            MFRC522::MIFARE_Key key;
            MFRC522::StatusCode status;
            uint8_t buffer[16];// = "Jose Silva";
            for (uint8_t i = 0; i < 6; i++) key.keyByte[i] = 0xFF;
            status = _MFRC522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(_MFRC522.uid));

            if (status != MFRC522::STATUS_OK) {
                ESP_LOGI(TAG, "PCD_Authenticate() failed: %s", _MFRC522.GetStatusCodeName(status));
            }
            else {
                //write
                // ESP_LOGI(TAG, "PCD_Authenticate() success!");
                //
                // status = _MFRC522.MIFARE_Write(block, buffer, 16);
                // if (status != MFRC522::STATUS_OK) {
                //     ESP_LOGI(TAG, "MIFARE_Write() failed: %s", _MFRC522.GetStatusCodeName(status));
                // }
                // else {
                //     ESP_LOGI(TAG, "MIFARE_Write() success");
                // }

                //read
                uint8_t len = 18;
                status = _MFRC522.MIFARE_Read(block, buffer, &len);
                if (status != MFRC522::STATUS_OK) {
                    ESP_LOGI(TAG, "Reading failed: %s", _MFRC522.GetStatusCodeName(status));
                }
                else {
                    ESP_LOGI(TAG, "Reading data: %s", buffer);
                }

                _MFRC522.PICC_HaltA();
                _MFRC522.PCD_StopCrypto1();
            }
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    _SPI.close();
}

extern "C" {
    void app_main()
    {
        ESP_LOGI(TAG, "Starting...");

        gpio_set_direction((gpio_num_t)0, GPIO_MODE_OUTPUT);
        gpio_set_direction((gpio_num_t)2, GPIO_MODE_OUTPUT);
        gpio_set_direction((gpio_num_t)4, GPIO_MODE_OUTPUT);

        gpio_set_level((gpio_num_t)0, 0);
        gpio_set_level((gpio_num_t)2, 0);
        gpio_set_level((gpio_num_t)4, 0);

        ESP_ERROR_CHECK(
            _SPI.init(5)
        );

        if (_MFRC522.PCD_PerformSelfTest()) {
            ESP_LOGI(TAG, "Self test OK");

            _MFRC522.PCD_Init();
            _MFRC522.PCD_DumpVersionToSerial();

            uint8_t antennaGain = _MFRC522.PCD_GetAntennaGain();

            ESP_LOGI(TAG, "Antenna gain: [%d].", antennaGain);

            xTaskCreate(searchNewCard, "searchNewCard", 5000, NULL, tskIDLE_PRIORITY, NULL);
        }
        else {
            ESP_LOGI(TAG, "Self test ERROR");
        }
    }
}
