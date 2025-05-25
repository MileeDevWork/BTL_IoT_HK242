# filepath: c:\Users\Dell\Desktop\Study\Allproject\BTL_IOT\Test\camera_fixed.py
import warnings
warnings.filterwarnings('ignore')

# Import necessary libraries
import os
import shutil
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import random
import cv2
import yaml
import torch
import pytesseract
from PIL import Image
from ultralytics import YOLO
from collections import deque
import subprocess  # For executing ffmpeg
# from IPython.display import display  # Import display for showing images
import glob  # Import glob for file pattern matching
from datetime import datetime  # For timestamp generation

# Define dataset paths relative to script directory to create absolute paths
script_dir = os.path.dirname(os.path.abspath(__file__))
train_dir = os.path.join(script_dir, "Data", "license-plate-dataset", "images", "train")
val_dir = os.path.join(script_dir, "Data", "license-plate-dataset", "images", "val")

# Define class names
classes = ["license_plate"]  # Add more classes if needed

pytesseract.pytesseract.tesseract_cmd = r'C:/Program Files/Tesseract-OCR/tesseract.exe'

# Load the trained YOLOv8 model (assuming you have already trained it)
model_path = "runs/detect/train7/weights/best.pt" # Replace with your model path
model = YOLO(model_path)

# Example of processing a single image and extracting text
def extract_plate_text(image_path, model):
    """
    Detects license plates in an image using a YOLO model and extracts text using OCR.

    Args:
        image_path (str): The path to the input image.
        model: The trained YOLO model.

    Returns:
        list: A list of extracted license plate texts.
    """
    extracted_texts = []
    results = model(image_path)

    for result in results:
        if result.boxes is not None:
            for box in result.boxes:
                x1, y1, x2, y2 = map(int, box.xyxy[0])
                conf = box.conf[0].item()

                # Crop the license plate region
                cropped_plate = cv2.imread(image_path)[y1:y2, x1:x2]

                # Convert the cropped image to grayscale for better OCR results
                gray_plate = cv2.cvtColor(cropped_plate, cv2.COLOR_BGR2GRAY)

                # Use pytesseract to extract text
                try:
                    text = pytesseract.image_to_string(gray_plate, config='--psm 7') # psm 7 is good for single text line
                    extracted_texts.append(text.strip())
                except Exception as e:
                    print(f"Error during OCR: {e}")
                    extracted_texts.append("") # Append empty string if OCR fails

    return extracted_texts

