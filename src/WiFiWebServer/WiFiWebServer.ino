/*
 *  This sketch demonstrates how to set up a simple HTTP-like server.
 *  The server will set a GPIO pin depending on the request
 *    http://server_ip/gpio/0 will set the GPIO2 low,
 *    http://server_ip/gpio/1 will set the GPIO2 high
 *  server_ip is the IP address of the ESP8266 module, will be 
 *  printed to Serial when the module is connected.
 */
#include <ESP8266WiFi.h>

#define bufferMax 128
int bufferSize;
char buffer[bufferMax];
String readString = String(128);
char post;

enum logs_state {NORMAL, SUCCESS, WARNING, DANGER};
enum sensor_name {LED_1, LED_2, LED_3, LED_4, D5, D6, D7, D8};

const char* ssid = "WiFi-DOM.ru-2463";
const char* password = "89502657277";

String logString = "";

// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(80);

typedef struct {
  sensor_name sensorName;
  int value;
  int pin;
  bool isPwm;
} State;

typedef struct {
  String timeExecute;
  int cahnel1;
  int cahnel2;
  int cahnel3;
  int cahnel4;
} LedSchedule;

State sensors[8] = {
   {LED_1, 0, 16, true},
   {LED_2, 0, 5, true},
   {LED_3, 0, 4, true},
   {LED_4, 0, 2, false},
   {D5, 0, 14, false},
   {D6, 0, 12, false},
   {D7, 0, 13, false},
   {D8, 0, 15, false}
};

LedSchedule timetable[24] = {
  {"08:00", 132, 10, 200, 233},
  {"09:30", 12, 120, 18, 180},
  {"10:23", 100, 100, 200, 255}
};

String WriteLedTable(){
  String row = "";
  for(int i = 0; i < sizeof(timetable); i++) {
    row += "              <tr>";
    row += "                <td>";
    row +=                    timetable[i].timeExecute;
    row +=                  "</td>";
    row += "                <td>";
    row +=                    timetable[i].cahnel1;
    row +=                  "</td>";
    row += "                <td>";
    row +=                    timetable[i].cahnel2;
    row +=                  "</td>";
    row += "                <td>";
    row +=                    timetable[i].cahnel3;
    row +=                  "</td>";
    row += "                <td>";
    row +=                    timetable[i].cahnel4;
    row +=                  "</td>";
    row += "              </tr>";
  }
  return row;
}

