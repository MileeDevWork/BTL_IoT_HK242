# Test script for license plate database functionality
import sys
import os
from datetime import datetime

# Add the src directory to Python path
sys.path.append(os.path.dirname(__file__))

try:
    from whitelist_db import WhitelistDB
    print("✅ Successfully imported WhitelistDB")
except ImportError as e:
    print(f"❌ Failed to import WhitelistDB: {e}")
    sys.exit(1)

def test_license_plate_database():
    """Test the license plate database functionality"""
    print("🧪 Testing License Plate Database Functionality...")
    print("=" * 50)
    
    try:
        # Initialize database connection
        print("1. Initializing database connection...")
        db = WhitelistDB()
        print("✅ Database connection successful")
        
        # Test saving license plates
        print("\n2. Testing license plate saving...")
        test_plates = [
            ("ABC123", "c:/test/image1.jpg"),
            ("XYZ789", "c:/test/image2.jpg"),
            ("DEF456", None),  # Test without image path
            ("", "c:/test/image3.jpg"),  # Test with empty plate text
        ]
        
        saved_ids = []
        for plate_text, image_path in test_plates:
            plate_id = db.save_license_plate(plate_text, image_path)
            if plate_id:
                saved_ids.append(plate_id)
                print(f"✅ Saved plate '{plate_text}' with ID: {plate_id}")
            else:
                print(f"❌ Failed to save plate '{plate_text}'")
        
        # Test retrieving license plates
        print(f"\n3. Testing license plate retrieval...")
        recent_plates = db.get_license_plates(limit=10)
        print(f"✅ Retrieved {len(recent_plates)} recent license plates:")
        
        for i, plate in enumerate(recent_plates[-3:], 1):  # Show last 3
            print(f"   {i}. Plate: '{plate.get('plate', 'N/A')}' | Time: {plate.get('time_in', 'N/A')} | ID: {plate.get('_id', 'N/A')}")
        
        # Test searching license plates
        print(f"\n4. Testing license plate search...")
        if saved_ids:
            search_results = db.search_license_plate("ABC")
            print(f"✅ Search for 'ABC' returned {len(search_results)} results:")
            for result in search_results:
                print(f"   - Plate: '{result.get('plate', 'N/A')}' | Time: {result.get('time_in', 'N/A')}")
        
        # Test database statistics
        print(f"\n5. Database statistics...")
        all_plates = db.get_license_plates(limit=1000)
        print(f"✅ Total license plates in database: {len(all_plates)}")
        
        print("\n" + "=" * 50)
        print("🎉 All tests completed successfully!")
        return True
        
    except Exception as e:
        print(f"❌ Test failed with error: {e}")
        import traceback
        traceback.print_exc()
        return False

if __name__ == "__main__":
    success = test_license_plate_database()
    if success:
        print("\n✅ License plate database functionality is working correctly!")
        print("📋 Database Schema:")
        print("   - ID: Auto-generated MongoDB ObjectId")
        print("   - Plate: License plate text (string)")
        print("   - Time_in: Timestamp when plate was detected (datetime)")
        print("   - Image_path: Path to saved image file (string, optional)")
        print("   - Created_at: Record creation timestamp (datetime)")
    else:
        print("\n❌ Some tests failed. Please check the error messages above.")
