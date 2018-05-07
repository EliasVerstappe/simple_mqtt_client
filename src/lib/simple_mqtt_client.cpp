#include "simple_mqtt_client/simple_mqtt_client.h"

#define MQTT_LOGGING

#ifdef MQTT_LOGGING
#include <iostream>
#endif

namespace BiosSimpleMqttClient {

  SimpleMQTTClient::SimpleMQTTClient(std::string brokerAddress, std::string clientId) {
    isConnected = false;
    numberOfConnectionRetries = 0;

    connectionOptions.set_keep_alive_interval(20);
    connectionOptions.set_clean_session(true);

    client = new mqtt::async_client(brokerAddress, clientId);
    client->set_callback(*this);

    subscriptionTopic = "";
    messageHandler = nullptr;
    connect();
  }

  SimpleMQTTClient::~SimpleMQTTClient(void) {
    disconnect();
    delete client;
  }

  void SimpleMQTTClient::subscribe(std::string topic, IMQTTMessageHandler * messageHandler) {
    this->subscriptionTopic = topic;
    this->messageHandler = messageHandler;

    if (isConnected) {
      client->subscribe(subscriptionTopic, QOS, (void*)(&subscribeContext), *this);
    } else {
#ifdef MQTT_LOGGING
      std::cout << "Cannot subscribe to " << subscriptionTopic << " - client not connected to broker. Will try again on connected" << std::endl;
#endif
    }
  }

  void SimpleMQTTClient::publish(MQTTMessage message) {
    if (!isConnected) {
#ifdef MQTT_LOGGING
      std::cout << "Cannot publish - not connected to broker" << std::endl;
#endif
      return;
    }

    mqtt::message_ptr pubmsg = mqtt::make_message(message.get_topic(), message.get_message());
    pubmsg->set_qos(QOS);

    try {
      mqtt::delivery_token_ptr pubtok = client->publish(pubmsg, (void*)(&publishContext), *this);
    	if (!pubtok->wait_for(std::chrono::seconds(TIMEOUT_SECONDS))) {
#ifdef MQTT_LOGGING
        std::cout << "Publish not completed within timeout" << std::endl;
#endif
      }
  	}
  	catch (const mqtt::exception& exc) {
#ifdef MQTT_LOGGING
      std::cout << "Failed to publish mqtt message " << std::string(exc.what()) << std::endl;
#endif
  	}
  }

  void SimpleMQTTClient::connect(void) {
  	try {
#ifdef MQTT_LOGGING
      std::cout << "Trying to connect to MQTT broker" << std::endl;
#endif
  		client->connect(connectionOptions, (void*)(&connectionContext), *this);
  	}
  	catch (const mqtt::exception& exc) {
#ifdef MQTT_LOGGING
      std::cout << "Connect failed with " << std::string(exc.what()) << std::endl;
#endif
  	}
  }

  void SimpleMQTTClient::reconnect() {
  	std::this_thread::sleep_for(std::chrono::milliseconds(2500));
#ifdef MQTT_LOGGING
    std::cout << "Reconnecting to MQTT broker" << std::endl;
#endif
  	connect();
  }

  void SimpleMQTTClient::disconnect(void) {
  	// Double check that there are no pending tokens
  	auto toks = client->get_pending_delivery_tokens();
#ifdef MQTT_LOGGING
  	if (!toks.empty()) {
      std::cout << "There are pending delivery tokens" << std::endl;
    }
#endif

  	try {
#ifdef MQTT_LOGGING
      std::cout << "Disconnecting from the MQTT broker ..." << std::endl;
#endif
  		client->disconnect()->wait();
#ifdef MQTT_LOGGING
      std::cout << "Disconnected from the MQTT broker" << std::endl;
#endif
      isConnected = false;
  	}
  	catch (const mqtt::exception& exc) {
#ifdef MQTT_LOGGING
      std::cout << "Disconnect failed with " << std::string(exc.what()) << std::endl;
#endif
  	}
  }

  void SimpleMQTTClient::on_failure(const mqtt::token& tok) {
    if (tok.get_user_context() == &connectionContext) {
#ifdef MQTT_LOGGING
      std::cout << "Connection attempt to MQTT broker failed" << std::endl;
#endif
      isConnected = false;
    	if (++numberOfConnectionRetries > N_RETRY_ATTEMPTS) {
    		return;
      }
    	reconnect();
    } else if (tok.get_user_context() == &subscribeContext) {
#ifdef MQTT_LOGGING
      std::cout << "Subscription failed for topic " << subscriptionTopic << std::endl;
#endif
    } else if (tok.get_user_context() == &publishContext) {
  		auto top = tok.get_topics();
  		if (top && !top->empty()) {
#ifdef MQTT_LOGGING
      std::cout << "Publish failed for topic" << (*top)[0] << std::endl;
#endif
      } else {
#ifdef MQTT_LOGGING
      std::cout << "Publish failed" << std::endl;
#endif
      }
    }
  }

  void SimpleMQTTClient::on_success(const mqtt::token& tok) {
    // We can't use connectionContext here. For some reason this callback
    // is activated twice for a single connection that is made. Luckely there is
    // the connected() callback that works just fine.
    if (tok.get_user_context() == &subscribeContext) {
#ifdef MQTT_LOGGING
      std::cout << "Subscription success for topic " << subscriptionTopic << std::endl;
#endif
    } else if (tok.get_user_context() == &publishContext) {
  		auto top = tok.get_topics();
  		if (top && !top->empty()) {
#ifdef MQTT_LOGGING
      std::cout << "Publish successfull for topic " << (*top)[0] << std::endl;
#endif
      } else {
#ifdef MQTT_LOGGING
      std::cout << "Publish successfull" << std::endl;
#endif
      }
    }
  }

  void SimpleMQTTClient::connected(const std::string& cause) {
#ifdef MQTT_LOGGING
      std::cout << "Connection successfully made to MQTT broker" << std::endl;
#endif
    isConnected = true;
    if (messageHandler) {
      subscribe(subscriptionTopic, messageHandler);
    }
  }

  void SimpleMQTTClient::connection_lost(const std::string& cause) {
    std::cout << "Connection to MQTT broker lost" << std::endl;
    isConnected = false;
    if (!cause.empty()) {
#ifdef MQTT_LOGGING
      std::cout << "\tcause: " << cause << std::endl;
#endif
    }

#ifdef MQTT_LOGGING
    std::cout << "Trying to reconnect to MQTT broker ..." << std::endl;
#endif
    numberOfConnectionRetries = 0;
    reconnect();
  }

  void SimpleMQTTClient::message_arrived(mqtt::const_message_ptr msg) {
    MQTTMessage message(msg->get_topic(), msg->to_string());
    messageHandler->handle_mqtt_message(message);
  }

  void SimpleMQTTClient::delivery_complete(mqtt::delivery_token_ptr token) { }

};
