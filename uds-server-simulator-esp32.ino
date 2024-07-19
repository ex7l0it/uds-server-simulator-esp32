

#include "src/globals.hpp"
#include "src/doip.hpp"
#include "src/doip_server.hpp"
#include "src/canuds.hpp"
#include <LittleFS.h>


void setup()
{
  randomSeed(analogRead(0));
  // Setup serial for debbuging.
  Serial.begin(115200);

  if (!LittleFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting LittleFS");
  }
  Serial.println("LittleFS mounted successfully");
  File file = LittleFS.open(CONFIG_JSON_FILE, "r");
  if (!file)
  {
    Serial.println("Please upload the config.json first.");
    return;
  }
  else
  {
    Serial.println("Initing...");
    cJSON *root = read_config();
    if (!root)
    {
      Serial.println("Error in read_config");
    }
    if (mode == 1)
    {
      doip_server_init();
    }

    uds_server_init(root, NULL);
  }

  // Set pins
  ESP32Can.setPins(CAN_TX, CAN_RX);

  // You can set custom size for the queues - those are default
  ESP32Can.setRxQueueSize(5);
  ESP32Can.setTxQueueSize(5);

  // .setSpeed() and .begin() functions require to use TwaiSpeed enum,
  // but you can easily convert it from numerical value using .convertSpeed()
  ESP32Can.setSpeed(ESP32Can.convertSpeed(500));

  // You can also just use .begin()..
  if (ESP32Can.begin())
  {
    Serial.println("CAN bus started!");
  }
  else
  {
    Serial.println("CAN bus failed!");
  }

  // or override everything in one command;
  // It is also safe to use .begin() without .end() as it calls it internally
  if (ESP32Can.begin(ESP32Can.convertSpeed(500), CAN_TX, CAN_RX, 10, 10))
  {
    Serial.println("CAN bus started!");
  }
  else
  {
    Serial.println("CAN bus failed!");
  }
}

cJSON *read_config()
{
  File file = LittleFS.open(CONFIG_JSON_FILE, "r");
  if (file == NULL)
  {
    Serial.println("## OPEN CONFIG FAIL ##");
    return NULL;
  }
  int fileSize = file.size();
  char *jsonStr = (char *)malloc(sizeof(char) * fileSize + 1);
  memset(jsonStr, 0, fileSize + 1);
  file.readBytes(jsonStr, fileSize);
  jsonStr[fileSize] = '\0';
  file.close();

  cJSON *root = cJSON_Parse(jsonStr);
  if (!root)
  {
    const char *err = cJSON_GetErrorPtr();
    Serial.printf("Error before: [%s]\n", err);
    free((void *)err);
    free(jsonStr);
    return NULL;
  }
  free(jsonStr);
  return root;
}

void loop()
{
  static uint32_t lastStamp = 0;
  uint32_t currentStamp = millis();

  if (random_frame && currentStamp - lastStamp > 1000)
  {
    // Generate random CAN data per second
    lastStamp = currentStamp;
    sendRandomFrame();
    uint8_t buffer[256] = {0};
  }

  // You can set custom timeout, default is 1000
  if (ESP32Can.readFrame(rxFrame, 1000))
  {
    // Comment out if too many requests
    can_uds::handle_pkt(rxFrame);
  }

  // if open DoIP mode
  if (mode == 1)
  {
    // udp
    int udp_len = udp.parsePacket();
    if (udp_len)
    {
      Serial.printf("Get UDP data from %s:%d\n", udp.remoteIP().toString().c_str(), udp.remotePort());
      u_int8_t buffer[256] = {0};
      udp.read(buffer, udp_len);
      DoIPFrame frame = DoIPFrame(nullptr, &udp, buffer, udp_len);
      Serial.printf("[*] Request Frame:\n");
      frame.debug_print();

      handle_doip_frame(&frame, nullptr, &udp, UDP_CLIENT);
      
      // drop data
      udp.flush();
    }

    // tcp
    WiFiClient client = server.available();
    if (client)
    {
      Serial.println("New Client.");
      WiFiClient *newClient = new WiFiClient(client);
      xTaskCreate(handle_client, "handle_client", 4096, newClient, 1, NULL);
    }
  }
}


void handle_client(void *parameter)
{
  WiFiClient client = *((WiFiClient *)parameter);
  free(parameter);

  while (client.connected())
  {
    if (client.available())
    {
      Serial.printf("GET TCP DATA FROM %s:%d\n", client.remoteIP().toString().c_str(), client.remotePort());
      u_int8_t buffer[256] = {0};
      int len = client.read(buffer, 256);
      DoIPFrame frame = DoIPFrame(&client, nullptr, buffer, len);
      Serial.printf("[*] Request Frame:\n");
      frame.debug_print();

      handle_doip_frame(&frame, &client, nullptr, TCP_CLIENT);
    }
  }
  vTaskDelete(NULL);
}
