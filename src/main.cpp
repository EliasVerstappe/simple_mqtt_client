#include <iostream>
#include <string>
#include <iostream>
#include <unistd.h>
#include "simple_mqtt_client/simple_mqtt_client.h"

using namespace BiosSimpleMqttClient;
using namespace std;

class SomeMessageHandler : public IMQTTMessageHandler {
  public:
    void handle_mqtt_message(MQTTMessage mqttMessage) override {
      std::cout << "Received message on topic '"
        << mqttMessage.get_topic() << "' with payload: "
        << mqttMessage.get_message() << std::endl;

      if (mqttMessage.get_topic() == "oop3/cat/question") {
        cout << "Received question: " << mqttMessage.get_message() << endl;
      } else if (mqttMessage.get_topic() == "oop3/cat/winner") {
        cout << "And the winner is: " << mqttMessage.get_message() << endl;
      }
    }
};

const std::string SERVER_ADDRESS("tcp://mqtt.labict.be:1883");
const std::string CLIENT_ID("sdasdas453953450439534v5");
const std::string TOPIC("oop3/cat");

int main(int argc, char* argv[])
{
  cout << "Sending hello via MQTT" << endl;

	SimpleMQTTClient simpleClient(SERVER_ADDRESS, CLIENT_ID);
  SomeMessageHandler messageHandler;
  
  // SimpleMQTTClient can only subscribe to single topic. However
  // you can subscribe using wilcard to a hierarchy of topics
  simpleClient.subscribe(TOPIC+"/#", &messageHandler);

  // Send a message
  sleep(2);   // Wait a bit for connection
  MQTTMessage message(TOPIC, "Hello @ ALL");
  simpleClient.publish(message);

	// Just block till user tells us to quit.
  std::cout << "Press Q to quit" << std::endl;
	while (std::tolower(std::cin.get()) != 'q');

 	return 0;
}
