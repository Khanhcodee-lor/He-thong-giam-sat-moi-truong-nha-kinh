1. Kết nối esp với cảm biến quang trở
   Vcc  ->  3v3 của esp32 hoặc nguồn 3.3v
   Gnd  -> Gnd
   A0   -> GPIO 32 (ADC1_CHANNEL_4)
   D0 không dùng  

2. Kết nối esp với dht11
   +    ->  3.3V hoặc 5V
   Data ->  GPIO 4 
   -    ->  GND

3. Kết nối esp với cảm biến độ ẩm đất
    Vcc  ->  3v3 của esp32 hoặc nguồn 3.3v
   Gnd  -> Gnd
   A0   -> GPIO 33 (ADC1_CHANNEL_5)
   D0 không dùng

   4. Kết nối esp32 với cảm biến mực sâu nước
   Vcc  ->  3v3 của esp32 hoặc nguồn 3.3v
   Gnd  -> Gnd
   A0   -> GPIO 34 (ADC1_CHANNEL_6)

   5. Kết nối esp32 với cảm biến khí gas mq2
   Vcc  ->  5v 
   Gnd  -> Gnd
   A0   -> GPIO 35 (ADC1_CHANNEL_7)

## Smart Plant BLE Mesh

Firmware hướng tới ESP32 với ESP-IDF **v5.4.3** (devcontainer đã ghim phiên bản).
Configuration Server và Vendor Model `05F1:0001` nằm trên primary element.
UUID gồm namespace 10 byte và factory Bluetooth MAC 6 byte, ổn định sau reboot.
Thiết bị mới bật PB-ADV, No OOB; thiết bị đã provision khôi phục từ NVS.
Không hardcode NetKey/AppKey và không tự xóa NVS khi khởi tạo lỗi.

### Build và hiệu chuẩn

```sh
idf.py set-target esp32
idf.py menuconfig
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

Trong `Smart Plant sensor calibration`, nhập ADC đất khô (0%) và ướt (100%),
cùng hai điểm ADC/lux đo bằng thiết bị tham chiếu, rồi bật `Enable telemetry`.
Các giá trị mặc định chỉ là placeholder, không phải hiệu chuẩn thực tế.
Phép đổi lux hiện là nội suy tuyến tính hai điểm: chỉ là xấp xỉ trong khoảng
đã hiệu chuẩn vì quang trở phi tuyến; cần bảng/đường cong hoặc cảm biến lux
chuyên dụng nếu yêu cầu độ chính xác trên dải rộng. Hai điểm ADC phải khác nhau.
Chưa bật hiệu chuẩn thì vẫn provisioning được nhưng không phát dữ liệu cảm biến.
Nếu dự án đã có sdkconfig, kiểm tra các tùy chọn tương ứng trong sdkconfig.defaults:
BLE Mesh Node, PB-ADV, Bluedroid, persistent settings và partition Single factory app large.

### Cấu hình phía Raspberry Pi

1. Quét UUID được in trên serial và provision bằng PB-ADV/No OOB, NetKey index 0.
2. Cấp AppKey index 0, bind vào Vendor Model `0x05F1 / 0x0001`.
3. Đặt publication của model tới `0x0001`, AppKey index 0, period 0.
4. Sau `node_configured` phía Pi, nhận opcode trên sóng `C1 F1 05`.

Firmware kiểm tra provisioning, khóa, binding và publication trước mỗi lần gửi;
không phụ thuộc callback cấu hình mới để tiếp tục sau reboot. Vòng lặp dùng
`vTaskDelayUntil` mỗi 5 giây. Mẫu lỗi cảm biến bị bỏ qua, không gửi số 0 thay thế.
Payload 8 byte little-endian: int16 nhiệt độ ×100, uint16 độ ẩm không khí ×100,
uint16 lux, uint16 độ ẩm đất ×100. Độ ẩm giới hạn 0–10000, lux 0–65535.
Opcode truyền riêng vào `esp_ble_mesh_model_publish`; publication buffer dài 11 byte.
Không có giao thức điều khiển bơm.

### Kiểm tra

Test host cho payload mẫu, nhiệt độ âm, giới hạn, NaN và phép hiệu chuẩn:

```sh
cc -std=c11 -Wall -Wextra -Werror -I main tests/test_sensor_payload.c main/sensor_payload.c -lm -o /tmp/smart-plant-payload-test
/tmp/smart-plant-payload-test
```

Nghiệm thu phần cứng: kiểm tra Pi quét thấy UUID, `node_configured`, dữ liệu
`27.50 / 65.00 / 400 / 45.20` tương ứng `BE 0A 64 19 90 01 A8 11` với đầu vào
mẫu, nhận chu kỳ 5 giây và tiếp tục nhận sau reboot mà không provision lại.
Kiểm tra bỏ binding/publication thì dừng gửi. `ESP_OK` khi publish chỉ xác nhận
đã xếp yêu cầu gửi; cần log Pi để xác nhận nhận thành công.
Không chạy `erase-flash` khi kiểm tra lưu mạng qua reboot.

API tham chiếu: https://docs.espressif.com/projects/esp-idf/en/v5.4.3/esp32/api-reference/bluetooth/esp-ble-mesh.html
