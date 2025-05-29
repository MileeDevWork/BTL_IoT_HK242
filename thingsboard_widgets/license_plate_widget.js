// ThingsBoard Widget JavaScript Controller
self.onInit = function() {
    // Widget configuration
    self.ctx.$scope.serverUrl = self.ctx.settings.serverUrl || 'http://127.0.0.1:5000';
    self.ctx.$scope.enableAutoRefresh = self.ctx.settings.enableAutoRefresh !== false;
    self.ctx.$scope.refreshInterval = self.ctx.settings.refreshInterval || 30000;
    self.ctx.$scope.mqttBroker = self.ctx.settings.mqttBroker || 'wss://test.mosquitto.org:8081/mqtt';
    self.ctx.$scope.mqttTopic = self.ctx.settings.mqttTopic || 'yolouno/rfid/response';
    
    // Initialize widget
    initializeWidget();
    
    // Setup MQTT connection for NFC detection
    setupMQTTConnection();
    
    // Setup auto refresh
    if (self.ctx.$scope.enableAutoRefresh) {
        self.ctx.$scope.refreshTimer = setInterval(function() {
            loadRecentPlates();
            checkSystemStatus();
        }, self.ctx.$scope.refreshInterval);
    }
    
    // Load initial data
    loadRecentPlates();
    checkSystemStatus();
    
    // Check auto-save status on widget initialization
    checkAutoSaveStatus();
    
    console.log('License Plate Widget initialized with MQTT support');
};

self.onDestroy = function() {
    // Clean up MQTT connection
    if (self.ctx.$scope.mqttClient) {
        self.ctx.$scope.mqttClient.end();
    }
    
    // Clean up refresh timer
    if (self.ctx.$scope.refreshTimer) {
        clearInterval(self.ctx.$scope.refreshTimer);
    }
};

function initializeWidget() {
    // Get DOM elements
    const snapshotBtn = self.ctx.$container.find('#snapshot-btn')[0];
    const extractBtn = self.ctx.$container.find('#extract-plate-btn')[0];
    const savePlateBtn = self.ctx.$container.find('#save-plate-btn')[0];
    const cancelEditBtn = self.ctx.$container.find('#cancel-edit-btn')[0];
    const plateInput = self.ctx.$container.find('#plate-input')[0];
    
    // Setup event listeners
    if (snapshotBtn) {
        snapshotBtn.addEventListener('click', takeSnapshot);
    }
    
    if (extractBtn) {
        extractBtn.addEventListener('click', extractLicensePlate);
    }
    
    if (savePlateBtn) {
        savePlateBtn.addEventListener('click', savePlateInfo);
    }
    
    if (cancelEditBtn) {
        cancelEditBtn.addEventListener('click', cancelEdit);
    }
    
    if (plateInput) {
        plateInput.addEventListener('input', function(e) {
            e.target.value = e.target.value.toUpperCase();
        });
    }
    
    // Setup video stream error handling
    const videoStream = self.ctx.$container.find('#video-stream')[0];
    if (videoStream) {
        videoStream.addEventListener('error', function() {
            updateCameraStatus('offline', 'Camera không khả dụng');
        });
        
        videoStream.addEventListener('load', function() {
            updateCameraStatus('online', 'Live');
        });
    }
}

function takeSnapshot() {
    const btn = self.ctx.$container.find('#snapshot-btn')[0];
    if (!btn) return;
    
    btn.disabled = true;
    btn.innerHTML = '<span class="loading-spinner"></span> Đang chụp...';
    
    fetch(self.ctx.$scope.serverUrl + '/snapshot?flag=1&crop=1')
        .then(response => {
            if (!response.ok) throw new Error('Lỗi chụp ảnh');
            return response.blob();
        })
        .then(blob => {
            // Create download link
            const url = URL.createObjectURL(blob);
            const a = document.createElement('a');
            a.href = url;
            a.download = `snapshot_${new Date().getTime()}.jpg`;
            a.click();
            
            showStatus('✅ Đã chụp ảnh thành công!', 'success');
        })
        .catch(error => {
            console.error('Error:', error);
            showStatus('❌ Lỗi chụp ảnh: ' + error.message, 'error');
        })
        .finally(() => {
            btn.disabled = false;
            btn.innerHTML = '📸 Chụp ảnh';
        });
}

