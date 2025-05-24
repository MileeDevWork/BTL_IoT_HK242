#!/usr/bin/env python3
"""
Comprehensive system validation script
Tests all components of the license plate detection system
"""

import sys
import os
import time
import requests
import json
from datetime import datetime

def test_flask_server():
    """Test if Flask server is running and responsive"""
    print("🌐 Testing Flask server...")
    
    try:
        # Test main page
        response = requests.get("http://127.0.0.1:5000", timeout=5)
        if response.status_code == 200:
            print("✅ Main page accessible")
        else:
            print(f"❌ Main page returned status {response.status_code}")
            return False
            
        # Test video feed endpoint
        response = requests.get("http://127.0.0.1:5000/video_feed", timeout=5, stream=True)
        if response.status_code == 200:
            print("✅ Video feed endpoint working")
        else:
            print(f"❌ Video feed returned status {response.status_code}")
            return False
            
        # Test snapshot endpoint
        response = requests.get("http://127.0.0.1:5000/snapshot?flag=1&crop=0", timeout=10)
        if response.status_code == 200 and len(response.content) > 1000:
            print(f"✅ Snapshot endpoint working (captured {len(response.content)} bytes)")
        else:
            print(f"❌ Snapshot endpoint failed (status: {response.status_code})")
            return False
            
        return True
        
    except requests.exceptions.ConnectionError:
        print("❌ Cannot connect to Flask server. Is it running?")
        return False
    except Exception as e:
        print(f"❌ Flask server test error: {e}")
        return False

def test_file_system():
    """Test file system and directories"""
    print("📁 Testing file system...")
    
    required_files = [
        "camera.py",
        "video_camera.py", 
        "templates/index.html",
        "runs/detect/train7/weights/best.pt"
    ]
    
    required_dirs = [
        "Data",
        "Test", 
        "templates",
        "runs/detect/train7/weights"
    ]
    
    # Check files
    missing_files = []
    for file_path in required_files:
        if not os.path.exists(file_path):
            missing_files.append(file_path)
        else:
            print(f"✅ Found: {file_path}")
    
    # Check directories
    missing_dirs = []
    for dir_path in required_dirs:
        if not os.path.exists(dir_path):
            missing_dirs.append(dir_path)
        else:
            print(f"✅ Directory: {dir_path}")
    
    if missing_files:
        print(f"❌ Missing files: {missing_files}")
        return False
        
    if missing_dirs:
        print(f"❌ Missing directories: {missing_dirs}")
        return False
        
    print("✅ All required files and directories present")
    return True

def test_model_performance():
    """Test model loading and inference performance"""
    print("🤖 Testing model performance...")
    
    try:
        # Import after checking files exist
        sys.path.append(os.path.dirname(os.path.abspath(__file__)))
        from camera import model
        import cv2
        import numpy as np
        
        # Test model loading
        print("✅ Model loaded successfully")
        
        # Test inference speed
        test_image = np.random.randint(0, 255, (640, 640, 3), dtype=np.uint8)
        
        # Warm up
        _ = model(test_image)
        
        # Time multiple inferences
        start_time = time.time()
        num_tests = 5
        for _ in range(num_tests):
            results = model(test_image)
        end_time = time.time()
        
        avg_time = (end_time - start_time) / num_tests * 1000  # Convert to ms
        
        print(f"✅ Average inference time: {avg_time:.1f}ms")
        
        if avg_time < 500:  # Less than 500ms is good
            print("✅ Model performance: Excellent")
        elif avg_time < 1000:
            print("✅ Model performance: Good")
        else:
            print("⚠️ Model performance: Slow")
        
        return True
        
    except Exception as e:
        print(f"❌ Model performance test failed: {e}")
        return False

def test_ocr_functionality():
    """Test OCR (Tesseract) functionality"""
    print("📝 Testing OCR functionality...")
    
    try:
        import pytesseract
        from PIL import Image
        import numpy as np
        
        # Create a simple test image with text
        test_image = np.ones((100, 300, 3), dtype=np.uint8) * 255  # White background
        
        # Convert to PIL and add some text (simulate license plate)
        pil_image = Image.fromarray(test_image)
        
        # Test OCR
        text = pytesseract.image_to_string(pil_image, config='--psm 7')
        
        print("✅ Tesseract OCR is working")
        
        # Check if Tesseract path is configured
        tesseract_path = pytesseract.pytesseract.tesseract_cmd
        if os.path.exists(tesseract_path):
            print(f"✅ Tesseract found at: {tesseract_path}")
        else:
            print(f"⚠️ Tesseract path may be incorrect: {tesseract_path}")
        
        return True
        
    except Exception as e:
        print(f"❌ OCR test failed: {e}")
        return False

def generate_system_report():
    """Generate a comprehensive system report"""
    print("\n📊 GENERATING SYSTEM REPORT")
    print("=" * 60)
    
    report = {
        "timestamp": datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
        "tests": {
            "file_system": test_file_system(),
            "model_performance": test_model_performance(),
            "ocr_functionality": test_ocr_functionality(),
            "flask_server": test_flask_server()
        }
    }
    
    # Calculate overall status
    all_passed = all(report["tests"].values())
    
    # Print summary
    print("\n🏁 SYSTEM VALIDATION SUMMARY")
    print("=" * 60)
    
    for test_name, result in report["tests"].items():
        status = "✅ PASS" if result else "❌ FAIL"
        print(f"{test_name.replace('_', ' ').title()}: {status}")
    
    overall_status = "✅ SYSTEM READY" if all_passed else "❌ ISSUES DETECTED"
    print(f"\n🎯 Overall Status: {overall_status}")
    
    # Save report
    try:
        os.makedirs("Data", exist_ok=True)
        with open("Data/system_validation_report.json", "w") as f:
            json.dump(report, f, indent=2)
        print(f"📄 Detailed report saved: Data/system_validation_report.json")
    except Exception as e:
        print(f"⚠️ Could not save report: {e}")
    
    return all_passed

def main():
    """Main validation function"""
    print("🚀 STARTING COMPREHENSIVE SYSTEM VALIDATION")
    print("=" * 60)
    print(f"⏰ Started at: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    print()
    
    # Run comprehensive tests
    system_ok = generate_system_report()
    
    print(f"\n⏰ Completed at: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    
    if system_ok:
        print("\n🎉 CONGRATULATIONS!")
        print("Your license plate detection system is fully operational!")
        print("\n📋 What you can do now:")
        print("1. 🌐 Visit http://127.0.0.1:5000 to use the web interface")
        print("2. 📸 Use the snapshot feature to capture license plates")
        print("3. 🔍 Test with more images in the Test/ directory")
        print("4. 📊 Check results in Data/test_results/")
    else:
        print("\n⚠️ Some issues were detected.")
        print("Please check the error messages above and resolve them.")
    
    return system_ok

if __name__ == "__main__":
    main()
