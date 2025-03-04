#include <Arduino.h>
#include <WiFi.h> // Sử dụng WiFiClient cho kết nối không bảo mật
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

// Thông tin Wi-Fi và MQTT broker
#define ssid "Hieu_TkZ"
#define password "123123123"

// Thông tin MQTT broker
const char *mqtt_server = "broker.emqx.io";
#define mqtt_port 1883
#define mqtt_username "hieutk1302"
#define mqtt_password "hieutk1302"

WiFiClient espClient; // Sử dụng WiFiClient cho kết nối không bảo mật
PubSubClient client(espClient);

bool isLoginProcessed = false; // Biến trạng thái đã xử lý login

// Định nghĩa kích thước màn hình OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
// Khởi tạo đối tượng OLED
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Định nghĩa chân
#define DC 16        // động cơ
#define BUTTON_DC 4  // nút động cơ
#define AC 12        // đèn
#define BUTTON_AC 13 // nút đèn

// cảm biến
#define Light 14 // cảm biến ánh sáng
#define Flame 26 // cảm biến lửa

// Biến lưu trạng thái cảm biến
#define cambienxe1 32 // Chân cảm biến xe 1
#define cambienxe2 33 // Chân cảm biến xe 2
#define cambienxe3 25 // Chân cảm biến xe 3

int xe1 = 0;
int xe2 = 0;
int xe3 = 0;

bool ACState = false;          // Trạng thái đèn/motor
bool lastButtonACState = HIGH; // Trạng thái nút trước đó
bool overrideACSensor = false; // Biến để xác định có bỏ qua cảm biến hay không
int lastLightState = HIGH;     // Lưu trạng thái cảm biến trước đó

bool DCState = false;          // Trạng thái đèn/motor
bool lastButtonDCState = HIGH; // Trạng thái nút trước đó
bool overrideDCSensor = false; // Biến để xác định có bỏ qua cảm biến hay không
int lastFlameState = HIGH;     // Lưu trạng thái cảm biến trước đó

// Khai báo hàm
void connectWiFi();
void reconnect();
void callback(char *topic, byte *payload, unsigned int length);
void oled();
void parking();
void DongcoAC();
void DongcoDC();

void setup()
{
  Serial.begin(115200);
  connectWiFi(); // Kết nối Wi-Fi
  client.setServer(mqtt_server, mqtt_port);
  Serial.println("setServer completed !");
  client.setCallback(callback); // Thiết lập callback để nhận tin từ MQTT

  // Khởi tạo các chân cảm biến và động cơ
  pinMode(cambienxe1, INPUT);
  pinMode(cambienxe2, INPUT);
  pinMode(cambienxe3, INPUT);

  pinMode(DC, OUTPUT);
  pinMode(BUTTON_DC, INPUT_PULLUP);
  digitalWrite(DC, digitalRead(Flame)); // motor bật hay tắt lúc đầu là do cảm biến

  pinMode(AC, OUTPUT);
  pinMode(BUTTON_AC, INPUT_PULLUP);
  digitalWrite(AC, digitalRead(Light)); // đèn bật hay tắt lúc đầu là do cảm biến

  pinMode(Flame, INPUT_PULLUP);
  pinMode(Light, INPUT_PULLUP);

  // Khởi động màn hình OLED
  Wire.begin(21, 22);
  display.begin(0x3C, true);          // Khởi tạo với địa chỉ I2C 0x3C
  display.clearDisplay();             // Xóa bộ đệm hiển thị
  display.setTextSize(0.7);           // Đặt kích thước chữ
  display.setTextColor(SH110X_WHITE); // Đặt màu chữ

  // Hiển thị thông điệp chào mừng trên OLED
  display.setCursor(0, 0);
  display.println("Hello, PBL3");
  display.setCursor(0, 10);
  display.println("ESP32 & OLED");
  display.display(); // Cập nhật hiển thị
  delay(2000);       // Đợi 2 giây trước khi tiếp tục
}