function extractLicensePlate() {
    const btn = self.ctx.$container.find('#extract-plate-btn')[0];
    if (!btn) return;
    
    btn.disabled = true;
    btn.innerHTML = '<span class="loading-spinner"></span> Đang xử lý...';
    
    fetch(self.ctx.$scope.serverUrl + '/snapshot?flag=1&crop=1&extract_plate=1')
        .then(response => response.json())
        .then(data => {
            if (data.success) {
                showPlateEditSection(data);
                showStatus('✅ Đã trích xuất biển số thành công!', 'success');
            } else {
                showStatus('⚠️ ' + (data.error || 'Không trích xuất được biển số'), 'warning');
            }
        })
        .catch(error => {
            console.error('Error:', error);
            showStatus('❌ Lỗi trích xuất biển số: ' + error.message, 'error');
        })
        .finally(() => {
            btn.disabled = false;
            btn.innerHTML = '🔍 Trích xuất biển số';
        });
}

function showPlateEditSection(data) {
    const editSection = self.ctx.$container.find('#plate-edit-section')[0];
    const extractionInfo = self.ctx.$container.find('#extraction-info')[0];
    const plateInput = self.ctx.$container.find('#plate-input')[0];
    const uidInput = self.ctx.$container.find('#uid-input')[0];
    const capturedImage = self.ctx.$container.find('#captured-image')[0];
    
    if (!editSection) return;
    
    // Show edit section
    editSection.style.display = 'block';
    
    // Set captured image
    fetch(self.ctx.$scope.serverUrl + '/snapshot?flag=1&crop=1')
        .then(response => response.blob())
        .then(blob => {
            const imageUrl = URL.createObjectURL(blob);
            if (capturedImage) {
                capturedImage.src = imageUrl;
            }
        });
    
    // Pre-fill UID from NFC data
    if (uidInput && data.uid) {
        uidInput.value = data.uid;
    }
    
    // Set extraction info
    if (extractionInfo) {
        if (data.success && data.license_plate) {
            const userInfo = data.userName ? ` cho ${data.userName}` : '';
            extractionInfo.innerHTML = `
                <div style="background: #d4edda; color: #155724; padding: 10px; border-radius: 6px; border: 1px solid #c3e6cb;">
                    <strong>✅ Tự động nhận diện${userInfo}:</strong> ${data.license_plate}<br>
                    ${data.uid ? `<small>UID: ${data.uid}</small><br>` : ''}
                    <small>Thời gian: ${new Date(data.timestamp || Date.now()).toLocaleString('vi-VN')}</small>
                </div>
            `;
            if (plateInput) {
                plateInput.value = data.license_plate;
            }
        } else {
            const userInfo = data.userName ? ` cho ${data.userName}` : '';
            extractionInfo.innerHTML = `
                <div style="background: #fff3cd; color: #856404; padding: 10px; border-radius: 6px; border: 1px solid #ffeaa7;">
                    <strong>⚠️ Không nhận diện được tự động${userInfo}</strong><br>
                    ${data.uid ? `<small>UID: ${data.uid}</small><br>` : ''}
                    Vui lòng nhập thủ công
                </div>
            `;
        }
    }
    
    // Scroll to edit section
    editSection.scrollIntoView({ behavior: 'smooth' });
}

function savePlateInfo() {
    const plateInput = self.ctx.$container.find('#plate-input')[0];
    const uidInput = self.ctx.$container.find('#uid-input')[0];
    const btn = self.ctx.$container.find('#save-plate-btn')[0];
    
    if (!plateInput || !btn) return;
    
    const plateNumber = plateInput.value.trim().toUpperCase();
    const uid = uidInput ? uidInput.value.trim() : '';
    
    if (!plateNumber) {
        showStatus('❌ Vui lòng nhập biển số xe', 'error');
        return;
    }
    
    btn.disabled = true;
    btn.innerHTML = '<span class="loading-spinner"></span> Đang lưu...';
    
    const data = {
        license_plate: plateNumber,
        uid: uid,
        timestamp: new Date().toISOString(),
        source: 'thingsboard_widget'
    };
    
    fetch(self.ctx.$scope.serverUrl + '/vehicle/save_plate_info', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
        },
        body: JSON.stringify(data)
    })
    .then(response => response.json())
    .then(result => {
        if (result.success) {
            showStatus(`✅ Đã lưu biển số: ${plateNumber}`, 'success');
            cancelEdit();
            loadRecentPlates();
        } else {
            showStatus('❌ Lỗi lưu thông tin: ' + result.error, 'error');
        }
    })
    .catch(error => {
        console.error('Error:', error);
        showStatus('❌ Lỗi kết nối: ' + error.message, 'error');
    })
    .finally(() => {
        btn.disabled = false;
        btn.innerHTML = '💾 Lưu';
    });
}

