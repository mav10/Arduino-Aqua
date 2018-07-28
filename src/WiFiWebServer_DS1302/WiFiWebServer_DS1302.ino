/*
 *  This sketch manages of devices for aquarium automatisation.
 *  The server will set a GPIO pins depending on the request
 *  The Server is available by
 *    http://server_ip/
 *   and has got a  static html page via you can swith on/off some pins and watch some information.
 *   all requests (most part of them is performed via POST requests)
 *  Avaliable commands and pereferyies:
 *   - 4 LED controller:
 *   - - Time schedule execution for every channel
 *   - - - P.S. this pins assigned as PWM.
 *  - GPIO 5:
 *  - - You can only swith on/off devices which connected to this pins
 *  - GPIO 7:
 *  - - This is also swith on/off function, but it works only during the configured time
 *  - GPIO 8:
 *  - - It's a PWM pinout, and you can configure value by the slider.
 *  - Also you can watch Arduino current time (from RTC module) and re-configure them.
 *  - As we have got static page with some arrays and strings etc. so is needed to clear cashe to release RAM.
 *  server_ip is the IP address of the ESP8266 module, will be 
 *  printed to Serial when the module is connected.
 *  
 *  Sorting was downloaded here https://github.com/ivanseidel/LinkedList
 *  DS1302 waas downloaded here https://github.com/msparks/arduino-ds1302
 *  Rtc lib could be downloaded via Arduino MarketPlace "RTClib by Adafruit"
 *  
 *  Actualy, all libs there are in folder and you can include them via library manager (.zip setupper)
 *  This scetch uses and is configured to RTC DS1302. So we use liblary for that module. And also configured pins out for rela time clock
*/
#include <LinkedList.h>
#include <Wire.h>
#include <DS1302.h>
#include <RTClib.h>
#include <ESP8266WiFi.h>
#include <String.h>

#define bufferMax 128
int bufferSize;
char buffer[bufferMax];
char post;
String readString = String(128);
String logString = "";

enum logs_state {NORMAL, SUCCESS, WARNING, DANGER};
enum sensor_name {LED_1, LED_2, LED_3, LED_4, GD5, GD7, GD8};

const char* ssid = "WiFi-DOM.ru-2463"; 
const char* password = "89502657277"; 

/*
 * Set the appropriate digital I/O pin connections. These are the pin
 * assignments for the Arduino as well for as the DS1302 chip. See the DS1302
 * datasheet:  http://datasheets.maximintegrated.com/en/ds/DS1302.pdf
*/
const int kCePin   = D0;  // Chip Enable
const int kIoPin   = D1;  // Input/Output
const int kSclkPin = D2;  // Serial Clock

DS1302 rtc(kCePin, kIoPin, kSclkPin);

// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(80);

typedef struct {
  String startTime;
  String finishTime;
} TimeExecution;

typedef struct {
  Time startTime();
  Time finishTime();
} TimeExec;

typedef struct {
  sensor_name sensorName;
  int value;
  int pin;
  bool isPwm;
} State;

typedef struct {
  String timeExecute;
  int channel1;
  int channel2;
  int channel3;
  int channel4;
} LedSchedule;

State sensors[8] = {
   {LED_1, 0, D5, true},
   {LED_2, 0, D6, true},
   {LED_3, 0, D7, true},
   {LED_4, 0, D8, true},
   {GD5, 0, D4, false},
   {GD7, 0, 9, false},
   {GD8, 0, 10, true}
};

TimeExecution GD7TimeTable;
LinkedList<LedSchedule> *timetable = new LinkedList<LedSchedule>();

void setup() {
  Serial.begin(115200);
  delay(10);
  Serial.println();
  Serial.println("start configuring");

  InitializeLeds();
  delay(10);
  
  ConfigureClock();
  ConfigureGpio();
  ConfigureWiFi();
}

void loop() {
  ApplyCurrentState();
  delay(1);
  // Send the response to the client
  getPostRequest();
  delay(1);
  DoSchedule();
}

String GetCurrentTime(){
   // Get the current time and date from the chip.
  Time t = rtc.time();
  String parsedMin = String();
  String parsedHour = String();
  parsedHour = (t.hr < 10) ? "0" + String(t.hr, DEC) : String(t.hr, DEC);
  parsedMin = (t.min < 10) ? "0" + String(t.min, DEC) : String(t.min, DEC);
  return parsedHour + ":" + parsedMin;
}

