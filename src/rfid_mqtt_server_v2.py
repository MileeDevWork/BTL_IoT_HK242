import paho.mqtt.client as mqtt
import json
import logging
import threading
import time
import requests
from datetime import datetime
from whitelist_db import WhitelistDB
from mqtt_config import *

# Setup logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

class RFIDMQTTServer:
    def __init__(self):
        """Khởi tạo RFID MQTT Server với hỗ trợ vào/ra"""
        self.client = mqtt.Client()
        self.db = WhitelistDB()
        self.is_running = False
        
        # Setup MQTT callbacks
        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message
        self.client.on_disconnect = self.on_disconnect
        
        logger.info("🚀 RFID MQTT Server (Vào/Ra) đã được khởi tạo")
    
    def on_connect(self, client, userdata, flags, rc):
        """Callback khi kết nối MQTT thành công"""
        if rc == 0:
            logger.info(f"✅ Đã kết nối MQTT Broker: {MQTT_BROKER}:{MQTT_PORT}")
            
            # Subscribe cả 2 topic vào và ra
            client.subscribe(TOPIC_SUB_IN)
            client.subscribe(TOPIC_SUB_OUT)
            logger.info(f"📡 Đã subscribe topics:")
            logger.info(f"  - Vào: {TOPIC_SUB_IN}")
            logger.info(f"  - Ra: {TOPIC_SUB_OUT}")
            
            self.is_running = True
        else:
            logger.error(f"❌ Lỗi kết nối MQTT Broker. Code: {rc}")
    
    def on_disconnect(self, client, userdata, rc):
        """Callback khi mất kết nối MQTT"""
        logger.warning(f"⚠️ Mất kết nối MQTT Broker. Code: {rc}")
        self.is_running = False
    
    def on_message(self, client, userdata, msg):
        """
        Callback xử lý message nhận được từ MQTT
        
        Expected message format:
        {
            "uid": "A1B2C3D4",
            "device_id": "RFID_READER_001",
            "timestamp": "2025-05-25T10:30:00"
        }
        """
        try:
            # Decode message
            message = msg.payload.decode('utf-8')
            logger.info(f"📨 Nhận message từ {msg.topic}: {message}")
            
            # Parse JSON
            try:
                data = json.loads(message)
            except json.JSONDecodeError:
                # Nếu không phải JSON, coi như UID trực tiếp
                data = {"uid": message.strip()}
            
            uid = data.get("uid", "").strip()
            device_id = data.get("device_id", "UNKNOWN_DEVICE")
            
            if not uid:
                logger.warning("⚠️ Message không chứa UID hợp lệ")
                return
            
            # Xác định loại quét (vào hay ra) dựa trên topic
            is_entry = msg.topic == TOPIC_SUB_IN
            scan_type = "entry" if is_entry else "exit"
            
            # Kiểm tra UID trong whitelist
            auth_result = self.db.check_uid_allowed(uid)
            
            # Log access attempt
            self.db.log_access_attempt(
                uid=uid,
                allowed=auth_result["allowed"],
                additional_info={
                    "device_id": device_id,
                    "topic": msg.topic,
                    "scan_type": scan_type,
                    "timestamp": datetime.now().isoformat()
                }
            )
            
            if not auth_result["allowed"]:
                # Nếu không được phép, gửi phản hồi từ chối
                response = {
                    "uid": uid,
                    "allowed": False,
                    "scan_type": scan_type,
                    "message": auth_result["message"],
                    "timestamp": datetime.now().isoformat(),
                    "device_id": device_id
                }
                self.send_response(response, is_entry)
                return
            
            # Nếu được phép truy cập, chụp ảnh và xử lý biển số
            if is_entry:
                self.handle_vehicle_entry(uid, auth_result, device_id)
            else:
                self.handle_vehicle_exit(uid, auth_result, device_id)
            
        except Exception as e:
            logger.error(f"❌ Lỗi xử lý message: {e}")
            
            # Gửi error response
            error_response = {
                "uid": "UNKNOWN",
                "allowed": False,
                "message": f"Lỗi hệ thống: {str(e)}",
                "timestamp": datetime.now().isoformat(),
                "error": True
            }
            self.send_response(error_response, True)  # Default to entry topic
    
    def handle_vehicle_entry(self, uid, auth_result, device_id):
        """Xử lý xe vào bãi"""
        try:
            # Chụp ảnh và trích xuất biển số
            snapshot_result = self.capture_and_extract_plate(uid, "entry")
            
            if snapshot_result["success"]:
                license_plate = snapshot_result["license_plate"]
                image_path = snapshot_result["image_path"]
                
                # Ghi nhận xe vào database
                entry_result = self.db.vehicle_entry(uid, license_plate, image_path)
                
                # Tạo response
                response = {
                    "uid": uid,
                    "allowed": True,
                    "scan_type": "entry",
                    "name": auth_result["name"],
                    "department": auth_result["department"],
                    "license_plate": license_plate,
                    "message": entry_result["message"],
                    "success": entry_result["success"],
                    "timestamp": datetime.now().isoformat(),
                    "device_id": device_id
                }
                
                logger.info(f"🚗➡️ Xe vào: {auth_result['name']} - {license_plate}")
                
            else:
                # Lỗi chụp ảnh hoặc trích xuất biển số
                response = {
                    "uid": uid,
                    "allowed": True,
                    "scan_type": "entry",
                    "name": auth_result["name"],
                    "department": auth_result["department"],
                    "message": f"Lỗi xử lý ảnh: {snapshot_result['error']}",
                    "success": False,
                    "timestamp": datetime.now().isoformat(),
                    "device_id": device_id
                }
                
                logger.warning(f"⚠️ Lỗi xử lý xe vào {uid}: {snapshot_result['error']}")
            
            self.send_response(response, True)
            
        except Exception as e:
            logger.error(f"❌ Lỗi handle_vehicle_entry {uid}: {e}")
    
    def handle_vehicle_exit(self, uid, auth_result, device_id):
        """Xử lý xe ra khỏi bãi"""
        try:
            # Chụp ảnh và trích xuất biển số
            snapshot_result = self.capture_and_extract_plate(uid, "exit")
            
            if snapshot_result["success"]:
                license_plate = snapshot_result["license_plate"]
                image_path = snapshot_result["image_path"]
                
                # Ghi nhận xe ra và kiểm tra khớp biển số
                exit_result = self.db.vehicle_exit(uid, license_plate, image_path)
                
                # Tạo response
                response = {
                    "uid": uid,
                    "allowed": True,
                    "scan_type": "exit",
                    "name": auth_result["name"],
                    "department": auth_result["department"],
                    "license_plate": license_plate,
                    "message": exit_result["message"],
                    "success": exit_result["success"],
                    "match_status": exit_result.get("match_status"),
                    "entry_plate": exit_result.get("entry_plate"),
                    "exit_plate": exit_result.get("exit_plate"),
                    "timestamp": datetime.now().isoformat(),
                    "device_id": device_id
                }
                
                if exit_result["success"] and exit_result.get("match_status") == "match":
                    logger.info(f"🚗⬅️✅ Xe ra khớp: {auth_result['name']} - {license_plate}")
                elif exit_result["success"] and exit_result.get("match_status") == "mismatch":
                    logger.warning(f"🚗⬅️❌ Xe ra KHÔNG khớp: {auth_result['name']} - {exit_result.get('entry_plate')} ≠ {license_plate}")
                else:
                    logger.warning(f"🚗⬅️⚠️ Xe ra có vấn đề: {auth_result['name']} - {exit_result['message']}")
                
            else:
                # Lỗi chụp ảnh hoặc trích xuất biển số
                response = {
                    "uid": uid,
                    "allowed": True,
                    "scan_type": "exit",
                    "name": auth_result["name"],
                    "department": auth_result["department"],
                    "message": f"Lỗi xử lý ảnh: {snapshot_result['error']}",
                    "success": False,
                    "timestamp": datetime.now().isoformat(),
                    "device_id": device_id
                }
                
                logger.warning(f"⚠️ Lỗi xử lý xe ra {uid}: {snapshot_result['error']}")
            
            self.send_response(response, False)
            
        except Exception as e:
            logger.error(f"❌ Lỗi handle_vehicle_exit {uid}: {e}")
    
    def capture_and_extract_plate(self, uid, scan_type):
        """
        Chụp ảnh và trích xuất biển số xe
        
        Args:
            uid (str): UID của thẻ
            scan_type (str): "entry" hoặc "exit"
            
        Returns:
            dict: Kết quả chụp ảnh và trích xuất
        """
        try:
            # URL chụp ảnh với flag để trích xuất biển số
            snapshot_url = f"http://{FLASK_HOST}:{FLASK_PORT}/snapshot?flag=1&crop=1&extract_plate=1"
            
            logger.info(f"📸 Đang chụp ảnh {scan_type} cho UID: {uid}")
            
            # Gửi request chụp ảnh
            response = requests.get(snapshot_url, timeout=15)
            
            if response.status_code == 200:
                try:
                    # Parse JSON response từ camera
                    camera_result = response.json()
                    
                    if camera_result.get("success"):
                        license_plate = camera_result.get("license_plate", "").strip()
                        image_path = camera_result.get("image_path", "")
                        
                        if license_plate:
                            logger.info(f"✅ Đã trích xuất biển số {scan_type}: {license_plate}")
                            return {
                                "success": True,
                                "license_plate": license_plate,
                                "image_path": image_path
                            }
                        else:
                            logger.warning(f"⚠️ Không trích xuất được biển số từ ảnh {scan_type}")
                            return {
                                "success": False,
                                "error": "Không trích xuất được biển số từ ảnh"
                            }
                    else:
                        error_msg = camera_result.get("error", "Lỗi không xác định từ camera")
                        logger.warning(f"⚠️ Camera trả về lỗi {scan_type}: {error_msg}")
                        return {
                            "success": False,
                            "error": error_msg
                        }
                        
                except json.JSONDecodeError:
                    logger.warning(f"⚠️ Camera response không phải JSON {scan_type}")
                    return {
                        "success": False,
                        "error": "Camera response không đúng định dạng"
                    }
            else:
                logger.warning(f"⚠️ Lỗi chụp ảnh {scan_type}: HTTP {response.status_code}")
                return {
                    "success": False,
                    "error": f"HTTP {response.status_code}"
                }
                
        except requests.exceptions.Timeout:
            logger.warning(f"⚠️ Timeout chụp ảnh {scan_type}")
            return {
                "success": False,
                "error": "Timeout kết nối camera"
            }
        except requests.exceptions.RequestException as e:
            logger.warning(f"⚠️ Không thể kết nối camera {scan_type}: {e}")
            return {
                "success": False,
                "error": f"Không thể kết nối camera: {str(e)}"
            }
        except Exception as e:
            logger.error(f"❌ Lỗi capture_and_extract_plate {scan_type}: {e}")
            return {
                "success": False,
                "error": f"Lỗi hệ thống: {str(e)}"
            }
    def send_response(self, response_data, is_entry=True):
        """Gửi phản hồi về thiết bị qua MQTT"""
        try:
            response_json = json.dumps(response_data, ensure_ascii=False, indent=2)
            
            # Chọn topic phù hợp
            topic = TOPIC_PUB_IN if is_entry else TOPIC_PUB_OUT
            
            # Publish to specific in/out topic
            result = self.client.publish(topic, response_json)
            
            # Also publish to general response topic for widget compatibility
            self.client.publish("yolouno/rfid/response", response_json)
            
            if result.rc == mqtt.MQTT_ERR_SUCCESS:
                status = "✅ Cho phép" if response_data.get("allowed") else "❌ Từ chối"
                scan_type = response_data.get("scan_type", "unknown")
                logger.info(f"📤 Đã gửi response {scan_type}: {status} - UID: {response_data.get('uid')}")
            else:
                logger.error(f"❌ Lỗi gửi response. Code: {result.rc}")
                
        except Exception as e:
            logger.error(f"❌ Lỗi gửi response: {e}")
    
    def start(self):
        """Khởi động MQTT Server"""
        try:
            logger.info(f"🔄 Đang kết nối tới MQTT Broker...")
            
            # Kết nối MQTT Broker
            self.client.connect(MQTT_BROKER, MQTT_PORT, 60)
            
            # Bắt đầu loop
            self.client.loop_start()
            
            logger.info("🚀 RFID MQTT Server (Vào/Ra) đã khởi động!")
            logger.info(f"📡 Subscribe topics:")
            logger.info(f"  - Vào: {TOPIC_SUB_IN}")
            logger.info(f"  - Ra: {TOPIC_SUB_OUT}")
            logger.info(f"📤 Publish topics:")
            logger.info(f"  - Vào: {TOPIC_PUB_IN}")
            logger.info(f"  - Ra: {TOPIC_PUB_OUT}")
            logger.info(f"📸 Camera URL: http://{FLASK_HOST}:{FLASK_PORT}")
            
            # Keep running
            while True:
                if not self.is_running:
                    logger.warning("⚠️ MQTT connection lost. Attempting to reconnect...")
                    time.sleep(5)
                    try:
                        self.client.reconnect()
                    except Exception as e:
                        logger.error(f"❌ Reconnect failed: {e}")
                
                time.sleep(1)
                
        except KeyboardInterrupt:
            logger.info("🛑 Đang dừng server...")
            self.stop()
        except Exception as e:
            logger.error(f"❌ Lỗi khởi động server: {e}")
    
    def stop(self):
        """Dừng MQTT Server"""
        try:
            self.client.loop_stop()
            self.client.disconnect()
            logger.info("✅ RFID MQTT Server đã dừng")
        except Exception as e:
            logger.error(f"❌ Lỗi dừng server: {e}")
    
    def test_uid(self, uid, scan_type="entry"):
        """Test function để kiểm tra UID"""
        logger.info(f"🧪 Testing UID: {uid} ({scan_type})")
        
        # Tạo test message
        test_message = {
            "uid": uid,
            "device_id": "TEST_DEVICE",
            "timestamp": datetime.now().isoformat()
        }
        
        # Simulate incoming message
        class TestMessage:
            def __init__(self, payload, topic):
                self.payload = payload.encode('utf-8')
                self.topic = topic
        
        topic = TOPIC_SUB_IN if scan_type == "entry" else TOPIC_SUB_OUT
        test_msg = TestMessage(json.dumps(test_message), topic)
        self.on_message(self.client, None, test_msg)
    
    def show_dashboard(self):
        """Hiển thị dashboard thông tin"""
        try:
            print("\n" + "="*60)
            print("🚗 DASHBOARD - HỆ THỐNG QUẢN LÝ XE VÀO/RA")
            print("="*60)
            
            # Xe đang trong bãi
            vehicles_inside = self.db.get_vehicles_in_parking()
            print(f"\n🅿️ XE ĐANG TRONG BÃI ({len(vehicles_inside)} xe):")
            if vehicles_inside:
                for vehicle in vehicles_inside:
                    entry_time = vehicle["entry_time"].strftime("%H:%M %d/%m")
                    print(f"  - UID: {vehicle['uid']} | Biển số: {vehicle['license_plate']} | Vào lúc: {entry_time}")
            else:
                print("  (Không có xe nào trong bãi)")
            
            # Lịch sử gần đây
            recent_history = self.db.get_vehicle_history(limit=5)
            print(f"\n📊 LỊCH SỬ GẦN ĐÂY ({len(recent_history)} record):")
            for record in recent_history:
                status = "Trong bãi" if record["status"] == "inside" else "Đã ra"
                entry_time = record["entry_time"].strftime("%H:%M %d/%m")
                if record["exit_time"]:
                    exit_time = record["exit_time"].strftime("%H:%M %d/%m")
                    match_icon = "✅" if record.get("match_status") == "match" else "❌"
                    print(f"  - UID: {record['uid']} | {record['license_plate']} | {entry_time} → {exit_time} {match_icon}")
                else:
                    print(f"  - UID: {record['uid']} | {record['license_plate']} | {entry_time} | {status}")
            
            # Trường hợp không khớp biển số
            mismatches = self.db.get_mismatch_reports(limit=3)
            if mismatches:
                print(f"\n⚠️ BIỂN SỐ KHÔNG KHỚP ({len(mismatches)} trường hợp):")
                for mismatch in mismatches:
                    exit_time = mismatch["exit_time"].strftime("%H:%M %d/%m")
                    print(f"  - UID: {mismatch['uid']} | Vào: {mismatch['license_plate']} | Ra: {mismatch['exit_license_plate']} | {exit_time}")
            
            print("="*60)
            
        except Exception as e:
            logger.error(f"❌ Lỗi hiển thị dashboard: {e}")

