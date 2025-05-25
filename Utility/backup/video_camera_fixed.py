# filepath: c:\Users\Dell\Desktop\Study\Allproject\BTL_IOT\Test\video_camera.py
# Enhanced License Plate Detection Server with MQTT Integration
from flask import Flask, render_template, Response, request, abort
from camera import VideoCamera
import threading
import time
import cv2
import paho.mqtt.client as mqtt
import json
import datetime
import os

# Import MQTT configuration
try:
    from mqtt_config import *
except ImportError:
    # Fallback configuration if mqtt_config.py doesn't exist
    MQTT_BROKER = "test.mosquitto.org"
    MQTT_PORT = 1883
    MQTT_USERNAME = ""
    MQTT_PASSWORD = ""
    MQTT_TOPIC_SUBSCRIBE = "yolouno/sensor/nfc/+"
    MQTT_TOPIC_STATUS = "yolouno/camera/status"
    MQTT_SNAPSHOT_DIR = "mqtt_snapshots"
    DEFAULT_CROP = True

app = Flask(__name__)

# Global camera instance for sharing between video feed and snapshot
global_camera = None
camera_lock = threading.Lock()

# MQTT Configuration
MQTT_TOPIC = MQTT_TOPIC_SUBSCRIBE  # For backward compatibility
mqtt_client = None

def get_global_camera():
    """Get or create global camera instance"""
    global global_camera
    with camera_lock:
        if global_camera is None:
            global_camera = VideoCamera()
            # Initialize the camera connection
            global_camera.open_camera()
        return global_camera

def capture_snapshot_via_mqtt(crop=True):
    """Capture snapshot triggered by MQTT message"""
    try:
        camera = get_global_camera()
        
        # Chụp ảnh với tính năng lưu biển số
        with camera_lock:
            frame = camera.get_frame(save_plate=True, crop_vehicle=crop)
        
        if frame:
            # Lưu ảnh với timestamp
            timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
            filename = f"mqtt_snapshot_{timestamp}.jpg"
            
            # Tạo thư mục nếu chưa có
            os.makedirs(MQTT_SNAPSHOT_DIR, exist_ok=True)
            
            # Lưu ảnh
            filepath = os.path.join(MQTT_SNAPSHOT_DIR, filename)
            with open(filepath, "wb") as f:
                f.write(frame)
            
            print(f"📸 MQTT Snapshot saved: {filepath}")
            
            # Gửi thông báo trạng thái qua MQTT
            if mqtt_client and mqtt_client.is_connected():
                status_msg = {
                    "status": "snapshot_captured",
                    "filename": filename,
                    "timestamp": timestamp,
                    "crop": crop
                }
                mqtt_client.publish(MQTT_TOPIC_STATUS, json.dumps(status_msg))
            
            return True
        else:
            print("❌ Failed to capture MQTT snapshot")
            return False
    except Exception as e:
        print(f"❌ Error capturing MQTT snapshot: {e}")
        return False

# MQTT Callback functions
def on_connect(client, userdata, flags, rc):
    """Callback when MQTT client connects"""
    if rc == 0:
        print(f"✅ Connected to MQTT broker: {MQTT_BROKER}:{MQTT_PORT}")
        client.subscribe(MQTT_TOPIC_SUBSCRIBE)
        print(f"📡 Subscribed to topic: {MQTT_TOPIC_SUBSCRIBE}")
        
        # Gửi thông báo camera đã sẵn sàng
        status_msg = {
            "status": "camera_ready",
            "timestamp": datetime.datetime.now().strftime("%Y%m%d_%H%M%S"),
            "message": "License plate detection camera is ready"
        }
        client.publish(MQTT_TOPIC_STATUS, json.dumps(status_msg))
    else:
        print(f"❌ Failed to connect to MQTT broker, return code {rc}")

def on_message(client, userdata, msg):
    """Callback when MQTT message is received"""
    try:
        topic = msg.topic
        payload = msg.payload.decode('utf-8')
        
        print(f"📨 MQTT Message received - Topic: {topic}, Payload: {payload}")
        
        # Kiểm tra topic pattern yolouno/sensor/nfc/+
        if topic.startswith("yolouno/sensor/nfc/"):
            # Parse JSON payload nếu có
            try:
                data = json.loads(payload)
                flag = data.get('flag', 0)
                crop = data.get('crop', 1)
            except json.JSONDecodeError:
                # Nếu không phải JSON, kiểm tra payload đơn giản
                if payload.lower() in ['1', 'true', 'capture', 'snapshot']:
                    flag = 1
                    crop = 1
                else:
                    flag = 0
                    crop = 1
            
            # Kích hoạt chụp ảnh nếu flag = 1
            if flag == 1:
                print(f"🎯 Triggering snapshot via MQTT from topic: {topic}")
                capture_snapshot_via_mqtt(crop=(crop == 1))
            else:
                print(f"⏸️ MQTT message received but flag != 1: {flag}")
                
    except Exception as e:
        print(f"❌ Error processing MQTT message: {e}")

def on_disconnect(client, userdata, rc):
    """Callback when MQTT client disconnects"""
    print(f"🔌 Disconnected from MQTT broker, return code {rc}")