void LOG(String text, logs_state status){
   String color[4] = {"black", "green", "yellow", "red"};
  logString += "<p style='color:" + color[status] +"' >"  + "[" + GetCurrentTime()+ "]: " + text + "</p>";

  Serial.println("LOG:[" + GetCurrentTime() + "]: " + text);
}

int Compare(LedSchedule& a, LedSchedule& b) {
  if(a.timeExecute > b.timeExecute){
    return 1;
  }
  return -1;
}

void ConfigureClock(){
  // Initialize a new chip by turning off write protection and clearing the
  // clock halt flag. These methods needn't always be called. See the DS1302
  // datasheet for details.
  rtc.writeProtect(false);
  rtc.halt(false);

  // Make a new time object to set the date and time.
  // Sunday, July 21, 2018 at 22:59:00.
  Time t(2018, 7, 21, 22, 59, 0, Time::kSunday);

  // Set the time and date on the chip.
  rtc.time(t);
  CheckSystemTime();
}

void CheckSystemTime(){
  Time t = rtc.time();
  String day = dayAsString(t.day);
  LOG("Chech the time after configuring", WARNING);  
 
  LOG("system time: "
    + String(day.c_str())
    + "/"
    + String(t.yr, DEC)
    + "/"
    + String(t.mon, DEC)
    + "/"
    + String(t.date, DEC)
    + " "
    + String(t.hr, DEC)
    + ":"
    + String(t.min, DEC)
    + ":"
    + String(t.sec)
    , NORMAL);
  LOG("programm time: " + GetCurrentTime(), NORMAL);

  if(GetCurrentTime().length() != 5){
    LOG("Programm time format has been wrong!"
        + GetCurrentTime()
        + " Check the RTC module"
       , DANGER);
  }
}

String dayAsString(const Time::Day day) {
  switch (day) {
    case Time::kSunday: return "Sunday";
    case Time::kMonday: return "Monday";
    case Time::kTuesday: return "Tuesday";
    case Time::kWednesday: return "Wednesday";
    case Time::kThursday: return "Thursday";
    case Time::kFriday: return "Friday";
    case Time::kSaturday: return "Saturday";
  }
  return "(unknown day)";
}

void ConfigureWiFi(){
  LOG("Connecting to ", NORMAL);
  LOG(ssid, NORMAL);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  LOG("WiFi connected", SUCCESS);
  
  // Start the server
  server.begin();
  LOG("Server started", SUCCESS);
  LOG(WiFi.localIP().toString(), NORMAL);
}

void ConfigureGpio(){
  LOG("Configure GPIO", WARNING);
  ApplyCurrentState();
}

void InitializeLeds(){
  timetable->add({"08:00", 100, 100, 100, 100}); 
  timetable->add({"09:00", 200, 200, 200, 200}); 
}

void getPostRequest() {
  WiFiClient client = server.available();
  
  if (client) {
    Serial.println("Client connected");
    boolean currentLineIsBlank = true;
    bufferSize = 0;
    while (client.connected()) {
      if(client.available()){
        char c = client.read();
        // if you got the newLine symbol
        // and empty string symbol, the POST-request has been finished
        // so you can send response
        if (c == '\n' && currentLineIsBlank) {
          // DATA from POST request
          while(client.available()) {  
            post = client.read();   
            if(bufferSize < bufferMax)
              buffer[bufferSize++] = post;  // add new symbol to the buffer
          }

          client.flush();
          Serial.println("Received POST request:");
          Serial.println(buffer);

          //Do some commands
          PerformRequestedCommands();

          //Send request with new state
          client.print(GetPage());
          client.stop();
        } 
        else if (c == '\n') {
          currentLineIsBlank = true;
        } 
        else if (c != '\r') {
          currentLineIsBlank = false;
        }
      }
    }
    memset(buffer, 0, sizeof(buffer)/sizeof(char));
    readString = "";
  }
}

void UpdatePinValue(sensor_name sensorName, int value) {
  if(sensors[sensorName].sensorName == sensorName){
    int oldValue = sensors[sensorName].value;
    if(oldValue != value){
      sensors[sensorName].value = value;
      LOG(String(sensorName) + " sensor was updated from " + String(oldValue) + " to " + String(value), SUCCESS);
    }
  }else{
    LOG(String(sensorName) + " sensor was not updated. There are name conflict", DANGER);
  }
}

