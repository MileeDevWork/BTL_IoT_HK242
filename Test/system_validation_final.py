#!/usr/bin/env python3
"""
Final System Validation Script for Enhanced License Plate Detection System
This script validates that the camera snapshot issue has been resolved.
"""

import requests
import time
import json
import os
from datetime import datetime

def test_server_status():
    """Test if server is running and camera is operational"""
    print("🔍 Testing server status...")
    try:
        response = requests.get('http://127.0.0.1:5000/status', timeout=5)
        if response.status_code == 200:
            status = response.json()
            print(f"   ✅ Server responding: {response.status_code}")
            print(f"   📷 Camera Open: {status['camera_open']}")
            print(f"   🔢 Camera Index: {status['camera_index']}")
            return True
        else:
            print(f"   ❌ Server error: {response.status_code}")
            return False
    except Exception as e:
        print(f"   ❌ Connection failed: {e}")
        return False

def test_snapshot_functionality():
    """Test snapshot functionality without interrupting camera"""
    print("\n📸 Testing snapshot functionality...")
    
    snapshots_taken = []
    
    for i in range(3):
        try:
            print(f"   Taking snapshot {i+1}/3...")
            response = requests.get('http://127.0.0.1:5000/snapshot?flag=1', timeout=10)
            
            if response.status_code == 200:
                size = len(response.content)
                snapshots_taken.append(size)
                print(f"     ✅ Success: {size:,} bytes")
            else:
                print(f"     ❌ Failed: Status {response.status_code}")
                
        except Exception as e:
            print(f"     ❌ Error: {e}")
            
        # Short delay between snapshots
        time.sleep(1)
    
    return snapshots_taken

def test_camera_continuity():
    """Test that camera continues working after snapshots"""
    print("\n🎥 Testing camera continuity...")
    
    try:
        response = requests.get('http://127.0.0.1:5000/status', timeout=5)
        if response.status_code == 200:
            status = response.json()
            if status['camera_open']:
                print("   ✅ Camera operational after snapshots!")
                return True
            else:
                print("   ❌ Camera not operational after snapshots!")
                return False
        else:
            print(f"   ❌ Status check failed: {response.status_code}")
            return False
    except Exception as e:
        print(f"   ❌ Error checking camera status: {e}")
        return False

def check_saved_images():
    """Check if license plate images are being saved"""
    print("\n💾 Checking saved license plate images...")
    
    image_dir = os.path.join(os.path.dirname(__file__), "Data", "image")
    
    if os.path.exists(image_dir):
        images = [f for f in os.listdir(image_dir) if f.endswith('.jpg')]
        print(f"   📁 Image directory: {image_dir}")
        print(f"   🖼️  Saved images: {len(images)}")
        
        if images:
            for img in sorted(images):
                img_path = os.path.join(image_dir, img)
                size = os.path.getsize(img_path)
                print(f"     - {img} ({size:,} bytes)")
        else:
            print("     ℹ️  No license plate images detected in current camera view")
        
        return len(images)
    else:
        print(f"   ❌ Image directory not found: {image_dir}")
        return 0

def generate_final_report():
    """Generate final validation report"""
    print("\n" + "="*60)
    print("🎯 FINAL SYSTEM VALIDATION REPORT")
    print("="*60)
    
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    print(f"📅 Validation Time: {timestamp}")
    
    # Run all tests
    server_ok = test_server_status()
    snapshots = test_snapshot_functionality()
    camera_ok = test_camera_continuity()
    saved_images = check_saved_images()
    
    print("\n📊 RESULTS SUMMARY:")
    print(f"   🌐 Server Status: {'✅ PASS' if server_ok else '❌ FAIL'}")
    print(f"   📸 Snapshots Taken: {len(snapshots)}/3")
    print(f"   🎥 Camera Continuity: {'✅ PASS' if camera_ok else '❌ FAIL'}")
    print(f"   💾 Saved Images: {saved_images}")
    
    if snapshots:
        avg_size = sum(snapshots) / len(snapshots)
        print(f"   📏 Avg Snapshot Size: {avg_size:,.0f} bytes")
    
    # Overall assessment
    print(f"\n🏆 OVERALL STATUS:")
    if server_ok and len(snapshots) >= 2 and camera_ok:
        print("   ✅ SYSTEM FULLY OPERATIONAL")
        print("   🎉 Snapshot issue has been RESOLVED!")
        print("   📷 Camera continues running after snapshots")
        success = True
    else:
        print("   ❌ SYSTEM ISSUES DETECTED")
        success = False
    
    # Save report
    report_data = {
        "timestamp": timestamp,
        "server_status": server_ok,
        "snapshots_taken": len(snapshots),
        "snapshot_sizes": snapshots,
        "camera_continuity": camera_ok,
        "saved_images_count": saved_images,
        "system_operational": success,
        "issue_resolved": success
    }
    
    report_file = "system_validation_final_report.json"
    with open(report_file, 'w') as f:
        json.dump(report_data, f, indent=2)
    
    print(f"\n📄 Report saved to: {report_file}")
    print("="*60)
    
    return success

if __name__ == "__main__":
    print("🚀 Starting Final System Validation...")
    print("🎯 Objective: Verify snapshot functionality without camera interruption")
    
    success = generate_final_report()
    
    if success:
        print("\n🎊 VALIDATION COMPLETE - SYSTEM WORKING PERFECTLY!")
    else:
        print("\n⚠️  VALIDATION COMPLETE - ISSUES NEED ATTENTION")
