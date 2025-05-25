from flask import Flask, render_template, Response, request, abort  # thêm request, abort
from camera import VideoCamera  # Import VideoCamera class
import threading
import time
import cv2

app = Flask(__name__)

# Global camera instance for sharing between video feed and snapshot
global_camera = None
camera_lock = threading.Lock()

def get_global_camera():
    """Get or create global camera instance"""
    global global_camera
    with camera_lock:
        if global_camera is None:
            global_camera = VideoCamera()
            # Initialize the camera connection
            global_camera.cap = cv2.VideoCapture(global_camera.camera_index)
        return global_camera

@app.route("/")
def index():
    return render_template("index.html")

def gen(camera: VideoCamera):
    while True:
        with camera_lock:
            # Đặt save_plate=False để không lưu biển số khi chỉ hiển thị video
            frame = camera.get_frame(save_plate=False)
        if frame is None:
            time.sleep(0.1)  # Ngủ ngắn để tránh tốn CPU
            continue
        yield (
            b"--frame\r\n"
            b"Content-Type: image/jpeg\r\n\r\n" + frame + b"\r\n\r\n"
        )

@app.route("/video_feed")
def video_feed():
    camera = get_global_camera()
    return Response(
        gen(camera),
        mimetype="multipart/x-mixed-replace; boundary=frame",
    )

# --- thêm đoạn code này ---
@app.route("/snapshot")
def snapshot():
    flag = request.args.get("flag", default=0, type=int)
    crop = request.args.get("crop", default=1, type=int)  # Mặc định là cắt hình ảnh
    if flag != 1:
        abort(400, "Snapshot flag must be 1")
    
    # Sử dụng camera global thay vì tạo mới
    camera = get_global_camera()
    
    with camera_lock:
        # Truyền flag=1 để lưu hình ảnh biển số khi chụp
        # Nếu crop=1, trả về hình ảnh đã cắt
        frame = camera.get_frame(save_plate=True, crop_vehicle=(crop == 1))
    
    if not frame:
        abort(500, "Failed to capture image")
    
    print("📸 Snapshot captured successfully! Camera continues running...")
    return Response(frame, mimetype="image/jpeg")
# --- end thêm ---

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)