String GetPage(){
  String page = "";
  page += "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n";
  page += "<html charset=UTF-8>";
  page += "<head>";
  page += "  <meta http-equiv='Content-Type' content='text/html; charset=utf-8' name='viewport' content='width=device-width, initial-scale=1' />";
  page += "  <script src='https://code.jquery.com/jquery-3.2.1.slim.min.js' integrity='sha384-KJ3o2DKtIkvYIK3UENzmM7KCkRr/rE9/Qpg6aAZGJwFDMVNA/GpGFF93hXpG5KkN' crossorigin='anonymous'></script>";
  page += "  <script src='https://cdnjs.cloudflare.com/ajax/libs/popper.js/1.12.9/umd/popper.min.js' integrity='sha384-ApNbgh9B+Y1QKtv3Rn7W3mgPxhU9K/ScQsAP7hUibX39j7fakFPskvXusvfa0b4Q' crossorigin='anonymous'></script>";
  page += "  <script src='https://maxcdn.bootstrapcdn.com/bootstrap/4.0.0/js/bootstrap.min.js' integrity='sha384-JZR6Spejh4U02d8jOt6vLEHfe/JQGiRRSQQxSfFWpi1MquVdAyjUar5+76PVCmYl' crossorigin='anonymous'></script>";
  page += "  <script src='https://code.highcharts.com/highcharts.js'></script>";
  page += "  <script src='https://code.highcharts.com/modules/data.js'></script>";
  page += "  <script src='https://code.highcharts.com/modules/exporting.js'></script>";
  page += "  <script src='https://code.highcharts.com/modules/export-data.js'></script>";
  page += "  <script src='https://cdnjs.cloudflare.com/ajax/libs/bootstrap-slider/10.0.2/bootstrap-slider.min.js'></script>";
  page += "  <link rel='stylesheet' href='https://cdnjs.cloudflare.com/ajax/libs/bootstrap-slider/10.0.2/css/bootstrap-slider.min.css' />";
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
  page += "                <div class='row'>";
  page += "                  <div class='col-md-3'>";
  page += "                    <h6 class='card-title'>RAM: </h6>";
  page += "                  </div>";
  page += "                  <div class='col-md-9'>";
  page += "                    <div class='progress'>";
  page += "                      <div class='progress-bar bg-warning' role='progressbar' style='width: 75%' aria-valuenow='75' aria-valuemin='0' aria-valuemax='100'></div>";
  page += "                    </div>";
  page += "                  </div>";
  page += "                </div>";
  page += "              </div>";
  page += "        <div class='card-footer'>";
  page += "       <small>Work duration:</small>";
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
  page += "                  <button type='button' class='btn btn-success btn-sm' data-toggle='modal' data-target='#CreateLedModal'>Add</button>";
  page += "                  <button type='button' class='btn btn-danger btn-sm'>Remove ALL</button>";
  page += "                </div>";
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
  page +=                "WriteLedTable()";
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
  page += "                  D5  <span class='badge badge-pill  badge-primary'>Light</span>";
  page += "                </h5>";
  page += "              </div>";
  page += "              <div class='col-md-4'>";
  page += "                <form action='/' method='POST'>";
  page += "                  <button type='button submit' name='D5' value='1' class='btn btn-success'>ON</button>";
  page += "                </form>";
  page += "              </div>";
  page += "              <div class='col-md-4'>";
  page += "                <form action='/' method='POST'>";
  page += "                  <button type='button submit' name='D5' value='0' class='btn btn-danger'>OFF</button>";
  page += "                </form>";
  page += "              </div>";
  page += "              <div class='col-md-4'>";
  page += "                <h5 class='text-left'>";
  page += "                  D6  <span class='badge badge-pill  badge-primary'>Cooler</span>";
  page += "                </h5>";
  page += "              </div>";
  page += "              <div class='col-md-4'>";
  page += "                <form action='/' method='POST'><button type='button submit' name='D6' value='1' class='btn btn-success'>ON</button></form>";
  page += "              </div>";
  page += "              <div class='col-md-4'>";
  page += "                <form action='/' method='POST'><button type='button submit' name='D6' value='0' class='btn btn-danger'>OFF</button></form>";
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
  page += "                    <input type='time' class='form-control' name='D7timeStart' placeholder='20:30'>";
  page += "                  </div>";
  page += "                  <div class='col-md-2'>";
  page += "                    <p> -> </p>";
  page += "                  </div>";
  page += "                  <div class='col-md-5'>";
  page += "                     <input type='time' class='form-control' name='D7timeEnd' placeholder='20:30'>";
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
  page += "                <form method='POST'>";
  page += "                  <input id='D8ex' data-slider-id='ex1Slider' type='button submit' data-slider-min='0' data-slider-max='255' data-slider-step='1' data-slider-value='14' style='width:100px'/>";
  page += "                  <span id='ex6CurrentSliderValLabel'>Value: <span id='D8exVal'>3</span></span>";
  page += "                </form>";
  page += "              </div>";
  page += "              <div class='col-md-4'>";
  page += "                <form action='/' method='POST'>";
  page += "                  <button id='D8Button' type='button submit' name='D8' value='0' class='btn btn-info btn-sm'>SEND</button>";
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
  page += "              <input type='time' class='form-control' name='LEDtime' placeholder='20:30'>";
  page += "            </div>";
  page += "          </div>";
  page += "          <div class='form-group row'>";
  page += "            <label for='colFormLabel' class='col-sm-2 col-form-label'>Chanel 1</label>";
  page += "            <div class='col-sm-10'>";
  page += "              <input type='number' class='form-control' name='LEDchanel1' placeholder='0'>";
  page += "            </div>";
  page += "          </div>";
  page += "          <div class='form-group row'>";
  page += "            <label for='colFormLabel' class='col-sm-2 col-form-label'>Chanel 2</label>";
  page += "            <div class='col-sm-10'>";
  page += "              <input type='number' class='form-control' name='LEDchanel2' placeholder='255'>";
  page += "            </div>";
  page += "          </div>";
  page += "          <div class='form-group row'>";
  page += "            <label for='colFormLabel' class='col-sm-2 col-form-label'>Chanel 3</label>";
  page += "            <div class='col-sm-10'>";
  page += "              <input type='number' class='form-control' name='LEDchanel13' placeholder='0'>";
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
  page += "          $('#D8ex').slider({";
  page += "              formatter: function (value) {";
  page += "                  return 'Current value: ' + value;";
  page += "              },";
  page += "              tooltip: 'always'";
  page += "          });";
  page += "          $('#D8ex').on('slide', function(slideEvt) {";
  page += "           $('#D8exVal').text(slideEvt.value);";
  page += "            $('#D8Button').attr('value', slideEvt.value);";
  page += "          });";
  page += "      });";
  page += "  </script>";
  page += "</body>";
  page += "</html>";
  return page;
}

void Log(String text, logs_state status){
   String color[4] = {"black", "green", "yellow", "red"};
  logString += "<p style='color:" + color[status] +"' >" + text + "</p>";
}

