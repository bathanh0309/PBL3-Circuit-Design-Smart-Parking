#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>


// Định nghĩa kích thước màn hình OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
// Khởi tạo đối tượng OLED
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


// Định nghĩa chân
#define  DC 2   // động cơ
#define  BUTTON_DC 4 // nút động cơ
#define  AC 12          // đèn
#define  BUTTON_AC 13   // nút đèn


//cảm biến
#define Light 14      // cảm biến ánh sáng
#define Flame 15      // cảm biến lửa


// Biến lưu trạng thái cảm biến
#define cambienxe1 32 // Chân cảm biến xe 1
#define cambienxe2 35 // Chân cảm biến xe 2
#define cambienxe3 34 // Chân cảm biến xe 3


int xe1 = 0;
int xe2 = 0;
int xe3 = 0;


bool ACState = false;           // Trạng thái đèn/motor
bool lastButtonACState = HIGH;  // Trạng thái nút trước đó
bool overrideACSensor = false;  // Biến để xác định có bỏ qua cảm biến hay không
int lastLightState = HIGH;      // Lưu trạng thái cảm biến trước đó


bool DCState = false;           // Trạng thái đèn/motor
bool lastButtonDCState = HIGH;  // Trạng thái nút trước đó
bool overrideDCSensor = false;  // Biến để xác định có bỏ qua cảm biến hay không
int lastFlameState = HIGH;      // Lưu trạng thái cảm biến trước đó




void setup() {
  Serial.begin(115200);
 
  // Khởi tạo các chân cảm biến và động cơ
  pinMode(cambienxe1, INPUT);
  pinMode(cambienxe2, INPUT);
  pinMode(cambienxe3, INPUT);


  pinMode(DC, OUTPUT);
  pinMode(BUTTON_DC, INPUT_PULLUP);
  digitalWrite(DC, digitalRead(Flame));         // motor bật hay tắt lúc đầu là do cảm biến 

  pinMode(AC, OUTPUT);
  pinMode(BUTTON_AC, INPUT_PULLUP);
  digitalWrite(AC, digitalRead(Light));        // đèn bật hay tắt lúc đầu là do cảm biến 


  pinMode(Flame, INPUT_PULLUP);
  pinMode(Light, INPUT_PULLUP);


  // Khởi động màn hình OLED
    Wire.begin(21, 22);
    display.begin(0x3C, true);          // Khởi tạo với địa chỉ I2C 0x3C
    display.clearDisplay();             // Xóa bộ đệm hiển thị
    display.setTextSize(0.7);             // Đặt kích thước chữ
    display.setTextColor(SH110X_WHITE); // Đặt màu chữ


    // Hiển thị thông điệp chào mừng trên OLED
    display.setCursor(0, 0);
    display.println("Hello, PBL3");
    display.setCursor(0, 10);
    display.println("ESP32 & OLED");
    display.display(); // Cập nhật hiển thị
    delay(2000);       // Đợi 2 giây trước khi tiếp tục
   
    Serial.println("Đã chạy xong hàm setup");
}


void loop() {
  DongcoDC();
  DongcoAC();
  oled();
  Serial.println("Đang loop");
}


void oled(){
    // Đọc trạng thái cảm biến
  xe1 = digitalRead(cambienxe1);
  xe2 = digitalRead(cambienxe2);
  xe3 = digitalRead(cambienxe3);
 
  // Hiển thị thông điệp chào mừng
  display.clearDisplay();
  display.setCursor(30, 0);
  display.println("Welcome to DUT");


  // Hiển thị trạng thái các vị trí đỗ xe
if (xe1 == HIGH) {
    display.setCursor(30, 10);
    display.print("Slot1: Khong xe");
} else {
    display.setCursor(30, 10);
    display.print("Slot1: Co xe");
    delay(1000); // Thêm độ trễ nếu có xe
}


if (xe2 == HIGH) {
    display.setCursor(30, 20);
    display.print("Slot2: Khong xe");
} else {
    display.setCursor(30, 20);
    display.print("Slot2: Co xe");
    delay(1000); // Thêm độ trễ nếu có xe
}


if (xe3 == HIGH) {
    display.setCursor(30, 30);
    display.print("Slot3: Khong xe");
} else {
    display.setCursor(30, 30);
    display.print("Slot3: Co xe");
    delay(1000); // Thêm độ trễ nếu có xe
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




void DongcoAC() {
  // Đọc trạng thái nút nhấn đèn
  bool currentButtonACState = digitalRead(BUTTON_AC);
  // Đọc trạng thái cảm biến ánh sáng
  int currentLightState = digitalRead(Light);


  // Khi nút nhấn được nhấn (nhấn vào = LOW)
  if (lastButtonACState == HIGH && currentButtonACState == LOW) {
    overrideACSensor = true;       // Ưu tiên nút nhấn, bỏ qua cảm biến
    ACState = !ACState;          // Đảo trạng thái đèn/motor
    digitalWrite(AC, ACState ? HIGH : LOW);
  }
  lastButtonACState = currentButtonACState;


  // Kiểm tra nếu cảm biến thay đổi trạng thái
  if (currentLightState != lastLightState) {
    overrideACSensor = false;      // Khôi phục quyền điều khiển của cảm biến
    lastLightState = currentLightState; // Cập nhật trạng thái cảm biến
  }
  // Nếu không bị nút nhấn ưu tiên, dùng tín hiệu từ cảm biến
  if (!overrideACSensor) {
    if (currentLightState == LOW) {
      digitalWrite(AC, LOW);     // Cảm biến light tắt đèn
      ACState = false;           // Cập nhật trạng thái đèn
    } else {
      digitalWrite(AC, HIGH);    // Cảm biến Flame bật đèn
      ACState = true;            // Cập nhật trạng thái đèn
    }
    delay(10);
  }
}


void DongcoDC() {
  // Đọc trạng thái nút nhấn
  bool currentButtonDCState = digitalRead(BUTTON_DC);
  // Đọc trạng thái cảm biến Flame
  int currentFlameState = digitalRead(Flame);


  // Khi nút nhấn được nhấn (nhấn vào = LOW)
  if (lastButtonDCState == HIGH && currentButtonDCState == LOW) {
    overrideDCSensor = true;       // Ưu tiên nút nhấn, bỏ qua cảm biến
    DCState = !DCState;          // Đảo trạng thái đèn/motor
    digitalWrite(DC, DCState ? HIGH : LOW);
  }
  lastButtonDCState = currentButtonDCState;


  // Kiểm tra nếu cảm biến thay đổi trạng thái
  if (currentFlameState != lastFlameState) {
    overrideDCSensor = false;      // Khôi phục quyền điều khiển của cảm biến
    lastFlameState = currentFlameState; // Cập nhật trạng thái cảm biến
  }


  // Nếu không bị nút nhấn ưu tiên, dùng tín hiệu từ cảm biến
  if (!overrideDCSensor) {
    if (currentFlameState == LOW) {
      digitalWrite(DC, LOW);     // Cảm biến motor tắt
      DCState = false;           // Cập nhật trạng thái motor
    } else {
      digitalWrite(DC, HIGH);    // Cảm biến motor bật 
      DCState = true;            // Cập nhật trạng thái motor
    }
    delay(10);
  }
}