# VideoCamera class for Flask streaming
class VideoCamera:
    def __init__(self, camera_index=0):
        """
        Initialize the VideoCamera with camera index.
        
        Args:
            camera_index (int): Index of the camera to use (0 for default camera)
        """
        self.camera_index = camera_index
        self.cap = None
        self.model = model  # Use the globally loaded model
        self.save_dir = os.path.join(script_dir, "Data", "image")
        self.is_open = False
        
        # Create save directory if it doesn't exist
        os.makedirs(self.save_dir, exist_ok=True)
        
    def open_camera(self):
        """Open camera connection"""
        if not self.is_open:
            self.cap = cv2.VideoCapture(self.camera_index)
            if self.cap.isOpened():
                self.is_open = True
                return True
            else:
                return False
        return True
        
    def close_camera(self):
        """Close camera connection"""
        if self.cap and self.is_open:
            self.cap.release()
            self.is_open = False
            
    def __enter__(self):
        """Context manager entry"""
        self.open_camera()
        if not self.is_open:
            raise RuntimeError(f"Could not open camera {self.camera_index}")
        return self
        
    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager exit"""
        self.close_camera()
            
    def get_frame(self, save_plate=False, crop_vehicle=False):
        """
        Capture a frame from the camera and optionally detect license plates.
        
        Args:
            save_plate (bool): Whether to save detected license plate images
            crop_vehicle (bool): Whether to return cropped vehicle image
            
        Returns:
            bytes: JPEG encoded frame
        """
        # Ensure camera is open
        if not self.is_open:
            if not self.open_camera():
                return None
            
        if not self.cap.isOpened():
            return None
            
        ret, frame = self.cap.read()
        if not ret:
            return None
            
        # Detect license plates in the frame
        results = self.model(frame)
        
        # Draw bounding boxes and extract plates if needed
        for result in results:
            if result.boxes is not None:
                for box in result.boxes:
                    x1, y1, x2, y2 = map(int, box.xyxy[0])
                    conf = box.conf[0].item()
                    
                    # Only process detections with confidence > 0.5
                    if conf > 0.5:
                        # Draw bounding box
                        cv2.rectangle(frame, (x1, y1), (x2, y2), (0, 255, 0), 2)
                        cv2.putText(frame, f'License Plate: {conf:.2f}', 
                                  (x1, y1-10), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2)
                        
                        if save_plate:
                            # Crop the license plate region
                            cropped_plate = frame[y1:y2, x1:x2]
                            
                            # Extract text from the license plate
                            try:
                                gray_plate = cv2.cvtColor(cropped_plate, cv2.COLOR_BGR2GRAY)
                                text = pytesseract.image_to_string(gray_plate, config='--psm 7').strip()
                                
                                # Save the cropped license plate with timestamp
                                timestamp = datetime.now().strftime("%Y%m%d_%H%M%S_%f")[:-3]  # Include milliseconds
                                filename = f"license_plate_{timestamp}.jpg"
                                save_path = os.path.join(self.save_dir, filename)
                                cv2.imwrite(save_path, cropped_plate)
                                
                                # Print detected text
                                if text:
                                    print(f"📸 Detected license plate text: {text}")
                                    print(f"💾 Saved to: {filename}")
                                    # Add text to the frame
                                    cv2.putText(frame, f'Text: {text}', 
                                              (x1, y2+20), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 0, 0), 2)
                                else:
                                    print(f"📸 License plate detected but no text extracted")
                                    print(f"💾 Saved to: {filename}")
                                
                            except Exception as e:
                                print(f"❌ Error during OCR: {e}")
        
        # If crop_vehicle is True and we're saving, return the cropped vehicle region
        if crop_vehicle and save_plate:
            # For now, we'll return the detected license plate region
            # You can modify this to crop the entire vehicle if needed
            for result in results:
                if result.boxes is not None and len(result.boxes) > 0:
                    box = result.boxes[0]  # Take the first detection
                    x1, y1, x2, y2 = map(int, box.xyxy[0])
                    conf = box.conf[0].item()
                    
                    if conf > 0.5:
                        # Expand the crop area to include more of the vehicle
                        h, w = frame.shape[:2]
                        margin = 50
                        x1 = max(0, x1 - margin)
                        y1 = max(0, y1 - margin)
                        x2 = min(w, x2 + margin)
                        y2 = min(h, y2 + margin)
                        
                        cropped_frame = frame[y1:y2, x1:x2]
                        ret, jpeg = cv2.imencode('.jpg', cropped_frame)
                        return jpeg.tobytes()
        
        # Encode frame as JPEG
        ret, jpeg = cv2.imencode('.jpg', frame)
        if ret:
            return jpeg.tobytes()
        else:
            return None
            
    def __del__(self):
        """Destructor to release camera"""
        self.close_camera()

# Function to extract text from image frame (for direct use)
def extract_plate_text_from_frame(frame, model):
    """
    Detects license plates in a frame and extracts text using OCR.
    
    Args:
        frame: OpenCV image frame
        model: The trained YOLO model
        
    Returns:
        list: A list of extracted license plate texts
    """
    extracted_texts = []
    results = model(frame)
    
    for result in results:
        if result.boxes is not None:
            for box in result.boxes:
                x1, y1, x2, y2 = map(int, box.xyxy[0])
                conf = box.conf[0].item()
                
                if conf > 0.5:  # Only process high confidence detections
                    # Crop the license plate region
                    cropped_plate = frame[y1:y2, x1:x2]
                    
                    # Convert to grayscale for better OCR
                    gray_plate = cv2.cvtColor(cropped_plate, cv2.COLOR_BGR2GRAY)
                    
                    # Use pytesseract to extract text
                    try:
                        text = pytesseract.image_to_string(gray_plate, config='--psm 7')
                        extracted_texts.append(text.strip())
                    except Exception as e:
                        print(f"Error during OCR: {e}")
                        extracted_texts.append("")
    
    return extracted_texts

# --- Example Usage ---
if __name__ == "__main__":
    # Test with static images
    all_images = glob.glob("Test/*.jpg")
    
    if all_images:
        # Select a few images for testing
        test_images = random.sample(all_images, min(3, len(all_images)))
        
        for img_path in test_images:
            print(f"Processing image: {os.path.basename(img_path)}")
            plate_texts = extract_plate_text(img_path, model)
            
            if plate_texts:
                print(f"Detected plate text(s): {', '.join(plate_texts)}")
            else:
                print("No license plates detected or text extracted.")
            print("-" * 30)
    
    # Test with camera (uncomment to test)
    # print("Testing camera...")
    # try:
    #     with VideoCamera() as cam:
    #         for i in range(10):  # Capture 10 frames
    #             frame = cam.get_frame(save_plate=True)
    #             if frame:
    #                 print(f"Captured frame {i+1}")
    #             else:
    #                 print(f"Failed to capture frame {i+1}")
    # except Exception as e:
    #     print(f"Camera test failed: {e}")
