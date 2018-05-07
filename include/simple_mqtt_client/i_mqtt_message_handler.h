#pragma once

#include "mqtt_message.h"

namespace BiosSimpleMqttClient {

  class IMQTTMessageHandler {
    public:
      virtual void handle_mqtt_message(MQTTMessage mqttMessage) = 0;
  };

};
