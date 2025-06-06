# 🎯 Hệ Thống Kiểm Soát Truy Cập Tích Hợp

Hệ thống kết hợp **RFID MQTT Access Control** và **License Plate Detection** sử dụng camera để tạo một giải pháp kiểm soát truy cập hoàn chỉnh.

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


