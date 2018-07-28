# Arduino-Aqua
----
Smart aquarium LED controller using arduino

#arduino #nodemcu #wifi #aquarium #led #WeMos

You can manage some devices via web-page. You can configure timetable, swith on/off, configure values for PWM chanels.

#### Supported devices from box:
* Boards:
  - NodeMcu v3
  - WeMos D1
  - etc (based on ESP8266)
* RTC:
  - DS3231
  - DS3207
  - DS1302


### Installation
1.	Clone the repo
2.	Choose scetch file ([WiFiWebServer.ino](https://github.com/mav10/Arduino-Aqua/blob/master/src/WiFiWebServer/WiFiWebServer.ino "WiFiWebServer.ino") or [WiFiWebServer_DS1302.ino](https://github.com/mav10/Arduino-Aqua/blob/master/src/WiFiWebServer_DS1302/WiFiWebServer_DS1302.ino "WiFiWebServer_DS1302.ino"))
3.	Upload/attach libs (All lib are included in folder Lib) or can be download via *ArduinoMarketPlace*. See comments in scetch)
4.	Re-configure pins according to your board connecction and platform
5.	Upload code in ardiono
6.	See com-port for recognize where your app will be hosted (ip-address)
7.	Connect to it via browser
8.	**Use it! Be Happy!**

# Example
![Web page example](/docs/example.png)

Or you can even click buttosk on static HTML [here](https://mav10.github.io/Arduino-Aqua/)

License
----

MIT


**Free Software, Hell Yeah!**