void loop() {
  if (!client.connected()) {
    Serial.println("Connecting to broker...");
    reconnect();
  }
  client.loop(); // Giữ kết nối với broker


  // Gửi trạng thái bãi đỗ xe liên tục mỗi 2 giây
  static unsigned long lastTime = 0;
  unsigned long currentTime = millis();
 
  if (currentTime - lastTime >= 2000) {  // Mỗi 2 giây gửi một lần
    parking();
    client.publish("esp32/motorState", DCState ? "On" : "Off");
    client.publish("esp32/ledState", ACState ? "Off" : "On");
    lastTime = currentTime;  // Cập nhật thời gian gửi tiếp theo
  }


  DongcoDC();
  DongcoAC();
  oled();
}



void callback(char *topic, byte *payload, unsigned int length) {
    String msg = "";
    for (int i = 0; i < length; i++) {
        msg += (char)payload[i]; // Chuyển payload thành chuỗi
    }

    Serial.print("Tin nhắn nhận từ topic ");
    Serial.print(topic);
    Serial.print(": ");
    Serial.println(msg);

    // Xử lý từng topic
    if (strcmp(topic, "web/ledControl") == 0) {
        if (msg == "Turn on") {
            ACState = true;
            digitalWrite(AC, HIGH); // Bật đèn
            overrideACSensor = true;
        } else if (msg == "Turn off") {
            ACState = false;
            digitalWrite(AC, LOW); // Tắt đèn
            overrideACSensor = true;
        }
        client.publish("esp32/ledState", ACState ? "On" : "Off");

    } else if (strcmp(topic, "web/motorControl") == 0) {
        if (msg == "Turn on") {
            DCState = true;
            digitalWrite(DC, HIGH); // Bật máy bơm
            overrideDCSensor = true;
        } else if (msg == "Turn off") {
            DCState = false;
            digitalWrite(DC, LOW); // Tắt máy bơm
            overrideDCSensor = true;
        }
        client.publish("esp32/motorState", DCState ? "On" : "Off");

      }else if (strcmp(topic, "web/Logout") == 0) {
      if (msg == "True") {
        isLoginProcessed = false;
        Serial.println("Logout successful, isLoginProcessed reset.");
    }

        
    } else if (strcmp(topic, "web/Login") == 0) {
        if (!isLoginProcessed) {
            if (msg == "admin|admin") {
                client.publish("web/Login", "true");  // Trả kết quả đúng
            } else {
                client.publish("web/Login", "false"); // Trả kết quả sai
            }
            isLoginProcessed = true; // Đánh dấu đã xử lý
        }

    } 
}

