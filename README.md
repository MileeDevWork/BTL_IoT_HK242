# 🎯 Hệ Thống Kiểm Soát Truy Cập Tích Hợp

Hệ thống kết hợp **RFID MQTT Access Control** và **License Plate Detection** sử dụng camera để tạo một giải pháp kiểm soát truy cập hoàn chỉnh.

## 🏗️ Kiến Trúc Hệ Thống

```
┌─────────────────┐    MQTT     ┌─────────────────┐    HTTP    ┌─────────────────┐
│   RFID Reader   │◄──────────►│  MQTT Broker    │◄─────────►│   Flask Server   │
│   (ESP32/Arduino)│             │ test.mosquitto.org│          │  (video_camera.py)│
└─────────────────┘             └─────────────────┘            └─────────────────┘
                                          │                              │
                                          │                              │
                                          ▼                              ▼
                                ┌─────────────────┐            ┌─────────────────┐
                                │  RFID MQTT      │            │   Camera +      │
                                │   Server        │            │ License Plate   │
                                │                 │            │   Detection     │
                                └─────────────────┘            └─────────────────┘
                                          │                              │
                                          │                              │
                                          ▼                              ▼
                                ┌─────────────────┐            ┌─────────────────┐
                                │   MongoDB       │            │   Snapshot      │
                                │  (Whitelist)    │            │   Storage       │
                                └─────────────────┘            └─────────────────┘
```

## 🚀 Cài Đặt và Khởi Động

### 1. Cài đặt Dependencies

```bash
pip install -r requirements.txt
```

### 2. Khởi động MongoDB (Tùy chọn)

```bash
# Cài đặt MongoDB Community Edition
# Hoặc sử dụng MongoDB Atlas (cloud)
mongod --dbpath ./data
```

### 3. Khởi động Hệ thống

#### Option 1: Sử dụng Startup Script (Khuyên dùng)

```bash
python startup.py
```

Chọn chế độ khởi động:
- **1**: Hệ thống tích hợp (RFID + Camera)
- **2**: Chỉ RFID MQTT Server  
- **3**: Chỉ Camera System
- **4**: Test hệ thống
- **5**: Xem cấu hình

#### Option 2: Khởi động thủ công

```bash
# Khởi động hệ thống tích hợp
python video_camera.py

# Hoặc chỉ RFID server
python rfid_mqtt_server.py
```

## 📡 MQTT Configuration

### Topics
- **Subscribe**: `yolouno/rfid/scan` - Nhận UID từ thiết bị RFID
- **Publish**: `yolouno/rfid/response` - Gửi kết quả xác thực

### Message Format

#### Gửi UID (từ thiết bị RFID):
```json
{
  "uid": "A1B2C3D4",
  "device_id": "RFID_READER_001",
  "timestamp": "2025-05-25T10:30:00"
}
```

#### Nhận Response (từ server):
```json
{
  "uid": "A1B2C3D4",
  "allowed": true,
  "name": "Nguyễn Văn A",
  "department": "IT",
  "message": "Truy cập được phép",
  "timestamp": "2025-05-25T10:30:01",
  "device_id": "RFID_READER_001"
}
```

## 🌐 Web Interface & APIs

### Truy cập Web Interface
- **URL**: http://localhost:5000
- **Video Feed**: http://localhost:5000/video_feed
- **Snapshot**: http://localhost:5000/snapshot?flag=1&crop=1

### REST APIs

#### Camera APIs
```bash
# Lấy trạng thái camera
GET /status

# Chụp ảnh
GET /snapshot?flag=1&crop=1
```

#### RFID APIs
```bash
# Trạng thái RFID server
GET /rfid/status

# Lấy danh sách thẻ
GET /rfid/whitelist

# Thêm thẻ mới
POST /rfid/add_card
Content-Type: application/json
{
  "uid": "NEW12345",
  "name": "Nguyễn Văn X",
  "department": "Marketing"
}

# Xóa thẻ
POST /rfid/remove_card
Content-Type: application/json
{
  "uid": "OLD12345"
}

# Test UID
POST /rfid/test_uid
Content-Type: application/json
{
  "uid": "A1B2C3D4"
}
```

## 🧪 Testing & Demo

### 1. Test Database
```bash
python whitelist_db.py
```

### 2. Demo RFID MQTT
```bash
python rfid_demo.py
```

### 3. Kiểm tra hệ thống
```bash
python startup.py
# Chọn option 4: Test hệ thống
```

## 🗃️ Database Schema

### Whitelist Collection (MongoDB)
```json
{
  "_id": ObjectId("..."),
  "uid": "A1B2C3D4",
  "name": "Nguyễn Văn A",
  "department": "IT",
  "status": "active",
  "created_at": ISODate("2025-05-25T10:00:00Z"),
  "updated_at": ISODate("2025-05-25T10:00:00Z")
}
```

### Access Logs Collection
```json
{
  "_id": ObjectId("..."),
  "uid": "A1B2C3D4",
  "allowed": true,
  "timestamp": ISODate("2025-05-25T10:30:00Z"),
  "additional_info": {
    "device_id": "RFID_READER_001",
    "topic": "yolouno/rfid/scan"
  }
}
```

