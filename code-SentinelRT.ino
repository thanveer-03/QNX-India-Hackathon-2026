#include <LiquidCrystal.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <HardwareSerial.h>

HardwareSerial sim800(1); // UART1

LiquidCrystal lcd(13, 12, 14, 27, 26, 25);


#define ONE_WIRE_BUS 4
#define current_sen 18//regulator
#define fire 15
#define buzzer 5
#define relay_battery 19
#define relay_fan  21//33

const int voltagePin = 34;

const float R1 = 30000.0;
const float R2 = 10000.0;
float inputVoltage;




OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);


const char* ssid = "a";
const char* password = "1234567890";

String apiKey = "U5QNRDI1OJUCH1TQ";  //UPY099IS3UF0TQ0O





void setup() {
  lcd.begin(16, 2);         // Initialize LCD
  Serial.begin(115200);
  sim800.begin(9600, SERIAL_8N1, 16, 17);
  Serial.println("Initializing GSM Module...");
  sensors.begin();
  pinMode(buzzer, OUTPUT);
  pinMode(current_sen, INPUT);
  pinMode(fire, INPUT);
  pinMode(relay_battery, OUTPUT);
  pinMode(relay_fan, OUTPUT);

  digitalWrite(relay_battery, LOW); //on
  digitalWrite(relay_fan, HIGH);//off

  lcd.print("EV Battery Fault ");
  lcd.setCursor(0, 1);
  lcd.print("Detection &Alert ");
  delay(2000);
  Serial.println("EV Battery Fault Detection & Alert system");


  analogReadResolution(12);       // 0 to 4095
  analogSetAttenuation(ADC_11db);  // Higher input range


  //--------------iot esp32-----------
  lcd.clear();
  lcd.print("WiFi Connecting");
  delay(500);
  WiFi.begin(ssid, password);
  lcd.clear();
  lcd.print("WiFi Connected ");
  delay(500);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("      GSM      ");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");

  sim800.println("AT+CNMI=2,2,0,0,0");
  delay(3500);
  sim800.println("AT+CMGF=1");
  delay(3500);
  sim800.println("AT+CMGS=\"+917995597623\"\r"); // Replace x with mobile number 7671956462
  delay(3500);
  sim800.println("System is ready ");// The SMS text you want to send
  delay(3500);
  sim800.println((char)26); // ASCII code of CTRL+Z
  delay(3000);

  lcd.setCursor(0, 0);
  lcd.print(" Initialized  ");
  lcd.setCursor(0, 1);
  lcd.print(" Successfully ");
  delay(500);
  lcd.clear();
}  //setup





