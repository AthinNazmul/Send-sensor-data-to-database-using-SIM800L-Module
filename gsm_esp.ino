#include <DHT.h>
#include <HardwareSerial.h>

#define DHT11_PIN 25
#define DHTTYPE DHT11
DHT dht(DHT11_PIN, DHTTYPE);

#define SIM800_RX_PIN 26  
#define SIM800_TX_PIN 27  
HardwareSerial SIM800Serial(2);  

const String APN = "WAP";  
const String USER = "";  
const String PASS = "";  
const String FIREBASE_HOST = "https://dht-sensor-data-2b533-default-rtdb.asia-southeast1.firebasedatabase.app/";  
const String FIREBASE_SECRET = "8OVT2hlalceordAAykz81t7VdNbx0XC9VSLqsEOX";  

#define USE_SSL true
#define DELAY_MS 500

void setup() {
  Serial.begin(115200);
  dht.begin();
  SIM800Serial.begin(9600, SERIAL_8N1, SIM800_RX_PIN, SIM800_TX_PIN);

  Serial.println("Initializing SIM800...");
  init_gsm();
}

void loop() {
  String data = get_temperature();
  Serial.println(data);

  if (!is_gprs_connected()) {
    gprs_connect();
  }

  post_to_firebase(data);

  delay(1000);
}

String get_temperature() {
  topFn:
  String h = String(dht.readHumidity(), 2);
  String t = String(dht.readTemperature(), 2);

  if (h == "" || t == "") {
    Serial.println(F("Failed to read from DHT sensor!"));
    goto topFn;
  }

  String Data = "{";
  Data += "\"temperature\":\"" + t + "\",";
  Data += "\"humidity\":\"" + h + "\"";
  Data += "}";

  return Data;
}

void post_to_firebase(String data) {
  SIM800Serial.println("AT+HTTPINIT");
  waitResponse("OK", 1000);  
  delay(DELAY_MS);

  if (USE_SSL == true) {
    SIM800Serial.println("AT+HTTPSSL=1");
    waitResponse("OK", 1000); 
    delay(DELAY_MS);
  }

  SIM800Serial.println("AT+HTTPPARA=\"CID\",1");
  waitResponse("OK", 1000);  
  delay(DELAY_MS);

  SIM800Serial.println("AT+HTTPPARA=\"URL\"," + FIREBASE_HOST + ".json?auth=" + FIREBASE_SECRET);
  waitResponse("OK", 1000);  
  delay(DELAY_MS);

  SIM800Serial.println("AT+HTTPPARA=\"REDIR\",1");
  waitResponse("OK", 1000);  
  delay(DELAY_MS);

  SIM800Serial.println("AT+HTTPPARA=\"CONTENT\",\"application/json\"");
  waitResponse("OK", 1000); 
  delay(DELAY_MS);

  SIM800Serial.println("AT+HTTPDATA=" + String(data.length()) + ",10000");
  waitResponse("DOWNLOAD", 10000);  

  SIM800Serial.println(data);
  waitResponse("OK", 1000); 
  delay(DELAY_MS);

  SIM800Serial.println("AT+HTTPACTION=1");
  waitResponse("+HTTPACTION:", 20000);  

  delay(DELAY_MS);

  SIM800Serial.println("AT+HTTPREAD");
  waitResponse("OK", 1000);  
  delay(DELAY_MS);

  SIM800Serial.println("AT+HTTPTERM");
  waitResponse("OK", 1000);  
  delay(DELAY_MS);
}

void init_gsm() {
  SIM800Serial.println("AT");
  waitResponse("OK", 1000); 
  delay(DELAY_MS);

  SIM800Serial.println("AT+CPIN?");
  waitResponse("+CPIN: READY", 10000);  
  delay(DELAY_MS);

  SIM800Serial.println("AT+CFUN=1");
  waitResponse("OK", 1000);  
  delay(DELAY_MS);

  SIM800Serial.println("AT+CMEE=2");
  waitResponse("OK", 1000);  
  delay(DELAY_MS);

  SIM800Serial.println("AT+CBATCHK=1");
  waitResponse("OK", 1000);  
  delay(DELAY_MS);

  SIM800Serial.println("AT+CREG?");
  waitResponse("+CREG: 0,", 1000);  
  delay(DELAY_MS);

  SIM800Serial.print("AT+CMGF=1\r");
  waitResponse("OK", 1000); 
  delay(DELAY_MS);
}

void gprs_connect() {
  SIM800Serial.println("AT+SAPBR=0,1");
  waitResponse("OK", 60000); 
  delay(DELAY_MS);

  SIM800Serial.println("AT+SAPBR=3,1,\"Contype\",\"GPRS\"");
  waitResponse("OK", 1000);  
  delay(DELAY_MS);

  SIM800Serial.println("AT+SAPBR=3,1,\"APN\"," + APN);
  waitResponse("OK", 1000);  
  delay(DELAY_MS);

  if (USER != "") {
    SIM800Serial.println("AT+SAPBR=3,1,\"USER\"," + USER);
    waitResponse("OK", 1000);  
    delay(DELAY_MS);
  }

  if (PASS != "") {
    SIM800Serial.println("AT+SAPBR=3,1,\"PASS\"," + PASS);
    waitResponse("OK", 1000);  
    delay(DELAY_MS);
  }

  SIM800Serial.println("AT+SAPBR=1,1");
  waitResponse("OK", 30000); 
  delay(DELAY_MS);

  SIM800Serial.println("AT+SAPBR=2,1");
  waitResponse("OK", 1000);  
  delay(DELAY_MS);
}

boolean is_gprs_connected() {
  SIM800Serial.println("AT+CGATT?");
  if (waitResponse("+CGATT: 1", 6000) == 1) {
    return false;
  }

  return true;
}

boolean waitResponse(String expected_answer, unsigned int timeout) {
  uint8_t x = 0, answer = 0;
  String response;
  unsigned long previous;

  while (SIM800Serial.available() > 0) SIM800Serial.read();

  previous = millis();
  do {
    if (SIM800Serial.available() != 0) {
      char c = SIM800Serial.read();
      response.concat(c);
      x++;
      if (response.indexOf(expected_answer) > 0) {
        answer = 1;
      }
    }
  } while ((answer == 0) && ((millis() - previous) < timeout));

  Serial.println(response);
  return answer;
}
