#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

int myFunction(int, int);

/*浼犳劅鍣ㄥ搴斿紩鑴氬拰AD鍊肩殑瀹氫箟*/
#define Sersor_Pin_Temp 12 //娓╁害浼犳劅鍣ㄧ殑AO寮曡剼鎺sp32鐨?2鍙峰紩鑴?
#define Sersor_Pin_Humi 13 //婀垮害浼犳劅鍣ㄧ殑AO寮曡剼鎺sp32鐨?3鍙峰紩鑴?
int SersorValue_Temp;
int SersorValue_Humi; 
int Timer_Value = 0;

/*wifi鍚嶇О浠ュ強瀵嗙爜*/
const char *Wifi_Name = "Redmi K50";
const char *Wifi_Passward = "xxy031205";

/*mqtt鐨勬湇鍔″櫒鍦板潃浠ュ強绔彛鍙? 浠ュ強esp32瀹㈡埛绔渶璁㈤槄鐨勪富棰樺悕*/
const char *Mqtt_ServerAddress = "mqtt.rymcu.com";
const uint16_t Mqtt_Com = 8883;
const char *R_Topic = "Order_esp32_112233";

/*閰嶇疆CA璇佷功*/
const char *ca_cert = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh\n" \
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n" \
"d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD\n" \
"QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT\n" \
"MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n" \
"b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG\n" \
"9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB\n" \
"CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97\n" \
"nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt\n" \
"43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P\n" \
"T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4\n" \
"gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO\n" \
"BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR\n" \
"TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw\n" \
"DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr\n" \
"hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg\n" \
"06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF\n" \
"PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls\n" \
"YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk\n" \
"CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=" \
"-----END CERTIFICATE-----\n";

WiFiClientSecure Wifi_Client;
PubSubClient Mqtt_Client(Wifi_Client);

void Mqtt_Connect();
void WiFi_Connect();
int Value_Filter(int i); //璇诲彇涓€杩炰覆SersorValue鍙栦腑鍊?
void Timer_Init(); //瀹氭椂鍣ㄥ垵濮嬪寲
void IRAM_ATTR Ontime(); //瀹氭椂鍣ㄤ腑鏂嚱鏁?
void Report_Data();//鏁版嵁涓婃姤鑷砿qtt鏈嶅姟绔?
void Receive(char *Topic, byte *Payload, unsigned int Length);//鍥炶皟鍑芥暟

void setup() 
{
  int result = myFunction(2, 3);
  Serial.begin(115200);
  Serial.println("Opening");
  WiFi.mode(WIFI_AP_STA);
  Wifi_Client.setCACert(ca_cert);
  
  Timer_Init();
  
  //杩炴帴wifi
  WiFi_Connect();
  //杩炴帴mqtt鏈嶅姟鍣?骞惰缃甊eceive浣滀负鍥炶皟鍑芥暟
  Mqtt_Client.setServer(Mqtt_ServerAddress, 8883);
  Mqtt_Connect();
  Mqtt_Client.setCallback(Receive);
  
  //閰嶇疆Seror_Pin涓烘ā鎷熻緭鍏ュ紩鑴氾紝骞惰缃墍鏈堿D1閫氶亾琛板噺鍊间负鏈€澶?
  pinMode(Sersor_Pin_Temp, INPUT);
  pinMode(Sersor_Pin_Humi, INPUT);
  analogSetAttenuation(ADC_ATTENDB_MAX);
  pinMode(1, OUTPUT);  
}


void loop() 
{
  //璇诲彇浼犳劅鍣ㄦ暟鎹?
  SersorValue_Temp = Value_Filter(Sersor_Pin_Temp);
  Serial.print("Get SersorValue_Temp : ");
  Serial.println(SersorValue_Temp);
  SersorValue_Humi = Value_Filter(Sersor_Pin_Humi);
  Serial.print("Get SersorValue_Humi : ");
  Serial.println(SersorValue_Humi);
  delay(10);

  Serial.print("....");
  
  //姣?0s杩涜涓€娆℃暟鎹笂鎶?
  if (Timer_Value >= 2) 
  {
    Report_Data();    
    Timer_Value = 0;  
  }

  Mqtt_Client.loop();

  delay(1000);
}