void loop() {


  sensors.requestTemperatures();
  vol_measure();

  lcd.setCursor(0, 0);
  lcd.print("Temp");
  lcd.setCursor(0, 1);
  lcd.print(int(sensors.getTempCByIndex(0)));

  lcd.setCursor(5, 0);
  lcd.print("Fire");
  lcd.setCursor(6, 1);
  lcd.print(!digitalRead(fire));

  lcd.setCursor(10, 0);
  lcd.print("Cur");
  lcd.setCursor(10, 1);
  lcd.print(digitalRead(current_sen));

  lcd.setCursor(14, 0);
  lcd.print("Vol");
  lcd.setCursor(13, 1);
  lcd.print(inputVoltage);

  delay(1200);
  lcd.clear();


  //-------------------------comparision-----------------------------
  if (sensors.getTempCByIndex(0) > 60)
  {
    digitalWrite(relay_fan, LOW);//on
    lcd.setCursor(0, 0);
    lcd.print("Temperature     ");
    lcd.setCursor(0, 1);
    lcd.print("Increase Alert..");
    digitalWrite(buzzer, HIGH);
    delay(1000);
    //send_temp_sms();
    digitalWrite(buzzer, LOW);
    lcd.clear();

  }
  else
  {
    digitalWrite(relay_fan, HIGH);//off
    lcd.setCursor(0, 0);
    lcd.print("   Temperature  ");
    lcd.setCursor(0, 1);
    lcd.print("     Normal     ");
    delay(500);
    lcd.clear();
  }

  if (digitalRead(current_sen) == 1)//short circuit
  {
    digitalWrite(relay_battery, HIGH);//off
    lcd.setCursor(0, 0);
    lcd.print("  Current High  ");
    lcd.setCursor(0, 1);
    lcd.print("  Disconnected  ");
    digitalWrite(buzzer, HIGH);
    send_current_sms();
    digitalWrite(buzzer, LOW);
    lcd.clear();
    digitalWrite(relay_battery, LOW);//on
  }
  else
  {
    digitalWrite(relay_battery, LOW);//on
    lcd.setCursor(0, 0);
    lcd.print("   Current      ");
    lcd.setCursor(0, 1);
    lcd.print(" Normal Load    ");
    delay(500);
    lcd.clear();
  }
  ///----------fire------------------------------
  if (digitalRead(fire) == 0)//flame
  {
    lcd.setCursor(0, 0);
    lcd.print("     Fire       ");
    lcd.setCursor(0, 1);
    lcd.print("    Detected    ");
    digitalWrite(buzzer, HIGH);
    send_fire_sms();
    digitalWrite(buzzer, LOW);
    lcd.clear();

  }
  else
  {
    lcd.setCursor(0, 0);
    lcd.print("     Fire      ");
    lcd.setCursor(0, 1);
    lcd.print(" Not Detected   ");
    delay(500);
    lcd.clear();
  }

  ///----------vol------------------------------
  if (inputVoltage < 5)//flame
  {
    lcd.setCursor(0, 0);
    lcd.print("     LOW        ");
    lcd.setCursor(0, 1);
    lcd.print("   Voltage      ");
    digitalWrite(buzzer, HIGH);
    delay(500);
    digitalWrite(buzzer, LOW);
    lcd.clear();

  }
  else
  {
    lcd.setCursor(0, 0);
    lcd.print("    Voltage     ");
    lcd.setCursor(0, 1);
    lcd.print("     Normal     ");
    delay(500);
    lcd.clear();
  }

  //-------------------------------------- SEND DATA-------------------------------
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String url = "http://api.thingspeak.com/update?api_key=" + apiKey + "&field1=" + String(sensors.getTempCByIndex(0)) + "&field2=" + String(digitalRead(fire)) + "&field3=" + String(digitalRead(current_sen)) + "&field4=" + String(digitalRead(inputVoltage));


    http.begin(url);
    int httpCode = http.GET();

    Serial.print("HTTP Response: ");
    Serial.println(httpCode);

    http.end();
  }

  // delay(15000);   // ThingSpeak delay

}//loop

//-----------voltage measure---------------------------------
void vol_measure()
{
  int adcValue = analogRead(voltagePin);

  float adcVoltage = (adcValue / 4095.0) * 3.3;

  inputVoltage = adcVoltage * ((R1 + R2) / R2);
  inputVoltage = inputVoltage *1.4;
  Serial.print("ADC Value: ");
  Serial.print(adcValue);

  Serial.print("   Voltage: ");
  Serial.print(inputVoltage, 2);
  Serial.println(" V");

}
//----------------------gsm--------------------------
void send_current_sms()
{

  lcd.setCursor(0, 0);
  lcd.print("       SMS     ");
  lcd.setCursor(0, 1);
  lcd.print("  Sending......");

  init_sms();
  delay(3000);
  send_data("Alert!High current detected.take immediate action");
  delay(3000);
  send_sms();

  lcd.setCursor(0, 0);
  lcd.print("       SMS     ");
  lcd.setCursor(0, 1);
  lcd.print("      Sent     ");
  delay(500);

}

void send_fire_sms()
{

  lcd.setCursor(0, 0);
  lcd.print("       SMS     ");
  lcd.setCursor(0, 1);
  lcd.print("  Sending......");

  init_sms();
  delay(2000);
  send_data("Alert!Fire is detected.take immediate action");
  delay(2000);
  send_sms();

  lcd.setCursor(0, 0);
  lcd.print("       SMS     ");
  lcd.setCursor(0, 1);
  lcd.print("      Sent     ");
  delay(500);

}
void init_sms()
{
  sim800.println("AT+CNMI=2,2,0,0,0");
  delay(3500);
  sim800.println("AT+CMGF=1");
  delay(3500);
  sim800.println("AT+CMGS=\"+917995597623\"\r"); // Replace x with mobile number
  delay(3500);
}



void send_data(String message)
{
  sim800.println(message);
  delay(3000);
}
void send_sms()
{
  sim800.write(26);
  delay(2000);
}