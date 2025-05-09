# Dự án IoT - Hệ thống giám sát môi trường và bãi đỗ xe

Dự án này sử dụng ESP32 để giám sát các thông số môi trường (nhiệt độ, độ ẩm) và phát hiện vật cản/xe trong bãi đỗ xe, sau đó gửi dữ liệu lên nền tảng ThingsBoard để theo dõi và phân tích.

## Các tính năng chính

- Đọc nhiệt độ và độ ẩm từ cảm biến DHT20
- Phát hiện vật thể/xe sử dụng cảm biến siêu âm HC-SR04
- Gửi dữ liệu lên ThingsBoard qua MQTT
- Quản lý nhiều thiết bị với các token riêng biệt
- Sử dụng FreeRTOS để xử lý đa nhiệm

## Cấu trúc thư mục

```
lib/
  ├── DeviceManager/        # Quản lý các thiết bị ThingsBoard
  ├── DHT20/               # Thư viện cảm biến DHT20
  ├── HCSR04/              # Thư viện cảm biến siêu âm
  ├── Global/              # Khai báo biến toàn cục
  ├── MQTT/                # Kết nối MQTT
  ├── PubSubClient/        # Thư viện MQTT Client
  ├── Sensors/             # Quản lý cảm biến
  ├── ThingsBoard/         # Thư viện ThingsBoard
  └── Wifi/                # Kết nối WiFi
src/
  ├── main.cpp             # File chính cũ
  ├── merge_to_main.cpp    # File phát triển
  └── unified_main.cpp     # File chính đã tổng hợp và cải tiến
```

## Cấu hình phần cứng

### Kết nối ESP32 với các cảm biến

- **DHT20**:
  - SDA - GPIO11
  - SCL - GPIO12

- **HC-SR04**:
  - Trigger - GPIO6
  - Echo - GPIO7

- **LED**:
  - GPIO48

## Cài đặt và Build

1. Clone repository về máy tính
2. Mở project trong PlatformIO
3. Cấu hình các tham số WiFi và ThingsBoard trong file `unified_main.cpp`
4. Build và upload lên ESP32

## Sử dụng

1. Sau khi upload firmware, ESP32 sẽ tự động kết nối vào WiFi và ThingsBoard
2. Dữ liệu nhiệt độ và độ ẩm sẽ được gửi lên ThingsBoard mỗi 10 giây
3. Trạng thái bãi đỗ xe sẽ được gửi lên khi có thay đổi (có xe hoặc không có xe)

## Lưu ý

- Đảm bảo đã cấu hình đúng thông tin WiFi và token ThingsBoard
- Các token trong code là ví dụ, cần thay thế bằng token thực từ ThingsBoard
- Có thể điều chỉnh ngưỡng phát hiện vật thể bằng cách thay đổi giá trị `DETECTION_THRESHOLD`
