# OASYS

General info

- MQTT broker: "broker.hivemq.com"
- MQTT publish topic: "Oasys/Publish"
- MQTT Subscribe topic: "Oasys/Subscribe"
- SSID: OASYS
- Password: Oasyspooltest  (Use shared wifi hotspot from pc or phone)

 For communication with the ESP8266, use a mqtt client such as mosquitto (https://mosquitto.org/)

 Glider Commands:
 
 Timer feedback
 
  Example: MQTT Publish --> "600427". Esp8266 recives [1][0][0][0][4][2][7] as a string 
  
  - First three digits "100" sets the diving delay. This value is multiplied by 1000, so "6" is a delay of 6 seconds, "9" is 90 seconds    etc
  - NB: For round numbers (like 10, 50, 120, 140) the "0" needs to be replaced with an small x. Example: For a 20 second delay, send "2X"
  
   Diving delay: 20 seconds
   Pumping from internal tank to external bladder: 27 seconds
   Pumping from external bladder to internal tank: 4 seconds
   
   Send "2x00427"
   
  - The second and third digit are also used for depth sensor feedback (see below)
  - Fourth and fifth digit "04" sets the pumping interval from the internal bladder to external bladder "04" = 4 seconds (Glider rises)
  - Digit six and seven "27" (27 seconds) sets the pumping interval from external bladder to internal bladder (glider dives)
  
  Depth Sensor feedback
   Example: MQTT Publish ---> "2.20427"
   - First three digits sets the maximum depth limit in meters. "2.2" = 2.2 meters
   - The rest of the digits is the same as the timer feedback. 