## ⚙️ Cấu Hình

### mqtt_config.py
```python
# MQTT Configuration
MQTT_BROKER = "test.mosquitto.org"
MQTT_PORT = 1883
TOPIC_SUB = "yolouno/rfid/scan"
TOPIC_PUB = "yolouno/rfid/response"

# MongoDB Configuration  
MONGODB_URI = "mongodb://localhost:27017/"
DATABASE_NAME = "rfid_system"
COLLECTION_NAME = "whitelist"

# Flask Configuration
FLASK_HOST = "127.0.0.1"
FLASK_PORT = 5000
```

## 🔧 Tích Hợp với Thiết Bị RFID

### Arduino/ESP32 Example
```cpp
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <MFRC522.h>

// MQTT settings
const char* mqtt_server = "test.mosquitto.org";
const char* topic_pub = "yolouno/rfid/scan";
const char* topic_sub = "yolouno/rfid/response";

void publishRFID(String uid) {
  StaticJsonDocument<200> doc;
  doc["uid"] = uid;
  doc["device_id"] = "ESP32_READER_001";
  doc["timestamp"] = getISOTimestamp();
  
  String message;
  serializeJson(doc, message);
  
  client.publish(topic_pub, message.c_str());
}

void callback(char* topic, byte* payload, unsigned int length) {
  StaticJsonDocument<300> doc;
  deserializeJson(doc, payload, length);
  
  bool allowed = doc["allowed"];
  String name = doc["name"];
  
  if (allowed) {
    // Mở cửa/LED xanh
    digitalWrite(GREEN_LED, HIGH);
    Serial.println("Access GRANTED for: " + name);
  } else {
    // LED đỏ/buzzer
    digitalWrite(RED_LED, HIGH);
    Serial.println("Access DENIED");
  }
}
```

## 📊 Tính Năng Chính

### 🔐 RFID Access Control
- ✅ Xác thực UID theo whitelist MongoDB
- ✅ MQTT real-time communication
- ✅ Logging tất cả access attempts
- ✅ RESTful API management
- ✅ Web-based whitelist management

### 📸 Camera System  
- ✅ Live video streaming
- ✅ License plate detection với YOLO
- ✅ HTTP snapshot API
- ✅ Automatic image capture khi access granted
- ✅ Cropped vehicle image saving

### 🔗 Integration Features
- ✅ Automatic camera trigger khi RFID granted
- ✅ Unified web interface
- ✅ Combined logging system
- ✅ Single startup script
- ✅ Health monitoring cho tất cả components

## 🚨 Troubleshooting

### Common Issues

#### 1. MongoDB Connection Error
```bash
# Kiểm tra MongoDB service
mongod --version

# Khởi động MongoDB
mongod --dbpath ./data
```

#### 2. Camera Not Found
```bash
# Kiểm tra camera devices
python -c "import cv2; print(cv2.VideoCapture(0).isOpened())"
```

#### 3. MQTT Connection Failed
```bash
# Test MQTT broker
pip install paho-mqtt
python -c "import paho.mqtt.client as mqtt; c=mqtt.Client(); print(c.connect('test.mosquitto.org', 1883, 60))"
```

#### 4. Dependencies Missing
```bash
# Reinstall requirements
pip install -r requirements.txt --force-reinstall
```

## 📝 Logs & Monitoring

### Log Locations
- **Application Logs**: Console output
- **Access Logs**: MongoDB `access_logs` collection
- **Camera Images**: `mqtt_snapshots/` directory
- **Error Logs**: Python logging output

### Health Check URLs
- **System Status**: http://localhost:5000/status
- **RFID Status**: http://localhost:5000/rfid/status
- **Whitelist Count**: http://localhost:5000/rfid/whitelist

## 🔒 Security Considerations

1. **MongoDB**: Sử dụng authentication trong production
2. **MQTT**: Sử dụng SSL/TLS và authentication
3. **Flask**: Thêm authentication cho web interface
4. **Network**: Firewall rules cho MQTT port 1883
5. **Camera**: Secure camera feed access

## 📞 Support

Nếu gặp vấn đề, hãy:
1. Chạy `python startup.py` → option 4 (Test hệ thống)
2. Kiểm tra logs trong console
3. Verify dependencies với `pip list`
4. Test từng component riêng biệt

---

## 📋 Checklist Triển Khai

- [ ] Cài đặt Python 3.7+
- [ ] Cài đặt MongoDB (hoặc cấu hình MongoDB Atlas)
- [ ] Clone/download source code
- [ ] Chạy `pip install -r requirements.txt`
- [ ] Cấu hình `mqtt_config.py` nếu cần
- [ ] Test camera với `python -c "import cv2; cv2.VideoCapture(0).read()"`
- [ ] Khởi động hệ thống với `python startup.py`
- [ ] Test RFID với `python rfid_demo.py`
- [ ] Verify web interface tại http://localhost:5000

🎉 **Hệ thống sẵn sàng hoạt động!**