def setup_mqtt():
    """Setup MQTT client with retry mechanism and fallback brokers"""
    global mqtt_client
    
    # Get brokers to try (primary + alternatives)
    brokers_to_try = [(MQTT_BROKER, MQTT_PORT)]
    
    # Add alternative brokers if defined in config
    try:
        from mqtt_config import ALTERNATIVE_BROKERS
        brokers_to_try.extend(ALTERNATIVE_BROKERS)
    except ImportError:
        # Add some common public brokers as fallback
        brokers_to_try.extend([
            ("broker.hivemq.com", 1883),
            ("mqtt.eclipseprojects.io", 1883)
        ])
    
    # Remove duplicates while preserving order
    seen = set()
    unique_brokers = []
    for broker in brokers_to_try:
        if broker not in seen:
            seen.add(broker)
            unique_brokers.append(broker)
    
    print(f"🔄 Trying to connect to MQTT brokers...")
    
    for i, (broker, port) in enumerate(unique_brokers):
        try:
            print(f"🌐 Attempt {i+1}/{len(unique_brokers)}: {broker}:{port}")
            
            # Create new client for each attempt
            mqtt_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1, clean_session=True)
            mqtt_client.on_connect = on_connect
            mqtt_client.on_message = on_message
            mqtt_client.on_disconnect = on_disconnect
            
            # Set credentials if provided
            if MQTT_USERNAME and MQTT_PASSWORD:
                mqtt_client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)
                print("🔐 MQTT credentials set")
            
            # Try to connect with timeout
            try:
                mqtt_client.connect(broker, port, 60)
                mqtt_client.loop_start()
                
                # Wait a bit to see if connection succeeds
                print("⏳ Testing connection...")
                time.sleep(3)
                
                if mqtt_client.is_connected():
                    print(f"✅ Successfully connected to {broker}:{port}")
                    
                    # Update global config to remember working broker
                    global MQTT_BROKER, MQTT_PORT
                    MQTT_BROKER = broker
                    MQTT_PORT = port
                    
                    return True
                else:
                    print(f"❌ Connection to {broker}:{port} failed")
                    mqtt_client.loop_stop()
                    
            except Exception as e:
                print(f"❌ Error connecting to {broker}:{port}: {e}")
                
        except Exception as e:
            print(f"❌ Failed to setup client for {broker}:{port}: {e}")
    
    print("❌ All MQTT broker attempts failed")
    print("💡 Possible solutions:")
    print("   1. Check internet connection")
    print("   2. Try disabling firewall temporarily")
    print("   3. Check if port 1883 is blocked")
    print("   4. Run 'python test_mqtt_connection.py' for detailed diagnosis")
    
    return False

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

# --- New endpoint for status ---
@app.route("/status")
def status():
    camera = get_global_camera()
    status_info = {
        "camera_open": camera.is_open,
        "camera_index": camera.camera_index,
        "mqtt_connected": mqtt_client.is_connected() if mqtt_client else False,
        "mqtt_broker": MQTT_BROKER,
        "mqtt_topic_subscribe": MQTT_TOPIC_SUBSCRIBE,
        "mqtt_topic_status": MQTT_TOPIC_STATUS,
        "mqtt_snapshot_dir": MQTT_SNAPSHOT_DIR,
        "message": "Camera is running and ready for snapshots!"
    }
    return status_info

# --- MQTT Test endpoint ---
@app.route("/mqtt_test")
def mqtt_test():
    """Test MQTT connection and send a test message"""
    if mqtt_client and mqtt_client.is_connected():
        # Send test message to trigger snapshot
        test_payload = json.dumps({"flag": 1, "crop": 1, "source": "http_test"})
        mqtt_client.publish("yolouno/sensor/nfc/test", test_payload)
        return {"status": "success", "message": "Test MQTT message sent"}
    else:
        return {"status": "error", "message": "MQTT client not connected"}, 500

@app.route("/mqtt_snapshot")
def manual_mqtt_snapshot():
    """Manual trigger for MQTT snapshot (for testing)"""
    result = capture_snapshot_via_mqtt(crop=True)
    if result:
        return {"status": "success", "message": "MQTT snapshot captured successfully"}
    else:
        return {"status": "error", "message": "Failed to capture MQTT snapshot"}, 500

if __name__ == "__main__":
    print("🚀 Starting Enhanced License Plate Detection Server with MQTT...")
    print("📸 Snapshot feature: Camera will continue running after snapshots")
    print("📡 MQTT Integration: Listening for snapshot triggers")
    print(f"🌐 HTTP Access at: http://127.0.0.1:5000")
    print(f"📨 MQTT Subscribe Topic: {MQTT_TOPIC_SUBSCRIBE}")
    print(f"📤 MQTT Status Topic: {MQTT_TOPIC_STATUS}")
    print(f"💾 MQTT Snapshots Directory: {MQTT_SNAPSHOT_DIR}")
    
    # Setup MQTT client
    mqtt_setup_success = setup_mqtt()
    if not mqtt_setup_success:
        print("⚠️ MQTT setup failed, continuing with HTTP only...")
    
    # Start Flask app
    app.run(host="0.0.0.0", port=5000, debug=True)
