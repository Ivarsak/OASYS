# OASYS

Requirements

- MQTT broker: "broker.hivemq.com"
- MQTT publish topic: "Oasys/Publish"
- MQTT Subscribe topic: "Oasys/Subscribe"

 For communication with the ESP8266, use a mqtt client such as mosquitto (https://mosquitto.org/)

 Glider Commands:
 
 Timer feedback:
 
  "1000427"
  - First digit "1" sets the diving delay. This value is multiplied by ten, so "1" is a delay of 10 seconds, "2" is 20 seconds etc
  - The second and third digit are used for depth sensor feedback (see below)
  - Fourth and fifth digit "04" sets the pumping interval from the internal bladder to external bladder "04" = 4 seconds (Glider rises)
  - Digit six and seven "27" (27 seconds) sets the pumping interval from external bladder to internal bladder (glider dives)
