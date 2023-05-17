#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

int myFunction(int, int);

/*传感器对应引脚和AD值的定义*/
#define Sersor_Pin_Temp 12 //温度传感器的AO引脚接Esp32的12号引脚
#define Sersor_Pin_Humi 13 //湿度传感器的AO引脚接Esp32的13号引脚
int SersorValue_Temp;
int SersorValue_Humi; 
int Timer_Value = 0;

/*wifi名称以及密码*/
const char *Wifi_Name = "Redmi K50";
const char *Wifi_Passward = "xxy031205";

/*mqtt的服务器地址以及端口号, 以及esp32客户端需订阅的主题名*/
const char *Mqtt_ServerAddress = "mqtt.rymcu.com";
const uint16_t Mqtt_Com = 8883;
const char *R_Topic = "Order_esp32_112233";

/*配置CA证书*/
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
int Value_Filter(int i); //读取一连串SersorValue取中值
void Timer_Init(); //定时器初始化
void IRAM_ATTR Ontime(); //定时器中断函数
void Report_Data();//数据上报至mqtt服务端
void Receive(char *Topic, byte *Payload, unsigned int Length);//回调函数

void setup() 
{
  int result = myFunction(2, 3);
  Serial.begin(115200);
  Serial.println("Opening");
  WiFi.mode(WIFI_AP_STA);
  Wifi_Client.setCACert(ca_cert);
  
  Timer_Init();
  
  //连接wifi
  WiFi_Connect();
  //连接mqtt服务器,并设置Receive作为回调函数
  Mqtt_Client.setServer(Mqtt_ServerAddress, 8883);
  Mqtt_Connect();
  Mqtt_Client.setCallback(Receive);
  
  //配置Seror_Pin为模拟输入引脚，并设置所有AD1通道衰减值为最大
  pinMode(Sersor_Pin_Temp, INPUT);
  pinMode(Sersor_Pin_Humi, INPUT);
  analogSetAttenuation(ADC_ATTENDB_MAX);
  pinMode(1, OUTPUT);  
}


void loop() 
{
  //读取传感器数据
  SersorValue_Temp = Value_Filter(Sersor_Pin_Temp);
  Serial.print("Get SersorValue_Temp : ");
  Serial.println(SersorValue_Temp);
  SersorValue_Humi = Value_Filter(Sersor_Pin_Humi);
  Serial.print("Get SersorValue_Humi : ");
  Serial.println(SersorValue_Humi);
  delay(10);

  Serial.print("....");
  
  //每20s进行一次数据上报
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

//连接mqtt服务端并订阅主题
void Mqtt_Connect()
{
  
  String clientId = "esp32-" + WiFi.macAddress();
  
  // 连接MQTT服务器
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

//Wifi连接
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

//对读取的传感器AD值进行中值滤波
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

void IRAM_ATTR Ontime()//每秒触发一次此函数
{
  Timer_Value++;
}

//初始化定时器
void Timer_Init()
{
  timer = timerBegin(0, 800, true);
  timerAttachInterrupt(timer, Ontime, true);
  timerAlarmWrite(timer, 1000000, true);
  timerAlarmEnable(timer);
}

//连接Mqtt并上报数据
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
