from flask import Flask, render_template, Response, request, abort, jsonify
from camera import VideoCamera
import threading
import time
import cv2
import json
import os
from datetime import datetime

# Import RFID MQTT components
from rfid_mqtt_server import RFIDMQTTServer
from whitelist_db import WhitelistDB

app = Flask(__name__)

# Global instances
global_camera = None
rfid_server = None
camera_lock = threading.Lock()
rfid_lock = threading.Lock()

def get_global_camera():
    """Get or create global camera instance"""
    global global_camera
    with camera_lock:
        if global_camera is None:
            global_camera = VideoCamera()
            # Initialize the camera connection
            global_camera.open_camera()
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

# --- Enhanced snapshot endpoint ---
@app.route("/snapshot")
def snapshot():
    flag = request.args.get("flag", default=0, type=int)
    crop = request.args.get("crop", default=1, type=int)  # Mặc định là cắt hình ảnh
    
    if flag != 1:
        abort(400, "Snapshot flag must be 1")
    
    # Sử dụng camera global thay vì tạo mới
    camera = get_global_camera()
    
    # Chụp ảnh với tính năng lưu biển số
    with camera_lock:
        frame = camera.get_frame(save_plate=True, crop_vehicle=(crop == 1))
    
    if not frame:
        abort(500, "Failed to capture image")
    
    print("📸 Snapshot captured successfully! Camera continues running...")
    return Response(frame, mimetype="image/jpeg")

# --- Status endpoint ---
@app.route("/status")
def status():
    camera = get_global_camera()
    status_info = {
        "camera_open": camera.is_open,
        "camera_index": camera.camera_index,
        "message": "Camera is running and ready for snapshots!"
    }
    return status_info

# Add RFID management routes
@app.route("/rfid/status")
def rfid_status():
    """Trạng thái RFID MQTT Server"""
    global rfid_server
    if rfid_server:
        return jsonify({
            "rfid_running": rfid_server.is_running,
            "mqtt_broker": "test.mosquitto.org",
            "mqtt_port": 1883,
            "subscribe_topic": "yolouno/rfid/scan",
            "publish_topic": "yolouno/rfid/response"
        })
    else:
        return jsonify({"rfid_running": False, "message": "RFID server not initialized"})

@app.route("/rfid/whitelist")
def get_whitelist():
    """Lấy danh sách thẻ cho phép"""
    try:
        db = WhitelistDB()
        cards = db.get_all_cards()
        return jsonify({
            "success": True,
            "total": len(cards),
            "cards": [
                {
                    "uid": card["uid"],
                    "name": card["name"],
                    "department": card["department"],
                    "created_at": card["created_at"].isoformat() if card.get("created_at") else None
                }
                for card in cards
            ]
        })
    except Exception as e:
        return jsonify({"success": False, "error": str(e)}), 500

@app.route("/rfid/add_card", methods=["POST"])
def add_card():
    """Thêm thẻ mới vào whitelist"""
    try:
        data = request.get_json()
        uid = data.get("uid", "").strip()
        name = data.get("name", "").strip()
        department = data.get("department", "Unknown").strip()
        
        if not uid or not name:
            return jsonify({"success": False, "error": "UID và tên không được để trống"}), 400
        
        db = WhitelistDB()
        success = db.add_card(uid, name, department)
        
        if success:
            return jsonify({"success": True, "message": f"Đã thêm thẻ {uid} - {name}"})
        else:
            return jsonify({"success": False, "error": "UID đã tồn tại"}), 400
            
    except Exception as e:
        return jsonify({"success": False, "error": str(e)}), 500

@app.route("/rfid/remove_card", methods=["POST"])
def remove_card():
    """Xóa thẻ khỏi whitelist"""
    try:
        data = request.get_json()
        uid = data.get("uid", "").strip()
        
        if not uid:
            return jsonify({"success": False, "error": "UID không được để trống"}), 400
        
        db = WhitelistDB()
        success = db.remove_card(uid)
        
        if success:
            return jsonify({"success": True, "message": f"Đã xóa thẻ {uid}"})
        else:
            return jsonify({"success": False, "error": "Không tìm thấy thẻ"}), 400
            
    except Exception as e:
        return jsonify({"success": False, "error": str(e)}), 500

@app.route("/rfid/test_uid", methods=["POST"])
def test_uid():
    """Test UID để kiểm tra quyền truy cập"""
    try:
        data = request.get_json()
        uid = data.get("uid", "").strip()
        
        if not uid:
            return jsonify({"success": False, "error": "UID không được để trống"}), 400
        
        db = WhitelistDB()
        result = db.check_uid_allowed(uid)
        
        return jsonify({
            "success": True,
            "uid": uid,
            "allowed": result["allowed"],
            "name": result.get("name"),
            "department": result.get("department"),
            "message": result["message"]
        })
        
    except Exception as e:
        return jsonify({"success": False, "error": str(e)}), 500

def start_rfid_server():
    """Khởi động RFID MQTT Server trong thread riêng"""
    global rfid_server
    try:
        with rfid_lock:
            if rfid_server is None:
                rfid_server = RFIDMQTTServer()
                print("🚀 Đang khởi động RFID MQTT Server...")
                
                # Khởi động RFID server trong thread riêng để không block Flask
                def run_rfid_server():
                    try:
                        rfid_server.start()
                    except Exception as e:
                        print(f"❌ Lỗi RFID Server thread: {e}")
                
                rfid_thread = threading.Thread(target=run_rfid_server, daemon=True)
                rfid_thread.start()
                
                # Chờ một chút để RFID server khởi động
                time.sleep(2)
                print("✅ RFID Server thread đã khởi động")
                
    except Exception as e:
        print(f"❌ Lỗi khởi động RFID Server: {e}")

if __name__ == "__main__":
    print("🚀 Starting License Plate Detection Server...")
    print("📸 Snapshot feature: Camera will continue running after snapshots")
    print(f"🌐 HTTP Access at: http://127.0.0.1:5000")
    print("📱 API Endpoints:")
    print("   - GET / : Web interface")
    print("   - GET /video_feed : Live video stream")
    print("   - GET /snapshot?flag=1&crop=1 : Capture snapshot")
    print("   - GET /status : System status")
    print("   - GET /rfid/status : RFID server status")
    print("   - GET /rfid/whitelist : Get whitelist")
    print("   - POST /rfid/add_card : Add card to whitelist")
    print("   - POST /rfid/remove_card : Remove card from whitelist")
    print("   - POST /rfid/test_uid : Test UID access")
    
    # Start RFID server
    start_rfid_server()
    
    # Start Flask app
    app.run(host="0.0.0.0", port=5000, debug=True)
