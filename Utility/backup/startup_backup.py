#!/usr/bin/env python3
"""
Startup script cho hệ thống kiểm soát truy cập tích hợp
Kết hợp RFID MQTT + Camera License Plate Detection
"""

import os
import sys
import time
import threading
import subprocess
import signal
from datetime import datetime

def print_banner():
    """In banner hệ thống"""
    print("="*80)
    print("🎯 HỆ THỐNG KIỂM SOÁT TRUY CẬP TÍCH HỢP")
    print("🔐 RFID MQTT Access Control + 📸 License Plate Detection")
    print("="*80)
    print(f"📅 Khởi động lúc: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    print()

def check_dependencies():
    """Kiểm tra các dependencies cần thiết"""
    print("🔍 Đang kiểm tra dependencies...")
    
    required_modules = [
        "paho.mqtt",
        "pymongo", 
        "flask",
        "cv2",
        "ultralytics"
    ]
      missing_modules = []
    
    for module in required_modules:
        try:
            __import__(module.replace(".", "_") if "." in module else module)
            print(f"  ✅ {module}")
        except ImportError:
            print(f"  ❌ {module}")
            missing_modules.append(module)
    
    if missing_modules:
        print(f"\n⚠️ Thiếu {len(missing_modules)} module(s). Cài đặt với:")
        print("pip install -r requirements.txt")
        return False
    
    print("✅ Tất cả dependencies đã sẵn sàng!")
    return True

def check_mongodb():
    """Kiểm tra kết nối MongoDB"""
    print("\n🔍 Đang kiểm tra MongoDB...")
    
    try:
        import pymongo
        client = pymongo.MongoClient("mongodb://localhost:27017/", serverSelectionTimeoutMS=3000)
        client.admin.command('ping')
        print("✅ MongoDB đã kết nối thành công!")
        client.close()
        return True
    except Exception as e:
        print(f"⚠️ MongoDB không khả dụng: {e}")
        print("💡 Hệ thống vẫn có thể chạy nhưng không có database persistence")
        return False

def check_camera():
    """Kiểm tra camera"""
    print("\n🔍 Đang kiểm tra camera...")
    
    try:
        import cv2
        cap = cv2.VideoCapture(0)
        if cap.isOpened():
            ret, frame = cap.read()
            if ret:
                print("✅ Camera hoạt động bình thường!")
                cap.release()
                return True
            else:
                print("⚠️ Camera không thể capture frame")
        else:
            print("⚠️ Không thể mở camera")
        cap.release()
        return False
    except Exception as e:
        print(f"❌ Lỗi kiểm tra camera: {e}")
        return False

def start_integrated_system():
    """Khởi động hệ thống tích hợp"""
    print("\n🚀 Đang khởi động hệ thống tích hợp...")
    
    try:
        # Khởi động video_camera.py (đã tích hợp RFID)
        process = subprocess.Popen([
            sys.executable, 'video_camera.py'
        ])
        
        print("✅ Hệ thống tích hợp đã khởi động!")
        print("🌐 Web interface: http://localhost:5000")
        print("📡 MQTT Topics:")
        print("  - Subscribe: yolouno/rfid/scan")
        print("  - Publish: yolouno/rfid/response")
        print("📊 APIs:")
        print("  - Camera Status: http://localhost:5000/status")
        print("  - RFID Status: http://localhost:5000/rfid/status")
        print("  - Whitelist: http://localhost:5000/rfid/whitelist")
        print("\n⏹️ Press Ctrl+C to stop...")
        
        # Chờ tín hiệu dừng
        try:
            process.wait()
        except KeyboardInterrupt:
            print("\n🛑 Đang dừng hệ thống...")
            process.terminate()
            
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                print("⚠️ Force killing process...")
                process.kill()
            
            print("✅ Hệ thống đã dừng thành công")
        
    except Exception as e:
        print(f"❌ Lỗi khởi động hệ thống: {e}")

def start_standalone_rfid():
    """Khởi động RFID server riêng biệt"""
    print("\n🚀 Đang khởi động RFID MQTT Server (standalone)...")
    
    try:
        process = subprocess.Popen([
            sys.executable, 'rfid_mqtt_server.py'
        ])
        
        print("✅ RFID MQTT Server đã khởi động!")
        print("📡 MQTT Broker: test.mosquitto.org:1883")
        print("📥 Subscribe: yolouno/rfid/scan")
        print("📤 Publish: yolouno/rfid/response")
        print("\n⏹️ Press Ctrl+C to stop...")
        
        try:
            process.wait()
        except KeyboardInterrupt:
            print("\n🛑 Đang dừng RFID server...")
            process.terminate()
            
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                print("⚠️ Force killing process...")
                process.kill()
            
            print("✅ RFID server đã dừng thành công")
        
    except Exception as e:
        print(f"❌ Lỗi khởi động RFID server: {e}")

def test_system():
    """Test các thành phần của hệ thống"""
    print("\n🧪 KIỂM TRA HỆ THỐNG")
    print("-" * 40)
    
    try:
        # Test database
        print("1. Kiểm tra Database...")
        from whitelist_db import WhitelistDB
        db = WhitelistDB()
        cards = db.get_all_cards()
        print(f"   ✅ Database: {len(cards)} thẻ trong whitelist")
        
        # Test UID mẫu
        test_uid = "A1B2C3D4"
        result = db.check_uid_allowed(test_uid)
        print(f"   ✅ Test UID {test_uid}: {'Cho phép' if result['allowed'] else 'Từ chối'}")
        
        # Test MQTT config
        print("2. Kiểm tra MQTT Config...")
        from mqtt_config import MQTT_BROKER, MQTT_PORT, TOPIC_SUB, TOPIC_PUB
        print(f"   ✅ MQTT Broker: {MQTT_BROKER}:{MQTT_PORT}")
        print(f"   ✅ Subscribe: {TOPIC_SUB}")
        print(f"   ✅ Publish: {TOPIC_PUB}")
        
        print("\n✅ Tất cả test đều PASS!")
        
    except Exception as e:
        print(f"❌ Lỗi test: {e}")

def show_config():
    """Hiển thị cấu hình hệ thống"""
    print("\n📋 CẤU HÌNH HỆ THỐNG")
    print("-" * 40)
    
    try:
        from mqtt_config import *
        
        print("🔐 RFID MQTT Configuration:")
        print(f"  - MQTT Broker: {MQTT_BROKER}:{MQTT_PORT}")
        print(f"  - Subscribe Topic: {TOPIC_SUB}")
        print(f"  - Publish Topic: {TOPIC_PUB}")
        print(f"  - MongoDB URI: {MONGODB_URI}")
        print(f"  - Database: {DATABASE_NAME}")
        print(f"  - Collection: {COLLECTION_NAME}")
        
        print(f"\n📸 Camera Configuration:")
        print(f"  - Flask Host: {FLASK_HOST}:{FLASK_PORT}")
        print(f"  - Web Interface: http://{FLASK_HOST}:{FLASK_PORT}")
        print(f"  - Video Feed: http://{FLASK_HOST}:{FLASK_PORT}/video_feed")
        print(f"  - Snapshot API: http://{FLASK_HOST}:{FLASK_PORT}/snapshot?flag=1")
        
        print(f"\n🔗 Integrated APIs:")
        print(f"  - RFID Status: http://{FLASK_HOST}:{FLASK_PORT}/rfid/status")
        print(f"  - Whitelist: http://{FLASK_HOST}:{FLASK_PORT}/rfid/whitelist")
        print(f"  - Add Card: POST http://{FLASK_HOST}:{FLASK_PORT}/rfid/add_card")
        
    except Exception as e:
        print(f"❌ Lỗi đọc config: {e}")

def main_menu():
    """Menu chính của ứng dụng"""
    print_banner()
    
    # Kiểm tra hệ thống
    deps_ok = check_dependencies()
    if not deps_ok:
        print("\n❌ Vui lòng cài đặt dependencies trước khi tiếp tục!")
        return
    
    mongo_ok = check_mongodb()
    camera_ok = check_camera()
    
    print("\n" + "="*50)
    print("🎛️  CHỌN CHẾ ĐỘ KHỞI ĐỘNG")
    print("="*50)
    print("1. 🔗 Hệ thống tích hợp (RFID + Camera)")
    print("2. 🔐 Chỉ RFID MQTT Server")
    print("3. 📸 Chỉ Camera System")
    print("4. 🧪 Test hệ thống")
    print("5. 📋 Xem cấu hình")
    print("6. 🚪 Thoát")
    
    while True:
        try:
            choice = input("\n👉 Chọn chế độ (1-6): ").strip()
            
            if choice == "1":
                print("\n🔗 Khởi động hệ thống tích hợp...")
                start_integrated_system()
                break
                
            elif choice == "2":
                print("\n🔐 Khởi động RFID MQTT Server...")
                start_standalone_rfid()
                break
                
            elif choice == "3":
                print("\n📸 Khởi động Camera System...")
                subprocess.run([sys.executable, 'video_camera.py'])
                break
                
            elif choice == "4":
                test_system()
                
            elif choice == "5":
                show_config()
                
            elif choice == "6":
                print("👋 Tạm biệt!")
                break
                
            else:
                print("❌ Lựa chọn không hợp lệ!")
                
        except KeyboardInterrupt:
            print("\n🛑 Đã dừng chương trình")
            break
            show_usage()
    else:
        # Kiểm tra và khởi động hệ thống
        if check_requirements() and check_files():
            start_system()
        else:
            print("\n❌ Cannot start system due to missing requirements")
            print("💡 Run: python startup.py check")

if __name__ == "__main__":
    main()
