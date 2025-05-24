import requests
import os
from datetime import datetime

def capture_snapshot(save_dir: str = "Data/image"):
    # Tạo thư mục nếu chưa tồn tại
    os.makedirs(save_dir, exist_ok=True)
    
    url = "http://127.0.0.1:5000/snapshot"
    params = {"flag": 1, "crop": 1}  # Thêm tham số crop=1 để nhận ảnh đã cắt

    try:
        resp = requests.get(url, params=params, timeout=5)
        resp.raise_for_status()
    except Exception as e:
        print(f"Error requesting snapshot: {e}")
        return

    if resp.headers.get("Content-Type") != "image/jpeg":
        print(f"Unexpected content type: {resp.headers.get('Content-Type')}")
        return

    # Đặt tên file theo timestamp
    filename = datetime.now().strftime("%Y%m%d_%H%M%S") + ".jpg"
    filepath = os.path.join(save_dir, filename)

    with open(filepath, "wb") as f:
        f.write(resp.content)

    print(f"Snapshot saved to {filepath}")

if __name__ == "__main__":
    capture_snapshot()