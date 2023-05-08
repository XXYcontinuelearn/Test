#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>

/*传感器对应引脚和AD值的定义*/
#define Sersor_Pin_Temp 12 //温度传感器的AO引脚接Esp32的12号引脚
#define Sersor_Pin_Humi 13 //湿度传感器的AO引脚接Esp32的13号引脚
int SersorValue_Temp;
int SersorValue_Humi; 

/*wifi名称以及密码*/
const char *Wifi_Name = "xxx";
const char *Wifi_Passward = "xxx";

/*mqtt的服务器地址以及端口号*/
const char *Mqtt_ServerAddress = "xxx";
const uint16_t Mqtt_Com = 123;

WiFiClient Wifi_Client;
PubSubClient Mqtt_Client(Wifi_Client);

void Mqtt_Connect();
void WiFi_Connect();
int Value_Filter(int i); //读取一连串SersorValue取中值

void setup() 
{
  Serial.begin(9600);
  
  //连接wifi
  WiFi_Connect();
  //连接mqtt服务器
  Mqtt_Client.setServer(Mqtt_ServerAddress, Mqtt_Com);
  Mqtt_Connect();

  
  //配置Seror_Pin为模拟输入引脚，并设置所有AD1通道衰减值为最大
  pinMode(Sersor_Pin_Temp, INPUT);
  analogSetAttenuation(ADC_ATTENDB_MAX);
  
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

  //通过mqtt进行数据上报
  if (Mqtt_Client.connected())
  {
    //发布主题为Topic， 内容为Payload的数据包
    String Topic = "Temp-Humi " + WiFi.macAddress();
    char Publish_Topic[Topic.length() + 1];
    strcpy(Publish_Topic, Topic.c_str());
    
    String Payload = "Temp : " + String(SersorValue_Temp) + "Humi : " + String(SersorValue_Humi);
    char Publish_Massage[Payload.length() + 1];
    strcpy(Publish_Massage, Payload.c_str());
    
    Mqtt_Client.publish(Publish_Topic, Publish_Massage);
    Serial.print("Publish Data Temp-Humi : ");
    Serial.print(SersorValue_Temp);
    Serial.println(SersorValue_Humi);
    Serial.print("Publish over");

    //保持心跳  
    Mqtt_Client.loop();
  }
  else
  {
    Mqtt_Connect();   
  }  

  delay(100);
}

//连接mqtt服务端
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