void setup() {
  Serial.begin(115200);
  delay(10);
  Serial.println();
  Serial.println("start configuring");
  delay(10);
  ConfigureGpio();
  ConfigureWiFi();
 
}

void loop() {
  ApplyCurrentState();
  delay(1);
  // Send the response to the client
  getPostRequest();
  delay(1);
}

void ConfigureWiFi(){
    // Connect to WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  Log("Connecting to ", NORMAL);
  Log(ssid, NORMAL);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  Log("WiFi connected", SUCCESS);
  // Start the server
  server.begin();
  Log("Server started", SUCCESS);
  Log(WiFi.localIP().toString(), NORMAL);
  
  // Print the IP address
  Serial.println(WiFi.localIP()); 
}

void ConfigureGpio(){
  Serial.println("GPIO...");
  Serial.println(String(sizeof(sensor_name)));
  Serial.println(String(sizeof(timetable)));
  for(int i = 0; i < 8; i++){
    if(sensors[i].isPwm) {
      Serial.println("skip");
      //analogWrite(sensors[i].pin, sensors[i].value);
    } else {
      Serial.println("do");
      pinMode(sensors[i].pin, OUTPUT);
      digitalWrite(sensors[i].pin, sensors[i].value);
    }
  }
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
        // если вы получили символ новой строки
        // и символ пустой строки, то POST запрос закончился
        // и вы можете отправить ответ
        if (c == '\n' && currentLineIsBlank) {
          // Здесь содержатся данные POST запроса 
          while(client.available()) {  
            post = client.read();   
            if(bufferSize < bufferMax)
              buffer[bufferSize++] = post;  // сохраняем новый символ в буфере и создаем приращение bufferSize 
          }

          client.flush();
          Serial.println("Received POST request:");
          // Разбор HTTP POST запроса                  
          Serial.println(buffer);
          // Выполнение команд
          PerformRequestedCommands();
          // Отправка ответа
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
    Serial.println("Port closed");
  }
}

void UpdatePinValue(sensor_name sensorName, int value) {
  if(sensors[sensorName].sensorName == sensorName){
    sensors[sensorName].value = value;
    Log(String(sensorName) + " sensor was updated to " + String(value), SUCCESS);
  }else{
    Log(String(sensorName) + " sensor was not updated", DANGER);
  }
}

void ApplyCurrentState(){
  for(int i=4; i < 8; i++) {
    digitalWrite(sensors[i].pin, sensors[i].value);
  }
}

void PerformRequestedCommands() {
  readString = buffer;
  if(readString.indexOf("D5") != -1) { 
    int value = getValueFromHtmlForm("D5", readString).toInt();
    UpdatePinValue(D5, value);              
  } else if(readString.indexOf("D6") != -1) {
    int value = getValueFromHtmlForm("D6", readString).toInt();
    UpdatePinValue(D6, value);
  } else if(readString.indexOf("D8") != -1) {
    int value = getValueFromHtmlForm("D8", readString).toInt();
    UpdatePinValue(D8, value);
  }else {
    Serial.println("do it nothing");
  }

  if(readString.indexOf("D7") != -1) {
    saveD7TimeShcedule(readString);
  }
}

int GetFreeCellInTimeTable() {
  for(int i =0; i < sizeof(timetable); i++){
    if (sizeof(timetable[i].timeExecute) == 0)
      return i;
  }
  for(int i = 1; i < sizeof(timetable); i++) {
    timetable[i-1] = timetable[i];
  }
  return sizeof(timetable) - 1;
}

void PerformNewLedEvent(String requestBody) {
  int chanel1 = getValueFromHtmlForm("LEDchanel1", requestBody).toInt();
  int chanel2 = getValueFromHtmlForm("LEDchanel2", requestBody).toInt();
  int chanel3 = getValueFromHtmlForm("LEDchanel3", requestBody).toInt();
  int chanel4 = getValueFromHtmlForm("LEDchanel4", requestBody).toInt();

  int index = GetFreeCellInTimeTable();
  timetable[index] = {"time", chanel1, chanel2, chanel3, chanel4};
}

void saveD7TimeShcedule(String requestBody){
  String startTime = getValueFromHtmlForm("timeStart", requestBody);
  startTime = startTime.substring(0, startTime.indexOf("&D7timeEnd"));
  String endTime = getValueFromHtmlForm("timeEnd", requestBody);

  startTime.replace("%3A", ":");
  endTime.replace("%3A", ":");
}

String getValueFromHtmlForm(String gpioName, String requestBody){
  int startIndex = requestBody.indexOf(gpioName + "=");
    return requestBody.substring(startIndex + gpioName.length() + 1);
}