void ApplyCurrentState(){
  int sizeArray = sizeof(sensors)/sizeof(State);
  for(int i = 0; i < sizeArray; i++){
    if(sensors[i].isPwm) {
      analogWrite(sensors[i].pin, sensors[i].value);
    } else {
      pinMode(sensors[i].pin, OUTPUT);
      digitalWrite(sensors[i].pin, sensors[i].value);
    }
  }
}

void AssignCurrentLedValueFromTimeTable(int timeTableEventIndex){
  int nextToIndex = (timeTableEventIndex < timetable->size() - 1) ? timeTableEventIndex + 1 : 0;
  String currentTime = GetCurrentTime();
  int timeFrom = TimeToMinutes(timetable->get(timeTableEventIndex).timeExecute);
  int timeTo = TimeToMinutes(timetable->get(nextToIndex).timeExecute);
  
  int curTime = TimeToMinutes(currentTime);
  
  int val1 = map(curTime, timeFrom, timeTo, timetable->get(timeTableEventIndex).channel1, timetable->get(nextToIndex).channel1);
  int val2 = map(curTime, timeFrom, timeTo, timetable->get(timeTableEventIndex).channel2, timetable->get(nextToIndex).channel2);
  int val3 = map(curTime, timeFrom, timeTo, timetable->get(timeTableEventIndex).channel3, timetable->get(nextToIndex).channel3);
  int val4 = map(curTime, timeFrom, timeTo, timetable->get(timeTableEventIndex).channel4, timetable->get(nextToIndex).channel4);

  UpdatePinValue(LED_1, val1);
  UpdatePinValue(LED_2, val2);
  UpdatePinValue(LED_3, val3);
  UpdatePinValue(LED_4, val4);
}

int TimeToMinutes(String incomingTime){
  int separatorIndex = incomingTime.indexOf(":");
  int hours = incomingTime.substring(0, separatorIndex).toInt();
  int minutes = incomingTime.substring(separatorIndex + 1).toInt();

  if((hours > 24 || hours < 0) ||(minutes > 59 || minutes < 0)){
    LOG("Something is wrong with time. Hours couldn't be more then 24 or negative. Minutes couldn't be more then 59 or negative. Please check your RTC module", DANGER);
  }
  
  return (hours * 60) + minutes;
}

void PerformRequestedCommands() {
  readString = buffer;
  if(readString.indexOf("GD5") != -1) { 
    int value = GetValueFromHtmlForm("GD5", readString).toInt();
    UpdatePinValue(GD5, value);              
  } else if(readString.indexOf("GD8") != -1) {
    int value = GetValueFromHtmlForm("GD8", readString).toInt();
    UpdatePinValue(GD8, value);
  }else if(readString.indexOf("GD7") != -1) {
    SaveGD7TimeSchedule(readString);
  }else if(readString.indexOf("SystemTime") != -1) {
    SetupTime(readString);
  }else if(readString.indexOf("clearAll") != -1) {
    ClearCache();
  }else if(readString.indexOf("LED") != -1) {
    PerformNewLedEvent(readString);
  }else if (readString.indexOf("clearTimeTable") != -1){
    timetable->clear();
  }else {
    Serial.println("empty request");
  }
}

void ClearCache() {
  post = ' ';
  logString = "";
  readString = "";
  memset(buffer, 0, sizeof(buffer)/sizeof(char));
}

void PerformNewLedEvent(String requestBody) {
  int chanel1 = GetValueFromHtmlForm("LEDchanel1", requestBody).toInt();
  int chanel2 = GetValueFromHtmlForm("LEDchanel2", requestBody).toInt();
  int chanel3 = GetValueFromHtmlForm("LEDchanel3", requestBody).toInt();
  int chanel4 = GetValueFromHtmlForm("LEDchanel4", requestBody).toInt();

  String timeSetup = ParseTime(requestBody, "LEDtime", "&LEDchanel1"); 

  timetable->add({timeSetup, chanel1, chanel2, chanel3, chanel4});
  LOG("Added new event for LED. Execition time starts at "
    + timeSetup
    + " ch1: " + chanel1
    + " ch2: " + chanel2
    + " ch3: " + chanel3
    + " ch4: " + chanel4, SUCCESS);
}