// Hàm kết nối với mqtt broker
void reconnect()
{
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    String clientID = "ESPClient-";
    clientID += String(random(0xffff), HEX);

    // Kết nối với broker MQTT với username và password
    if (client.connect(clientID.c_str(), mqtt_username, mqtt_password))
    {
      Serial.println("connected MQTT");

      client.subscribe("esp32/ledState");
      Serial.println("connected topic esp32/ledState");

      client.subscribe("esp32/motorState");
      Serial.println("connected topic esp32/motorState");

      client.subscribe("web/ledControl");
      Serial.println("connected topic web/ledControl");

      client.subscribe("esp32/parkingStatus");
      Serial.println("connected topic esp32/parkingStatus"); // đăng ký topic để gửi trạng thái ô đậu xe

      client.subscribe("web/motorControl");
      Serial.println("connected topic web/motorControl");

      client.subscribe("web/Login");
      client.subscribe("web/Logout");
      Serial.println("connected topic web/Login");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

// Hàm kết nối Wi-Fi
void connectWiFi()
{
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi..");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi !");
}
// Hàm Oled
void oled(){
// Đọc trạng thái cảm biến
xe1 = digitalRead(cambienxe1);
xe2 = digitalRead(cambienxe2);
xe3 = digitalRead(cambienxe3);

// Hiển thị thông điệp chào mừng
display.clearDisplay();
display.setCursor(30, 0);
display.println("Welcome to DUT");

// Mảng chứa trạng thái và vị trí
int xe[] = {xe1, xe2, xe3};
const char* slots[] = {"Slot1", "Slot2", "Slot3"};
const char* statuses[] = {"Co xe", "Khong xe"};

for (int i = 0; i < 3; i++) {
    display.setCursor(30, 10 + (i * 10));
    display.print(slots[i]);
    display.print(": ");
    display.println(statuses[xe[i]]);
}

// Kiểm tra nếu tất cả vị trí đều có xe
if (xe1 == LOW && xe2 == LOW && xe3 == LOW) {
    display.clearDisplay();
    display.setCursor(30, 20);
    display.println("Parking Full");
    display.setCursor(30, 40);
    display.println("See you again");
}

// Cập nhật màn hình OLED
display.display();
// Thêm một khoảng thời gian để giảm tần suất đọc
delay(100);
}

void parking() {
  int giaTri1 = digitalRead(cambienxe1);
  int giaTri2 = digitalRead(cambienxe2);
  int giaTri3 = digitalRead(cambienxe3);

    client.publish("esp32/parkingStatus", giaTri1 == HIGH ? "Slot1:Khong xe" : "Slot1:Co xe");
    client.publish("esp32/parkingStatus", giaTri2 == HIGH ? "Slot2:Khong xe" : "Slot2:Co xe");
    client.publish("esp32/parkingStatus", giaTri3 == HIGH ? "Slot3:Khong xe" : "Slot3:Co xe");
}

void DongcoAC()
{
  // Đọc trạng thái nút nhấn đèn
  bool currentButtonACState = digitalRead(BUTTON_AC);
  // Đọc trạng thái cảm biến ánh sáng
  int currentLightState = digitalRead(Light);

  // Khi nút nhấn được nhấn (nhấn vào = LOW)
  if (lastButtonACState == HIGH && currentButtonACState == LOW)
  {
    overrideACSensor = true; // Ưu tiên nút nhấn, bỏ qua cảm biến
    ACState = !ACState;      // Đảo trạng thái đèn/motor
    digitalWrite(AC, ACState ? LOW : HIGH);
  }
  lastButtonACState = currentButtonACState;

  // Kiểm tra nếu cảm biến thay đổi trạng thái
  if (currentLightState != lastLightState)
  {
    overrideACSensor = false;           // Khôi phục quyền điều khiển của cảm biến
    lastLightState = currentLightState; // Cập nhật trạng thái cảm biến
  }
  // Nếu không bị nút nhấn ưu tiên, dùng tín hiệu từ cảm biến
  if (!overrideACSensor)
  {
    if (currentLightState == HIGH)
    {
      digitalWrite(AC, HIGH); // Cảm biến light tắt đèn
      ACState = false;       // Cập nhật trạng thái đèn
    }
    else
    {
      digitalWrite(AC, LOW); // Cảm biến Flame bật đèn
      ACState = true;         // Cập nhật trạng thái đèn
    }
    delay(10);
  }
}

void DongcoDC()
{
  // Đọc trạng thái nút nhấn
  bool currentButtonDCState = digitalRead(BUTTON_DC);
  // Đọc trạng thái cảm biến Flame
  int currentFlameState = digitalRead(Flame);

  // Khi nút nhấn được nhấn (nhấn vào = LOW)
  if (lastButtonDCState == HIGH && currentButtonDCState == LOW)
  {
    overrideDCSensor = true; // Ưu tiên nút nhấn, bỏ qua cảm biến
    DCState = !DCState;      // Đảo trạng thái đèn/motor
    digitalWrite(DC, DCState ? HIGH : LOW);
  }
  lastButtonDCState = currentButtonDCState;

  // Kiểm tra nếu cảm biến thay đổi trạng thái
  if (currentFlameState != lastFlameState)
  {
    overrideDCSensor = false;           // Khôi phục quyền điều khiển của cảm biến
    lastFlameState = currentFlameState; // Cập nhật trạng thái cảm biến
  }

  // Nếu không bị nút nhấn ưu tiên, dùng tín hiệu từ cảm biến
  if (!overrideDCSensor)
  {
    if (currentFlameState == LOW)
    {
      digitalWrite(DC, LOW); // Cảm biến motor tắt
      DCState = false;       // Cập nhật trạng thái motor
    }
    else
    {
      digitalWrite(DC, HIGH); // Cảm biến motor bật
      DCState = true;         // Cập nhật trạng thái motor
    }
    delay(10);
  }
}

