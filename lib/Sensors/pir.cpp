#include <sensor.hpp>

// Task đọc cảm biến chuyển động PIR
void pirTask(void *pvParameters) {
    // Khởi tạo cảm biến PIR
    pinMode(PIR_PIN, INPUT);
    bool previousMotionState = false;
    unsigned long lastDetectionTime = 0;
    const unsigned long MOTION_TIMEOUT = 30000; // 30 giây timeout cho trạng thái chuyển động
    
    // Chờ cảm biến PIR ổn định (thường cần khoảng 60 giây)
    Serial.println("Khởi tạo cảm biến PIR...");
    vTaskDelay(pdMS_TO_TICKS(10000)); // Đợi 10 giây
    Serial.println("Cảm biến PIR đã sẵn sàng");
    
    for (;;) {
        // Đọc trạng thái cảm biến PIR
        bool currentMotionState = digitalRead(PIR_PIN);
        unsigned long currentTime = millis();
        
        // Phát hiện chuyển động
        if (currentMotionState == HIGH) {
            lastDetectionTime = currentTime;
            
            // Nếu trạng thái thay đổi từ không có chuyển động sang có chuyển động
            if (!previousMotionState) {
                Serial.println("Phát hiện chuyển động!");
                motionDetected = true;
                
                // Gửi dữ liệu lên ThingsBoard
                if (tb.connected()) {
                    tb.sendTelemetryData("motion", "true");
                    Serial.println("Đã gửi thông báo chuyển động lên ThingsBoard");
                }
            }
            previousMotionState = true;
        } 
        // Không có chuyển động hoặc hết thời gian timeout
        else if (currentMotionState == LOW || (currentTime - lastDetectionTime > MOTION_TIMEOUT)) {
            // Nếu trạng thái thay đổi từ có chuyển động sang không có chuyển động
            if (previousMotionState) {
                Serial.println("Không phát hiện chuyển động");
                motionDetected = false;
                
                // Gửi dữ liệu lên ThingsBoard
                if (tb.connected()) {
                    tb.sendTelemetryData("motion", "false");
                    Serial.println("Đã gửi thông báo không có chuyển động lên ThingsBoard");
                }
            }
            previousMotionState = false;        }
        
        // Cập nhật mỗi 5 giây theo yêu cầu
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