function cancelEdit() {
    const editSection = self.ctx.$container.find('#plate-edit-section')[0];
    const plateInput = self.ctx.$container.find('#plate-input')[0];
    const uidInput = self.ctx.$container.find('#uid-input')[0];
    
    if (editSection) {
        editSection.style.display = 'none';
    }
    
    if (plateInput) {
        plateInput.value = '';
    }
    
    if (uidInput) {
        uidInput.value = '';
    }
}

function loadRecentPlates() {
    fetch(self.ctx.$scope.serverUrl + '/vehicle/recent_plates')
        .then(response => response.json())
        .then(data => {
            const listContainer = self.ctx.$container.find('#recent-plates-list')[0];
            if (!listContainer) return;
            
            if (data.success && data.plates.length > 0) {
                let html = '';
                data.plates.slice(0, 5).forEach(plate => {
                    // Sử dụng processed_at cho thời gian hiển thị
                    const time = new Date(plate.processed_at).toLocaleString('vi-VN');
                    
                    // Xác định nguồn biển số
                    let source = 'Tự động';
                    if (plate.source === 'manual_edit' || plate.source === 'thingsboard_widget') {
                        source = 'Thủ công';
                    }
                    
                    // Xác định trạng thái xe
                    let statusBadge = '';
                    if (plate.status === 'inside') {
                        statusBadge = '<span style="background: #28a745; color: white; padding: 2px 6px; border-radius: 10px; font-size: 11px; margin-left: 5px;">Trong bãi</span>';
                    } else if (plate.status === 'completed') {
                        statusBadge = '<span style="background: #6c757d; color: white; padding: 2px 6px; border-radius: 10px; font-size: 11px; margin-left: 5px;">Đã ra</span>';
                    } else if (plate.status === 'manual_entry') {
                        statusBadge = '<span style="background: #17a2b8; color: white; padding: 2px 6px; border-radius: 10px; font-size: 11px; margin-left: 5px;">Thủ công</span>';
                    }
                    
                    // Hiển thị trạng thái khớp biển số nếu có
                    let matchBadge = '';
                    if (plate.match_status === 'match') {
                        matchBadge = '<span style="background: #28a745; color: white; padding: 2px 6px; border-radius: 10px; font-size: 11px; margin-left: 5px;">Khớp</span>';
                    } else if (plate.match_status === 'mismatch') {
                        matchBadge = '<span style="background: #dc3545; color: white; padding: 2px 6px; border-radius: 10px; font-size: 11px; margin-left: 5px;">Không khớp</span>';
                    }
                    
                    html += `
                        <div class="plate-item">
                            <div>
                                <strong style="color: #007bff;">${plate.license_plate}</strong>
                                ${plate.uid ? `<br><small>UID: ${plate.uid}</small>` : ''}
                            </div>
                            <div style="text-align: right;">
                                <small style="color: #6c757d;">${time}</small><br>
                                <span style="background: #e9ecef; padding: 2px 6px; border-radius: 10px; font-size: 11px;">${source}</span>
                                ${statusBadge}
                                ${matchBadge}
                            </div>
                        </div>
                    `;
                });
                listContainer.innerHTML = html;
            } else {
                listContainer.innerHTML = `
                    <div style="text-align: center; color: #6c757d; padding: 20px;">
                        <i class="material-icons" style="font-size: 48px; opacity: 0.5;">search</i>
                        <p style="margin: 10px 0 0 0;">Chưa có dữ liệu</p>
                    </div>
                `;
            }
        })
        .catch(error => {
            console.error('Error loading recent plates:', error);
            const listContainer = self.ctx.$container.find('#recent-plates-list')[0];
            if (listContainer) {
                listContainer.innerHTML = `
                    <div style="text-align: center; color: #dc3545; padding: 20px;">
                        <i class="material-icons" style="font-size: 48px; opacity: 0.5;">error</i>
                        <p style="margin: 10px 0 0 0;">Lỗi tải dữ liệu</p>
                    </div>
                `;
            }
        });
}

function checkSystemStatus() {
    fetch(self.ctx.$scope.serverUrl + '/status')
        .then(response => response.json())
        .then(data => {
            const status = data.camera_open ? 'online' : 'offline';
            const text = data.camera_open ? 'Live' : 'Offline';
            updateCameraStatus(status, text);
        })
        .catch(error => {
            console.error('Error checking system status:', error);
            updateCameraStatus('offline', 'Error');
        });
}

function updateCameraStatus(status, text) {
    const indicator = self.ctx.$container.find('#status-indicator')[0];
    const statusText = self.ctx.$container.find('#status-text')[0];
    
    if (indicator) {
        indicator.style.background = status === 'online' ? '#28a745' : '#dc3545';
    }
    
    if (statusText) {
        statusText.textContent = text;
    }
}

