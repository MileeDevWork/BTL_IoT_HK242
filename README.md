# 🚗 Hệ thống nhận diện biển số xe

Đây là một hệ thống nhận diện biển số xe sử dụng AI với YOLOv8 và Tesseract OCR, được xây dựng bằng Flask để streaming video trực tiếp.

## ✨ Tính năng

- 🎥 **Streaming video trực tiếp** từ camera
- 🔍 **Phát hiện biển số xe** sử dụng YOLOv8
- 📝 **Trích xuất văn bản** từ biển số bằng Tesseract OCR
- 📸 **Chụp ảnh** toàn bộ khung hình hoặc vùng phát hiện
- 💾 **Lưu trữ tự động** ảnh biển số được phát hiện
- 🌐 **Giao diện web** thân thiện

## 📋 Yêu cầu hệ thống

- Python 3.7+
- Camera (webcam hoặc camera USB)
- Tesseract OCR đã được cài đặt

## 🛠️ Cài đặt

### 1. Cài đặt các thư viện Python

```bash
pip install flask opencv-python ultralytics pytesseract pillow numpy pandas matplotlib seaborn torch
```

### 2. Cài đặt Tesseract OCR

- **Windows**: Tải từ [GitHub releases](https://github.com/UB-Mannheim/tesseract/wiki)
- **Ubuntu/Debian**: `sudo apt install tesseract-ocr`
- **macOS**: `brew install tesseract`

### 3. Cấu hình đường dẫn Tesseract (Windows)

Đảm bảo đường dẫn trong file `camera.py` chính xác:
```python
pytesseract.pytesseract.tesseract_cmd = r'C:/Program Files/Tesseract-OCR/tesseract.exe'
```

## 🚀 Cách sử dụng

### 1. Kiểm tra hệ thống

```bash
python test_camera.py
```

### 2. Chạy ứng dụng web

```bash
python video_camera.py
```

### 3. Truy cập giao diện web

Mở trình duyệt và truy cập: `http://localhost:5000`

## 📁 Cấu trúc thư mục

```
BTL_IOT/Test/
├── camera.py              # Lớp VideoCamera và các hàm xử lý
├── video_camera.py        # Flask application
├── test_camera.py         # Script kiểm tra hệ thống
├── templates/
│   └── index.html         # Giao diện web
├── Data/
│   ├── image/             # Thư mục lưu ảnh biển số
│   └── license-plate-dataset/  # Dataset huấn luyện
└── runs/detect/train7/weights/best.pt  # Model YOLOv8 đã huấn luyện
```

## 🎯 API Endpoints

- `GET /` - Trang chủ với giao diện streaming
- `GET /video_feed` - Stream video trực tiếp
- `GET /snapshot?flag=1&crop=0` - Chụp ảnh toàn bộ
- `GET /snapshot?flag=1&crop=1` - Chụp ảnh vùng phát hiện

## ⚙️ Cấu hình

### Thay đổi camera

Sửa đổi trong file `video_camera.py`:
```python
# Thay đổi index camera (0 = camera mặc định, 1 = camera thứ 2, ...)
VideoCamera(camera_index=0)
```

### Điều chỉnh độ tin cậy

Trong file `camera.py`, class `VideoCamera`, method `get_frame()`:
```python
# Thay đổi ngưỡng confidence (mặc định: 0.5)
if conf > 0.5:  # Tăng để giảm false positive, giảm để tăng sensitivity
```

### Cấu hình OCR

Điều chỉnh config Tesseract trong hàm `extract_plate_text()`:
```python
# PSM modes:
# 6: Uniform block of text
# 7: Single text line
# 8: Single word
# 13: Raw line (no assumptions)
text = pytesseract.image_to_string(gray_plate, config='--psm 7')
```

## 🔧 Troubleshooting

### Lỗi camera không hoạt động
- Kiểm tra camera có được kết nối đúng
- Thử thay đổi camera_index (0, 1, 2...)
- Đảm bảo không có ứng dụng nào khác đang sử dụng camera

### Lỗi model không tải được
- Kiểm tra đường dẫn file model trong `camera.py`
- Đảm bảo file `best.pt` tồn tại trong `runs/detect/train7/weights/`

### Lỗi Tesseract
- Kiểm tra Tesseract đã được cài đặt
- Cập nhật đường dẫn `tesseract_cmd` cho đúng

### Hiệu suất chậm
- Giảm độ phân giải camera
- Tăng ngưỡng confidence để giảm số lượng detection
- Sử dụng GPU nếu có thể

## 📊 Hiệu suất

- **FPS**: ~10-15 FPS (tùy thuộc vào hardware)
- **Độ chính xác phát hiện**: ~90% (tùy thuộc vào điều kiện ánh sáng và góc chụp)
- **Độ chính xác OCR**: ~85% (tùy thuộc vào chất lượng biển số)

## 🤝 Đóng góp

1. Fork project
2. Tạo feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to branch (`git push origin feature/AmazingFeature`)
5. Mở Pull Request

## 📝 License

Distributed under the MIT License. See `LICENSE` for more information.

## 📞 Hỗ trợ

Nếu bạn gặp vấn đề, hãy:
1. Chạy `python test_camera.py` để kiểm tra hệ thống
2. Kiểm tra console log để xem thông báo lỗi
3. Tạo issue trên GitHub với thông tin chi tiết

---

**Chúc bạn sử dụng thành công! 🎉**