int myFunction(int x, int y) {
  return x + y;
}

//杩炴帴mqtt鏈嶅姟绔苟璁㈤槄涓婚
void Mqtt_Connect()
{
  
  String clientId = "esp32-" + WiFi.macAddress();
  
  // 杩炴帴MQTT鏈嶅姟鍣?
  if (Mqtt_Client.connect(clientId.c_str())) 
  { 
    Serial.println("MQTT Server Connected.");
    Serial.println("Server Address: ");
    Serial.println(Mqtt_ServerAddress);
    Serial.println("ClientId:");
    Serial.println(clientId);
    Mqtt_Client.subscribe(R_Topic);
  } 
  else 
  {
    Serial.print("MQTT Server Connect Failed. Client State:");
    Serial.println(Mqtt_Client.state());
    delay(3000);
  }   
}

//Wifi杩炴帴
void WiFi_Connect()
{
  WiFi.begin(Wifi_Name, Wifi_Passward);

  while(WiFi.status() != WL_CONNECTED)
  {
    Serial.print("...");
    delay(100);   
  }

  Serial.print(" ");
  Serial.println("Wifi Connected");  
}

//瀵硅鍙栫殑浼犳劅鍣ˋD鍊艰繘琛屼腑鍊兼护娉?
int Value_Filter(int Sersor_Pin)
{
 int Value[9] = {0};
 int i,j,temp;
 for (i = 0; i < 9; i++)
 {
  Value[i] = analogRead(Sersor_Pin);
  delay(10);
 }

 for (i = 0; i < 9; i++)  
 {
  for (j = i; j < 9; j++)
  {
    if (Value[i] > Value[j])
    {
      temp = Value[i];
      Value[i] = Value[j];
      Value[j] = temp;
    }
  }
 }

 return Value[4];
}

hw_timer_t* timer= NULL; 

void IRAM_ATTR Ontime()//姣忕瑙﹀彂涓€娆℃鍑芥暟
{
  Timer_Value++;
}

//鍒濆鍖栧畾鏃跺櫒
void Timer_Init()
{
  timer = timerBegin(0, 800, true);
  timerAttachInterrupt(timer, Ontime, true);
  timerAlarmWrite(timer, 1000000, true);
  timerAlarmEnable(timer);
}

//杩炴帴Mqtt骞朵笂鎶ユ暟鎹?
void Report_Data()
{
  if (Mqtt_Client.connected())
  { 
    String Topic = "Temp-Humi " + WiFi.macAddress();
    char Publish_Topic[Topic.length() + 1];
    strcpy(Publish_Topic, Topic.c_str());
    
    String Payload = "Temp : " + String(SersorValue_Temp) + "  Humi : " + String(SersorValue_Humi);
    char Publish_Massage[Payload.length() + 1];
    strcpy(Publish_Massage, Payload.c_str());
    
    if(Mqtt_Client.publish(Publish_Topic, Publish_Massage))
    {
     Serial.print("Publish Data Temp-Humi : ");
     Serial.print(SersorValue_Temp);
     Serial.print("  ");
     Serial.println(SersorValue_Humi);
     Serial.println("Publish over");
    }
    else
    {
      Serial.println("publish failed !!!!");
    }
  }
  else
  {
    Mqtt_Connect();
  }
}

void Receive(char *Topic, byte *Payload, unsigned int Length)
{
    Serial.print("R_Topic : [ ");
    Serial.print(Topic);
    Serial.println(" ]");
    Serial.print("Payload : ");
    for (int i = 0; i < Length; i++)
    {
      Serial.print((char)Payload[i]);   
    }

}
