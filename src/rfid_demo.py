#!/usr/bin/env python3
"""
Demo MQTT Client để test hệ thống RFID
Gửi UID RFID đến MQTT broker và nhận phản hồi
"""

import paho.mqtt.client as mqtt
import json
import time
import threading
from datetime import datetime
from mqtt_config import MQTT_BROKER, MQTT_PORT, TOPIC_SUB, TOPIC_PUB

class RFIDMQTTDemo:
    def __init__(self):
        """Khởi tạo demo client"""
        self.client = mqtt.Client()
        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message
        self.client.on_disconnect = self.on_disconnect
        
        self.responses = []
        self.connected = False
        
        print("🧪 RFID MQTT Demo Client")
        print("=" * 40)
    
    def on_connect(self, client, userdata, flags, rc):
        """Callback khi kết nối thành công"""
        if rc == 0:
            self.connected = True
            print(f"✅ Đã kết nối MQTT Broker: {MQTT_BROKER}:{MQTT_PORT}")
            
            # Subscribe để nhận response
            client.subscribe(TOPIC_PUB)
            print(f"📡 Đã subscribe topic response: {TOPIC_PUB}")
        else:
            print(f"❌ Lỗi kết nối MQTT. Code: {rc}")
    
    def on_disconnect(self, client, userdata, rc):
        """Callback khi mất kết nối"""
        self.connected = False
        print(f"⚠️ Mất kết nối MQTT. Code: {rc}")
    
    def on_message(self, client, userdata, msg):
        """Callback nhận response từ server"""
        try:
            message = msg.payload.decode('utf-8')
            print(f"\n📨 Nhận response từ {msg.topic}:")
            
            # Parse JSON response
            try:
                response = json.loads(message)
                
                status = "✅ CHO PHÉP" if response.get("allowed") else "❌ TỪ CHỐI"
                print(f"   {status}")
                print(f"   UID: {response.get('uid', 'N/A')}")
                print(f"   Tên: {response.get('name', 'N/A')}")
                print(f"   Phòng ban: {response.get('department', 'N/A')}")
                print(f"   Thông báo: {response.get('message', 'N/A')}")
                print(f"   Thời gian: {response.get('timestamp', 'N/A')}")
                
                self.responses.append(response)
                
            except json.JSONDecodeError:
                print(f"   Raw message: {message}")
                
        except Exception as e:
            print(f"❌ Lỗi xử lý response: {e}")
    
    def connect(self):
        """Kết nối tới MQTT broker"""
        try:
            print(f"🔄 Đang kết nối tới {MQTT_BROKER}:{MQTT_PORT}...")
            self.client.connect(MQTT_BROKER, MQTT_PORT, 60)
            self.client.loop_start()
            
            # Chờ kết nối
            timeout = 10
            while not self.connected and timeout > 0:
                time.sleep(0.5)
                timeout -= 0.5
            
            if not self.connected:
                raise Exception("Timeout kết nối")
                
            return True
            
        except Exception as e:
            print(f"❌ Lỗi kết nối: {e}")
            return False
    
    def send_rfid_scan(self, uid, device_id="DEMO_DEVICE"):
        """Gửi UID RFID đến server"""
        if not self.connected:
            print("❌ Chưa kết nối MQTT broker")
            return False
        
        try:
            # Tạo message
            message = {
                "uid": uid,
                "device_id": device_id,
                "timestamp": datetime.now().isoformat()
            }
            
            message_json = json.dumps(message, ensure_ascii=False)
            
            # Gửi message
            result = self.client.publish(TOPIC_SUB, message_json)
            
            if result.rc == mqtt.MQTT_ERR_SUCCESS:
                print(f"📤 Đã gửi UID: {uid}")
                return True
            else:
                print(f"❌ Lỗi gửi message. Code: {result.rc}")
                return False
                
        except Exception as e:
            print(f"❌ Lỗi gửi RFID scan: {e}")
            return False
    
    def disconnect(self):
        """Ngắt kết nối"""
        try:
            self.client.loop_stop()
            self.client.disconnect()
            print("✅ Đã ngắt kết nối")
        except Exception as e:
            print(f"❌ Lỗi ngắt kết nối: {e}")
    
    def demo_sequence(self):
        """Chạy demo với các UID mẫu"""
        test_uids = [
            "A1B2C3D4",  # UID hợp lệ trong whitelist
            "E5F6G7H8",  # UID hợp lệ trong whitelist  
            "I9J0K1L2",  # UID hợp lệ trong whitelist
            "XXXXXXXX",  # UID không hợp lệ
            "12345678",  # UID không tồn tại
        ]
        
        print("\n🎯 Bắt đầu demo sequence...")
        print("=" * 50)
        
        for i, uid in enumerate(test_uids, 1):
            print(f"\n🧪 Test {i}/{len(test_uids)}: Gửi UID {uid}")
            print("-" * 30)
            
            success = self.send_rfid_scan(uid)
            if success:
                # Chờ response
                time.sleep(2)
            else:
                print("❌ Không thể gửi UID")
            
            # Nghỉ giữa các test
            if i < len(test_uids):
                time.sleep(1)
        
        print(f"\n✅ Demo hoàn thành! Nhận được {len(self.responses)} response(s)")

def interactive_mode():
    """Chế độ interactive cho phép nhập UID thủ công"""
    demo = RFIDMQTTDemo()
    
    if not demo.connect():
        print("❌ Không thể kết nối MQTT broker!")
        return
    
    print("\n🎮 CHMODE INTERACTIVE")
    print("=" * 30)
    print("Nhập UID để test, hoặc 'quit' để thoát")
    print("Các UID mẫu: A1B2C3D4, E5F6G7H8, I9J0K1L2")
    
    try:
        while True:
            uid = input("\n👉 Nhập UID: ").strip()
            
            if uid.lower() in ['quit', 'exit', 'q']:
                break
            
            if not uid:
                print("⚠️ UID không được để trống")
                continue
            
            print(f"📤 Đang gửi UID: {uid}")
            demo.send_rfid_scan(uid)
            
            # Chờ response
            time.sleep(1.5)
    
    except KeyboardInterrupt:
        print("\n🛑 Đã dừng interactive mode")
    
    finally:
        demo.disconnect()

def main():
    """Main function"""
    print("🧪 RFID MQTT Demo Tool")
    print("=" * 40)
    print("1. 🎯 Chạy demo sequence")
    print("2. 🎮 Chế độ interactive")
    print("3. 🚪 Thoát")
    
    while True:
        try:
            choice = input("\n👉 Chọn chế độ (1-3): ").strip()
            
            if choice == "1":
                demo = RFIDMQTTDemo()
                if demo.connect():
                    demo.demo_sequence()
                    demo.disconnect()
                break
                
            elif choice == "2":
                interactive_mode()
                break
                
            elif choice == "3":
                print("👋 Tạm biệt!")
                break
                
            else:
                print("❌ Lựa chọn không hợp lệ!")
                
        except KeyboardInterrupt:
            print("\n🛑 Đã dừng chương trình")
            break

if __name__ == "__main__":
    main()