function showStatus(message, type = 'info') {
    const statusContainer = self.ctx.$container.find('#status-messages')[0];
    if (!statusContainer) return;
    
    const colors = {
        success: { bg: '#d4edda', border: '#c3e6cb', color: '#155724' },
        error: { bg: '#f8d7da', border: '#f5c6cb', color: '#721c24' },
        warning: { bg: '#fff3cd', border: '#ffeaa7', color: '#856404' },
        info: { bg: '#d1ecf1', border: '#bee5eb', color: '#0c5460' }
    };
    
    const color = colors[type] || colors.info;
    
    statusContainer.innerHTML = `
        <div style="background: ${color.bg}; color: ${color.color}; padding: 10px; border-radius: 6px; border: 1px solid ${color.border}; margin-bottom: 10px;">
            ${message}
        </div>
    `;
    
    // Auto-hide after 5 seconds
    setTimeout(() => {
        if (statusContainer) {
            statusContainer.innerHTML = '';
        }
    }, 5000);
}

// MQTT Integration Functions
function setupMQTTConnection() {
    // Load MQTT library dynamically
    if (typeof mqtt === 'undefined') {
        const script = document.createElement('script');
        script.src = 'https://unpkg.com/mqtt/dist/mqtt.min.js';
        script.onload = function() {
            initMQTTClient();
        };
        document.head.appendChild(script);
    } else {
        initMQTTClient();
    }
}

function initMQTTClient() {
    try {
        // Connect to MQTT broker
        self.ctx.$scope.mqttClient = mqtt.connect(self.ctx.$scope.mqttBroker);
          self.ctx.$scope.mqttClient.on('connect', function() {
            console.log('✅ Widget connected to MQTT broker');
            showStatus('🔗 Đã kết nối MQTT - Chờ quét NFC', 'success');
            
            // Subscribe to RFID response topic
            self.ctx.$scope.mqttClient.subscribe(self.ctx.$scope.mqttTopic, function(err) {
                if (!err) {
                    console.log(`📡 Subscribed to topic: ${self.ctx.$scope.mqttTopic}`);
                } else {
                    console.error('❌ Failed to subscribe to MQTT topic:', err);
                }
            });
            
            // Subscribe to scan topics to receive direct NFC messages
            self.ctx.$scope.mqttClient.subscribe('yolouno/rfid/scan/in', function(err) {
                if (!err) {
                    console.log('📡 Subscribed to scan/in topic');
                } else {
                    console.error('❌ Failed to subscribe to scan/in topic:', err);
                }
            });
            
            self.ctx.$scope.mqttClient.subscribe('yolouno/rfid/scan/out', function(err) {
                if (!err) {
                    console.log('📡 Subscribed to scan/out topic');
                } else {
                    console.error('❌ Failed to subscribe to scan/out topic:', err);
                }
            });
        });
        
        self.ctx.$scope.mqttClient.on('message', function(topic, message) {
            handleMQTTMessage(topic, message);
        });
        
        self.ctx.$scope.mqttClient.on('error', function(error) {
            console.error('❌ MQTT connection error:', error);
            showStatus('❌ Lỗi kết nối MQTT', 'error');
        });
        
        self.ctx.$scope.mqttClient.on('close', function() {
            console.log('🔌 MQTT connection closed');
            showStatus('🔌 Mất kết nối MQTT', 'warning');
        });
        
    } catch (error) {
        console.error('❌ Error setting up MQTT:', error);
        showStatus('❌ Không thể kết nối MQTT', 'error');
    }
}