# CLI Interface cho testing
def main():
    server = RFIDMQTTServer()
    
    print("\n🎯 RFID MQTT Access Control System (Vào/Ra)")
    print("="*60)
    print("1. Khởi động server")
    print("2. Test UID vào")
    print("3. Test UID ra")
    print("4. Xem dashboard")
    print("5. Xem danh sách thẻ")
    print("6. Thêm thẻ mới")
    print("7. Thoát")
    
    while True:
        try:
            choice = input("\n👉 Chọn chức năng (1-7): ").strip()
            
            if choice == "1":
                print("\n🚀 Đang khởi động RFID MQTT Server...")
                server.start()
                break
                
            elif choice == "2":
                uid = input("Nhập UID để test (VÀO): ").strip()
                if uid:
                    server.test_uid(uid, "entry")
                
            elif choice == "3":
                uid = input("Nhập UID để test (RA): ").strip()
                if uid:
                    server.test_uid(uid, "exit")
            
            elif choice == "4":
                server.show_dashboard()
                
            elif choice == "5":
                cards = server.db.get_all_cards()
                print(f"\n📋 Danh sách thẻ ({len(cards)} thẻ):")
                for card in cards:
                    print(f"  - UID: {card['uid']} | {card['name']} | {card['department']}")
                
            elif choice == "6":
                uid = input("Nhập UID: ").strip()
                name = input("Nhập tên: ").strip()
                dept = input("Nhập phòng ban: ").strip() or "Unknown"
                
                if uid and name:
                    success = server.db.add_card(uid, name, dept)
                    if success:
                        print(f"✅ Đã thêm thẻ {uid} - {name}")
                    else:
                        print(f"❌ Lỗi thêm thẻ (có thể đã tồn tại)")
                
            elif choice == "7":
                print("👋 Tạm biệt!")
                break
                
            else:
                print("❌ Lựa chọn không hợp lệ!")
                
        except KeyboardInterrupt:
            print("\n🛑 Đã dừng chương trình")
            break

if __name__ == "__main__":
    main()
