#pragma once

#include <string>

namespace BiosSimpleMqttClient {

  class MQTTMessage {

    public:
      MQTTMessage(std::string topic, std::string message);

    public:
      std::string get_topic(void);
      std::string get_message(void);
      
    private:
      std::string topic;
      std::string message;
  };

};
