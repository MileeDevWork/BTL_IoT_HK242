import pymongo
from datetime import datetime
import logging
from mqtt_config import MONGODB_URI, DATABASE_NAME, COLLECTION_NAME

# Setup logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

class WhitelistDB:
    def __init__(self):
        """Khởi tạo kết nối MongoDB"""
        try:
            self.client = pymongo.MongoClient(MONGODB_URI)
            self.db = self.client[DATABASE_NAME]
            self.collection = self.db[COLLECTION_NAME]
            logger.info("✅ Đã kết nối MongoDB thành công")
            
            # Tạo sample data nếu collection trống
            self._init_sample_data()
            
        except Exception as e:
            logger.error(f"❌ Lỗi kết nối MongoDB: {e}")
            raise
    
    def _init_sample_data(self):
        """Khởi tạo dữ liệu mẫu cho whitelist"""
        if self.collection.count_documents({}) == 0:
            sample_cards = [
                {
                    "uid": "A1B2C3D4",
                    "name": "Nguyễn Văn A",
                    "department": "IT",
                    "status": "active",
                    "created_at": datetime.now(),
                    "updated_at": datetime.now()
                },
                {
                    "uid": "E5F6G7H8",
                    "name": "Trần Thị B", 
                    "department": "HR",
                    "status": "active",
                    "created_at": datetime.now(),
                    "updated_at": datetime.now()
                },
                {
                    "uid": "I9J0K1L2",
                    "name": "Lê Văn C",
                    "department": "Security",
                    "status": "active", 
                    "created_at": datetime.now(),
                    "updated_at": datetime.now()
                }
            ]
            
            self.collection.insert_many(sample_cards)
            logger.info(f"✅ Đã thêm {len(sample_cards)} thẻ mẫu vào whitelist")
    
    def check_uid_allowed(self, uid):
        """
        Kiểm tra UID có trong danh sách cho phép không
        
        Args:
            uid (str): UID của thẻ RFID
            
        Returns:
            dict: Thông tin kết quả kiểm tra
        """
        try:
            card = self.collection.find_one({
                "uid": uid,
                "status": "active"
            })
            
            if card:
                logger.info(f"✅ UID {uid} được phép truy cập - {card['name']}")
                return {
                    "allowed": True,
                    "uid": uid,
                    "name": card["name"],
                    "department": card["department"],
                    "message": "Truy cập được phép"
                }
            else:
                logger.warning(f"❌ UID {uid} không được phép truy cập")
                return {
                    "allowed": False,
                    "uid": uid,
                    "name": None,
                    "department": None,
                    "message": "Truy cập bị từ chối"
                }
                
        except Exception as e:
            logger.error(f"❌ Lỗi kiểm tra UID {uid}: {e}")
            return {
                "allowed": False,
                "uid": uid,
                "name": None,
                "department": None,
                "message": f"Lỗi hệ thống: {str(e)}"
            }
    
    def add_card(self, uid, name, department="Unknown"):
        """Thêm thẻ mới vào whitelist"""
        try:
            card_data = {
                "uid": uid,
                "name": name,
                "department": department,
                "status": "active",
                "created_at": datetime.now(),
                "updated_at": datetime.now()
            }
            
            # Kiểm tra UID đã tồn tại chưa
            existing = self.collection.find_one({"uid": uid})
            if existing:
                logger.warning(f"⚠️ UID {uid} đã tồn tại")
                return False
            
            self.collection.insert_one(card_data)
            logger.info(f"✅ Đã thêm thẻ {uid} - {name}")
            return True
            
        except Exception as e:
            logger.error(f"❌ Lỗi thêm thẻ {uid}: {e}")
            return False
    
    def remove_card(self, uid):
        """Xóa thẻ khỏi whitelist (soft delete)"""
        try:
            result = self.collection.update_one(
                {"uid": uid},
                {
                    "$set": {
                        "status": "inactive",
                        "updated_at": datetime.now()
                    }
                }
            )
            
            if result.modified_count > 0:
                logger.info(f"✅ Đã vô hiệu hóa thẻ {uid}")
                return True
            else:
                logger.warning(f"⚠️ Không tìm thấy thẻ {uid}")
                return False
                
        except Exception as e:
            logger.error(f"❌ Lỗi xóa thẻ {uid}: {e}")
            return False
    
    def get_all_cards(self):
        """Lấy danh sách tất cả thẻ active"""
        try:
            cards = list(self.collection.find({"status": "active"}))
            return cards
        except Exception as e:
            logger.error(f"❌ Lỗi lấy danh sách thẻ: {e}")
            return []
    
    def log_access_attempt(self, uid, allowed, additional_info=None):
        """Ghi log lại việc truy cập"""
        try:
            log_collection = self.db["access_logs"]
            log_data = {
                "uid": uid,
                "allowed": allowed,
                "timestamp": datetime.now(),
                "additional_info": additional_info or {}
            }
            
            log_collection.insert_one(log_data)
            logger.info(f"📝 Đã ghi log truy cập: {uid} - {'Cho phép' if allowed else 'Từ chối'}")
            
        except Exception as e:
            logger.error(f"❌ Lỗi ghi log: {e}")

# Test functions
if __name__ == "__main__":
    print("🧪 Testing WhitelistDB...")
    
    try:
        db = WhitelistDB()
        
        # Test check existing UID
        result = db.check_uid_allowed("A1B2C3D4")
        print(f"Test UID A1B2C3D4: {result}")
        
        # Test check non-existing UID
        result = db.check_uid_allowed("XXXXXXXX")
        print(f"Test UID XXXXXXXX: {result}")
        
        # Test get all cards
        cards = db.get_all_cards()
        print(f"Tổng số thẻ active: {len(cards)}")
        
        print("✅ Test hoàn thành!")
        
    except Exception as e:
        print(f"❌ Lỗi test: {e}")
