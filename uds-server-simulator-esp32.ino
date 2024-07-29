#include "src/globals.hpp"
#include "src/doip.hpp"
#include "src/canuds.hpp"
#include "src/dashboard.hpp"
#include <LittleFS.h>

void setup()
{
  // check GPIO18 is high or low
  gpio_get_level(GPIO_NUM_18) ? mode = RUN_MODE_DOIP : mode = RUN_MODE_CAN;
  // check GPIO21 is high or low
  if (mode == RUN_MODE_CAN)
  {
    mode = gpio_get_level(GPIO_NUM_21) ? mode = RUN_MODE_CAN_DASHBOARD : mode = RUN_MODE_CAN;
  }

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
    uds_server_init(root, NULL);
  }

  // show current mode
  switch (mode)
  {
  case RUN_MODE_CAN:
    Serial.println("[+] CAN Mode");
    can_init();
    break;
  case RUN_MODE_DOIP:
    Serial.println("[+] DOIP Mode");
    doip_server_init();
    break;
  case RUN_MODE_CAN_DASHBOARD:
    Serial.println("[+] CAN Mode with Dashboard");
    can_init();
    dashboard_init();
    break;
  default:
    Serial.println("[+] Unknown Mode");
    break;
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
  if (mode == RUN_MODE_CAN)
  {
    static uint32_t lastStamp = 0;
    uint32_t currentStamp = millis();

    if (currentStamp - lastStamp > 1000)
    {
      lastStamp = currentStamp;
      if (random_frame)
      {
        // Generate random CAN data per second
        lastStamp = currentStamp;
        sendRandomFrame();
        uint8_t buffer[256] = {0};
      }
    }

    // You can set custom timeout, default is 1000
    if (ESP32Can.readFrame(rxFrame, 1000))
    {
      // Comment out if too many requests
      can_uds::handle_pkt(rxFrame);
    }
  }
  // if open DoIP mode
  else if (mode == RUN_MODE_DOIP)
  {
    // if new client connect to the AP
    if (WiFiAP.softAPgetStationNum() > 0 && !is_announced)
    {
      // announcement
      doip_identification_announcement(&udp);
    }

    // udp
    int udp_len = udp.parsePacket();
    if (udp_len)
    {
      // Serial.printf("Get UDP data from %s:%d\n", udp.remoteIP().toString().c_str(), udp.remotePort());
      u_int8_t buffer[256] = {0};
      udp.read(buffer, udp_len);
      DoIPFrame frame = DoIPFrame(nullptr, &udp, buffer, udp_len);

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
  else if (mode == RUN_MODE_CAN_DASHBOARD)
  {
    dash_server.handleClient();
    // You can set custom timeout, default is 1000
    if (ESP32Can.readFrame(rxFrame, 1000))
    {
      // Comment out if too many requests
      can_uds::handle_pkt(rxFrame);
    }
  }
}

void handle_client(void *parameter)
{
  WiFiClient client = *((WiFiClient *)parameter);
  free(parameter);

  if (mode == RUN_MODE_DOIP)
  {
    while (client.connected())
    {
      if (client.available())
      {
        // Serial.printf("GET TCP DATA FROM %s:%d\n", client.remoteIP().toString().c_str(), client.remotePort());
        u_int8_t buffer[256] = {0};
        int len = client.read(buffer, 256);
        DoIPFrame frame = DoIPFrame(&client, nullptr, buffer, len);

        handle_doip_frame(&frame, &client, nullptr, TCP_CLIENT);
      }
    }
  }

  vTaskDelete(NULL);
}