String ParseTime(String requestBody, String patternStart, String patternEnd){
  String parsedTime = GetValueFromHtmlForm(patternStart, requestBody);
  if(patternEnd != "")
    parsedTime = parsedTime.substring(0, parsedTime.indexOf(patternEnd));
  
  parsedTime.replace("%3A", ":");
  return parsedTime;
}

void SaveGD7TimeSchedule(String requestBody){
  String startTime = GetValueFromHtmlForm("timeStart", requestBody);
  startTime = startTime.substring(0, startTime.indexOf("&GD7timeEnd"));
  String endTime = GetValueFromHtmlForm("timeEnd", requestBody);

  startTime.replace("%3A", ":");
  endTime.replace("%3A", ":");
  GD7TimeTable = {startTime, endTime};

  LOG("Added new event for timeDepends GPIO. Time of execution "
  + startTime
  + " - "
  + endTime, SUCCESS);
}

void SetupTime(String requestBody){
  String newTime = ParseTime(requestBody, "SystemTime", String());
   LOG("time parser: "
        + newTime, NORMAL);
  newTime.replace("%3A", ":");
  int index = newTime.indexOf(":");
  int hoursNumber =  newTime.substring(0, index).toInt();
  int minNumber = newTime.substring(index + 1).toInt();
  /* TODO: 
   * right now as you can see data (21th July 2018 is mocked)
   * later will be configurable from UI part too.
   */
  LOG("Received time params Hours: "
        + String(hoursNumber)
        + ", Minutes: "
        + String(minNumber), SUCCESS);

  // Sunday, July 21, 2013 at HH:mm:00.
  Time t(2018, 7, 21, hoursNumber, minNumber, 0, Time::kSunday);
  rtc.time(t);

  LOG("Time was reconfigured", NORMAL);
  CheckSystemTime();
}


String GetValueFromHtmlForm(String gpioName, String requestBody){
  int startIndex = requestBody.indexOf(gpioName + "=");
    return requestBody.substring(startIndex + gpioName.length() + 1);
}

String WriteLedTable(){
  String currentTime = GetCurrentTime();
  timetable->sort(Compare);
  int leng = timetable->size();
  String row = "";
  for(int i = 0; i < leng - 1; i++) {
    if ((String(timetable->get(i).timeExecute) <= String(currentTime)) && (String(currentTime) < String(timetable->get(i+1).timeExecute)))
      row += "              <tr style='color:green font-style:italic'>";
    else
      row += "              <tr>";
      row += "                <td>";
      row +=                    timetable->get(i).timeExecute;
      row +=                  "</td>";
      row += "                <td>";
      row +=                    timetable->get(i).channel1;
      row +=                  "</td>";
      row += "                <td>";
      row +=                    timetable->get(i).channel2;
      row +=                  "</td>";
      row += "                <td>";
      row +=                    timetable->get(i).channel3;
      row +=                  "</td>";
      row += "                <td>";
      row +=                    timetable->get(i).channel4;
      row +=                  "</td>";
      row += "              </tr>";
  }
      row += "              <tr>";
      row += "                <td>";
      row +=                    timetable->get(leng - 1).timeExecute;
      row +=                  "</td>";
      row += "                <td>";
      row +=                    timetable->get(leng - 1).channel1;
      row +=                  "</td>";
      row += "                <td>";
      row +=                    timetable->get(leng - 1).channel2;
      row +=                  "</td>";
      row += "                <td>";
      row +=                    timetable->get(leng - 1).channel3;
      row +=                  "</td>";
      row += "                <td>";
      row +=                    timetable->get(leng - 1).channel4;
      row +=                  "</td>";
      row += "              </tr>";
  return row;
}

void DoSchedule(){
  String currentTime = GetCurrentTime();
  timetable->sort(Compare);
  int leng = timetable->size();
  for(int i = 0; i < leng - 1; i++){
    if ((String(timetable->get(i).timeExecute) <= String(currentTime)) && (String(currentTime) < String(timetable->get(i+1).timeExecute))) {
      Serial.println("execute Event ["
                    + String(i)
                    + "] time: "
                    + currentTime
                    + " event is beeing executed from "
                    + timetable->get(i).timeExecute
                    + " to "
                    + timetable->get(i+1).timeExecute);
      AssignCurrentLedValueFromTimeTable(i);
    }
  }
  if ((String(currentTime) > String(timetable->get(leng -1).timeExecute)) && (String(timetable->get(0).timeExecute) >= String(currentTime))){
    //TODO: we have to repeat loop again
     AssignCurrentLedValueFromTimeTable(leng -1);
  }

  bool MoreThenStart = (String(GD7TimeTable.startTime) <= String(currentTime));
  bool LessThenFinish = (String(GD7TimeTable.finishTime) >= String(currentTime));
  if (MoreThenStart && LessThenFinish)
  {
    sensors[GD7].value = 1;
  }else{
    sensors[GD7].value = 0;
  }
}