function handleMQTTMessage(topic, message) {
    try {
        console.log(`📨 Received MQTT message from topic: ${topic}`);
        
        // Kiểm tra nếu là tin nhắn quét thẻ trực tiếp
        if (topic === 'yolouno/rfid/scan/in' || topic === 'yolouno/rfid/scan/out') {
            // Xử lý tin nhắn quét thẻ trực tiếp
            handleScanMessage(topic, message);
            return;
        }
        
        // Xử lý tin nhắn phản hồi từ server
        const data = JSON.parse(message.toString());
        console.log('MQTT message content:', data);
        
        // Check if this is a relevant RFID message by looking for uid field
        if (!data.uid) {
            console.log('Skipping message without UID');
            return;
        }
        
        // Show status regardless of allowed status to provide feedback
        if (data.allowed === true) {
            // Log detection
            const userName = data.name || 'Không xác định';
            const scanType = data.scan_type === 'exit' ? 'ra' : 'vào';
            showStatus(`🎯 NFC phát hiện: ${userName} (${data.uid}) - Quét ${scanType}`, 'info');
            
            // Store UID for later use
            self.ctx.$scope.currentUID = data.uid;
            self.ctx.$scope.currentUserName = data.name;
            
            // Check if license_plate is already in the message (might be provided by RFID server)
            if (data.license_plate) {
                showStatus(`🚗 Biển số xe đã được phát hiện: ${data.license_plate}`, 'success');
                
                // Show plate edit section with pre-filled data
                showPlateEditSection({
                    success: true,
                    license_plate: data.license_plate,
                    uid: data.uid,
                    userName: data.name,
                    timestamp: data.timestamp || new Date().toISOString()
                });
            } else {
                // Need to extract plate from camera
                showStatus('📸 Đang chụp ảnh và trích xuất biển số tự động...', 'info');
                
                // Automatically trigger plate extraction with small delay
                setTimeout(() => {
                    autoExtractPlateFromNFC(data);
                }, 1000); // Small delay to ensure message is displayed
            }
        } else {
            // Access denied
            showStatus(`❌ Truy cập bị từ chối: ${data.message || 'Không được phép'}`, 'error');
        }
        
    } catch (error) {
        console.error('❌ Error parsing MQTT message:', error);
        showStatus('❌ Lỗi xử lý thông điệp MQTT', 'error');
    }
}

function autoExtractPlateFromNFC(nfcData) {
    showStatus('📸 Đang chụp ảnh và trích xuất biển số tự động...', 'info');
    
    fetch(self.ctx.$scope.serverUrl + '/snapshot?flag=1&crop=1&extract_plate=1')
        .then(response => response.json())
        .then(data => {
            if (data.success) {
                // Pre-fill UID information from NFC
                data.uid = nfcData.uid;
                data.userName = nfcData.name;
                
                showPlateEditSection(data);
                showStatus(`✅ Đã trích xuất biển số cho ${nfcData.name || nfcData.uid}!`, 'success');
            } else {
                showPlateEditSection({ 
                    success: false, 
                    uid: nfcData.uid, 
                    userName: nfcData.name,
                    error: data.error || 'Không trích xuất được biển số'
                });
                showStatus('⚠️ Không trích xuất được biển số - Vui lòng nhập thủ công', 'warning');
            }
        })
        .catch(error => {
            console.error('Error:', error);
            showPlateEditSection({ 
                success: false, 
                uid: nfcData.uid, 
                userName: nfcData.name,
                error: error.message
            });
            showStatus('❌ Lỗi chụp ảnh tự động: ' + error.message, 'error');
        });
}

// Check auto-save status
function checkAutoSaveStatus() {
    fetch(self.ctx.$scope.serverUrl + '/vehicle/auto_save_status')
        .then(response => response.json())
        .then(data => {
            if (data.success) {
                if (!data.auto_save_enabled) {
                    showStatus('ℹ️ Auto-save is disabled. License plates will only be saved when you click Save.', 'info');
                }
            }
        })
        .catch(error => {
            console.log('Could not check auto-save status:', error);
        });
}

// Hàm xử lý tin nhắn quét thẻ trực tiếp từ các topic scan/in và scan/out
function handleScanMessage(topic, message) {
    try {
        // Cố gắng parse JSON nếu tin nhắn là JSON
        let data;
        try {
            data = JSON.parse(message.toString());
        } catch (jsonError) {
            // Nếu không phải JSON, coi như UID trực tiếp
            const uid = message.toString().trim();
            data = { uid: uid };
        }
        
        const uid = data.uid;
        if (!uid) {
            console.log('Skipping scan message without UID');
            return;
        }
        
        // Xác định loại quét (vào hay ra) dựa trên topic
        const isEntry = topic === 'yolouno/rfid/scan/in';
        const scanType = isEntry ? 'vào' : 'ra';
        
        // Hiển thị thông báo quét thẻ
        showStatus(`🔄 Đang xử lý thẻ NFC: ${uid} (Quét ${scanType})`, 'info');
        
        // Tự động chụp ảnh và trích xuất biển số
        setTimeout(() => {
            autoExtractPlateFromNFC({
                uid: uid,
                name: data.name || 'Không xác định',
                scan_type: isEntry ? 'entry' : 'exit',
                device_id: data.device_id || 'UNKNOWN_DEVICE',
                timestamp: data.timestamp || new Date().toISOString()
            });
        }, 1000);
        
    } catch (error) {
        console.error('❌ Error handling scan message:', error);
        showStatus('❌ Lỗi xử lý tin nhắn quét thẻ', 'error');
    }
}