String GetPage(){
  String page = "";
  page += "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n";
  page += "<html charset=UTF-8>";
  page += "<head>";
  page += "  <meta http-equiv='Content-Type' content='text/html, charset=utf-8'/> ";
  page += "  <meta name='viewport' content='width=device-width, initial-scale=0.8'>";
  page += "  <script src='https://code.jquery.com/jquery-3.2.1.slim.min.js' integrity='sha384-KJ3o2DKtIkvYIK3UENzmM7KCkRr/rE9/Qpg6aAZGJwFDMVNA/GpGFF93hXpG5KkN' crossorigin='anonymous'></script>";
  page += "  <script src='https://cdnjs.cloudflare.com/ajax/libs/popper.js/1.12.9/umd/popper.min.js' integrity='sha384-ApNbgh9B+Y1QKtv3Rn7W3mgPxhU9K/ScQsAP7hUibX39j7fakFPskvXusvfa0b4Q' crossorigin='anonymous'></script>";
  page += "  <script src='https://maxcdn.bootstrapcdn.com/bootstrap/4.0.0/js/bootstrap.min.js' integrity='sha384-JZR6Spejh4U02d8jOt6vLEHfe/JQGiRRSQQxSfFWpi1MquVdAyjUar5+76PVCmYl' crossorigin='anonymous'></script>";
  page += "  <script src='https://code.highcharts.com/highcharts.js'></script>";
  page += "  <script src='https://code.highcharts.com/modules/data.js'></script>";
  page += "  <script src='https://code.highcharts.com/modules/exporting.js'></script>";
  page += "  <script src='https://code.highcharts.com/modules/export-data.js'></script>";
  page += "  <script src='https://cdnjs.cloudflare.com/ajax/libs/bootstrap-slider/10.0.2/bootstrap-slider.min.js'></script>";
  page += "  <script src='https://cdnjs.cloudflare.com/ajax/libs/bootstrap-slider/10.0.2/bootstrap-slider.js'></script>";
  page += "  <link rel='stylesheet' href='https://cdnjs.cloudflare.com/ajax/libs/bootstrap-slider/10.0.2/css/bootstrap-slider.min.css' />";
  page += "  <link rel='stylesheet' href='https://cdnjs.cloudflare.com/ajax/libs/bootstrap-slider/10.0.2/css/bootstrap-slider.css' />";
  page += "  <link rel='stylesheet' href='https://maxcdn.bootstrapcdn.com/bootstrap/4.0.0/css/bootstrap.min.css' integrity='sha384-Gn5384xqQ1aoWXA+058RXPxPg6fy4IWvTNh0E263XmFcJlSAwiGgFAW/dAiS6JXm' crossorigin='anonymous'>";
  page += "  <title>Arduino Dashboard</title>";
  page += "</head>";
  page += "<body>";
  page += "  <div class='container-fluid'>";
  page += "    <div class='row'>";
  page += "      <div class='col-md-12'>";
  page += "        <h1>Dashboard</h1>";
  page += "        <!-- Temperature section -->";
  page += "        <div class='row'>";
  page += "          <div class='col-md-4'>";
  page += "            <div class='card text-white bg-dark mb-3'>";
  page += "              <div class='card-header'>Room's temperature</div>";
  page += "              <div class='card-body'>";
  page += "                <h2 class='card-title'>21 &#8451;</h2>";
  page += "                <p class='card-text'>Chart is unavailable</p>";
  page += "              </div>";
  page += "              <div class='card-footer'>";
  page += "                <small class='text-muted'>Refreshed: 3 second ago</small>";
  page += "              </div>";
  page += "            </div>";
  page += "          </div>";
  page += "          <div class='col-md-4'>";
  page += "            <div class='card text-white bg-dark mb-3'>";
  page += "              <div class='card-header'>Water's temperature</div>";
  page += "              <div class='card-body'>";
  page += "                <h2 class='card-title'>17 &#8451;</h2>";
  page += "                <p class='card-text'>Chart is unavailable</p>";
  page += "              </div>";
  page += "              <div class='card-footer'>";
  page += "                <small class='text-muted'>Refreshed: 3 second ago</small>";
  page += "              </div>";
  page += "            </div>";
  page += "          </div>";
  page += "          <div class='col-md-4'>";
  page += "            <div class='card text-white bg-info mb-3'>";
  page += "              <div class='card-header'>Arduino info</div>";
  page += "              <div class='card-body'>";
  page += "                <h3 class='card-title'>Connection: ON</h3>";
  page += "                   <form action='/' method='POST' style='margin:0px'>";
  page += "                     <button type='button' class='btn btn-warning btn-sm' data-toggle='modal' data-target='#SetupTime'>Setup time</button>";
  page += "                     <button type='submit' name='clearAll' class='btn btn-warning btn-sm'>Clear cash and logs </button>";
  page += "                   </form>";
  page += "              </div>";
  page += "        <div class='card-footer'>";
  page += "         <small>Work duration:";
  page +=             GetCurrentTime();
  page += "         </small>";
  page += "         </div>";
  page += "            </div>";
  page += "          </div>";
  page += "        </div>";
  page += "        <!-- End of temp section-->";
  page += "        <!--LED section info-->";
  page += "        <h3>LED controller</h3>";
  page += "        <div id='LedContainer' style='min-width: 310px; height: 350px; margin: 0 auto'></div>";
  page += "        <br/>";
  page += "        <div class='col-md-10' style='margin: 0 auto'>";
  page += "          <table class='table table-striped' id='LedTable'>";
  page += "            <caption>";
  page += "              <div class='row'>";
  page += "                <div class='col-md-9'>";
  page += "                <i>Daytime LED-controller's value</i>";
  page += "              </div>";
  page += "                <div class='col-md-3'>";
  page += "                 <form action='/' method='POST'>";
  page += "                  <button type='button' class='btn btn-success btn-sm' data-toggle='modal' data-target='#CreateLedModal'>Add</button>";
  page += "                  <button type='submit' name='clearTimeTable' class='btn btn-danger btn-sm'>Remove ALL</button>";
  page += "                </div>";
  page += "               </form>";
  page += "              </div>";
  page += "            </caption>";
  page += "            <thead>";
  page += "              <tr>";
  page += "                <th>Time</th>";
  page += "                <th>Chanel 1 (dig unit)</th>";
  page += "                <th>Chanel 2 (dig unit)</th>";
  page += "                <th>Chanel 3 (dig unit)</th>";
  page += "                <th>Chanel 4 (dig unit)</th>";
  page += "              </tr>";
  page += "            </thead>";
  page += "            <tbody>";
  page +=                WriteLedTable();
  page += "            </tbody>";
  page += "          </table>";
  page += "        </div>";
  page += "        <!--END of LED section info-->";
  page += "        <h3>Additional board interface</h3>";
  page += "        <div class='row'>";
  page += "          <div class='col-md-4'>";
  page += "            <h3>GPIO</h3>";
  page += "            <div class='row'>";
  page += "              <div class='col-md-4'>";
  page += "                <h5 class='text-left'>";
  page += "                  D5  <span class='badge badge-pill badge-";
  page +=                   (sensors[GD5].value == 1) ? "primary" : "dark";
  page += "                 '>Light</span>";
  page += "                </h5>";
  page += "              </div>";
  page += "              <div class='col-md-4'>";
  page += "                <form action='/' method='POST'>";
  page += "                  <button type='button submit' name='GD5' value='1' class='btn btn-success'>ON</button>";
  page += "                </form>";
  page += "              </div>";
  page += "              <div class='col-md-4'>";
  page += "                <form action='/' method='POST'>";
  page += "                  <button type='button submit' name='GD5' value='0' class='btn btn-danger'>OFF</button>";
  page += "                </form>";
  page += "              </div>";
  page += "              <div class='col-md-4'>";
  page += "                <h5 class='text-left'>";
  page += "                  D7  <span class='badge badge-pill  badge-warning'><i>title</i></span>";
  page += "                </h5>";
  page += "              </div>";
  page += "              <div class='col-md-8'>";
  page += "              <form action='/' method='POST'>";
  page += "               <div class='row'>";
  page += "                  <div class='col-md-5'>";
  page += "                    <input type='time' class='form-control' name='GD7timeStart' value='" + GD7TimeTable.startTime + "' placeholder='20:30'>";
  page += "                  </div>";
  page += "                  <div class='col-md-2'>";
  page += "                    <p> -> </p>";
  page += "                  </div>";
  page += "                  <div class='col-md-5'>";
  page += "                     <input type='time' class='form-control' name='GD7timeEnd' value='" + GD7TimeTable.finishTime + "' placeholder='20:30'>";
  page += "                   </div>";
  page += "               </div>";
  page += "               <button type='submit' class='btn btn-info btn-sm btn-block'>SEND</button>";
  page += "             </form>";
  page += "            </div>";
  page += "              <div class='col-md-4'>";
  page += "                <h5 class='text-left'>";
  page += "                  D8  <span class='badge badge-pill  badge-warning'><i>title</i></span>";
  page += "                </h5>";
  page += "              </div>";
  page += "              <div class='col-md-4'>";
  page += "                  <input id='GD8ex' data-slider-id='ex1Slider' type='button submit' data-slider-min='0' data-slider-max='1023' data-slider-step='1' style='width:100px' data-slider-value='";
  page +=                     sensors[GD8].value;
  page += "                   ' />";
  page += "                  <span id='ex6CurrentSliderValLabel'>Value: <span id='GD8exVal'>";
  page +=                       sensors[GD8].value;
  page += "                   </span></span>";
  page += "              </div>";
  page += "              <div class='col-md-4'>";
  page += "                <form action='/' method='POST'>";
  page += "                  <button id='GD8Button' type='button submit' name='GD8' value='0' class='btn btn-info btn-sm'>SEND</button>";
  page += "                </form>";
  page += "              </div>";
  page += "            </div>";
  page += "          </div>";
  page += "";
  page += "        <!--End of GPIO section -->";
  page += "";
  page += "        <!-- Console section -->";
  page += "        <div class='col-md-8' style='height: 250px; overflow-y: scroll'>";
  page += "          <nav id='console-bar' class='navbar sticky-top navbar-light bg-light sticky'>";
  page += "           <a class='navbar-brand' data-toggle='collapse' href='#multiCollapseExample1' role='button' aria-expanded='false' aria-controls='multiCollapseExample1'>Show console log</a>";
  page += "          </nav>";
  page += "          <div class='collapse multi-collapse' id='multiCollapseExample1' style='margin: 10px'>";
  page += "            <div data-spy='scroll' data-target='#navbar-example2' data-offset='0'>";
  page += logString;
  page += "            </div>";
  page += "          </div>";
  page += "        </div>";
  page += "      </div>";
  page += "      </br>";
  page += "      </div>";
  page += "    </div>";
  page += "  </div>";
  page += "  </div>";
  page += "  <div class='modal fade' id='CreateLedModal' tabindex='-1' role='dialog' aria-labelledby='myModalLabel' aria-hidden='true'>";
  page += "  <div class='modal-dialog' role='document'>";
  page += "    <div class='modal-content'>";
  page += "      <div class='modal-header'>";
  page += "        <h4 class='modal-title'>New event of LED controller</h4>";
  page += "        <button type='button' class='close' data-dismiss='modal' aria-label='Close'>";
  page += "          <span aria-hidden='true'>&times;</span>";
  page += "        </button>";
  page += "      </div>";
  page += "      <div class='modal-body'>";
  page += "        <form action='/' method='POST'>";
  page += "          <div class='form-group row'>";
  page += "            <label for='colFormLabel' class='col-sm-2 col-form-label'>Time</label>";
  page += "            <div class='col-sm-10'>";
  page += "              <input type='time' min='0' max='1023' class='form-control' name='LEDtime' placeholder='20:30'>";
  page += "            </div>";
  page += "          </div>";
  page += "          <div class='form-group row'>";
  page += "            <label for='colFormLabel' class='col-sm-2 col-form-label'>Chanel 1</label>";
  page += "            <div class='col-sm-10'>";
  page += "              <input type='number' min='0' max='1023' class='form-control' name='LEDchanel1' placeholder='0'>";
  page += "            </div>";
  page += "          </div>";
  page += "          <div class='form-group row'>";
  page += "            <label for='colFormLabel' class='col-sm-2 col-form-label'>Chanel 2</label>";
  page += "            <div class='col-sm-10'>";
  page += "              <input type='number' min='0' max='1023' class='form-control' name='LEDchanel2' placeholder='255'>";
  page += "            </div>";
  page += "          </div>";
  page += "          <div class='form-group row'>";
  page += "            <label for='colFormLabel' class='col-sm-2 col-form-label'>Chanel 3</label>";
  page += "            <div class='col-sm-10'>";
  page += "              <input type='number' min='0' max='1023' class='form-control' name='LEDchanel3' placeholder='0'>";
  page += "            </div>";
  page += "          </div>";
  page += "          <div class='form-group row'>";
  page += "            <label for='colFormLabel' class='col-sm-2 col-form-label'>Chanel 4</label>";
  page += "            <div class='col-sm-10'>";
  page += "              <input type='number' class='form-control' name='LEDchanel4' placeholder='255'>";
  page += "            </div>";
  page += "          </div>";
  page += "          <div class='modal-footer'>";
  page += "            <button type='reset' class='btn btn-secondary'>Reset</button>";
  page += "            <button type='submit' class='btn btn-primary'>Save</button>";
  page += "         </div>";
  page += "        </form>";
  page += "      </div>";
  page += "    </div>";
  page += "  </div>";
  page += "</div>";
  page += "";
  page += "  <div class='modal fade' id='SetupTime' tabindex='-1' role='dialog' aria-labelledby='myModalLabel' aria-hidden='true'>";
  page += "    <div class='modal-dialog' role='document'>";
  page += "      <div class='modal-content'>";
  page += "        <div class='modal-header'>";
  page += "          <h4 class='modal-title'>Time's settings</h4>";
  page += "          <button type='button' class='close' data-dismiss='modal' aria-label='Close'>";
  page += "            <span aria-hidden='true'>&times;</span>";
  page += "          </button>";
  page += "        </div>";
  page += "        <div class='modal-body'>";
  page += "          <form action='/' method='POST'>";
  page += "            <div class='form-group row'>";
  page += "              <label for='colFormLabel' class='col-sm-2 col-form-label'>Time</label>";
  page += "              <div class='col-sm-10'>";
  page += "                <input type='time' class='form-control' name='SystemTime' placeholder='20:30'>";
  page += "              </div>";
  page += "            </div>";
  page += "            ";
  page += "            <div class='modal-footer'>";
  page += "              <button type='reset' class='btn btn-secondary'>Reset</button>";
  page += "              <button type='submit' class='btn btn-primary'>Save</button>";
  page += "           </div>";
  page += "          </form>";
  page += "        </div>";
  page += "      </div>";
  page += "    </div>";
  page += "  </div>";
  page += "  <script type='text/javascript'>";
  page += "    Highcharts.chart('LedContainer', {";
  page += "      data: {";
  page += "        table: 'LedTable'";
  page += "      },";
  page += "      chart: {";
  page += "        type: 'line'";
  page += "      },";
  page += "      title: {";
  page += "        text: 'Daytime term'";
  page += "      },";
  page += "      yAxis: {";
  page += "        allowDecimals: true,";
  page += "        title: {";
  page += "          text: 'Value of unit'";
  page += "        }";
  page += "      },";
  page += "      tooltip: {";
  page += "        formatter: function () {";
  page += "          return '<b>' + this.series.name + '</b><br/>' +";
  page += "          'время:' + this.point.name.toLowerCase() + '<br/>' + this.point.y;";
  page += "        }";
  page += "      }";
  page += "    });";
  page += "  </script>";
  page += "  <script type='text/javascript'>";
  page += "      $(document).ready(function () {";
  page += "          $('#GD8ex').slider({";
  page += "              formatter: function (value) {";
  page += "                  return 'Current value: ' + value;";
  page += "              },";
  page += "              tooltip: 'always'";
  page += "          });";
  page += "          $('#GD8ex').on('slide', function(slideEvt) {";
  page += "           $('#GD8exVal').text(slideEvt.value);";
  page += "            $('#GD8Button').attr('value', slideEvt.value);";
  page += "          });";
  page += "      });";
  page += "  </script>";
  page += "</body>";
  page += "</html>";
  return page;
}

