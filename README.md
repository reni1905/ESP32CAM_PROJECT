# ESP32CAM_PROJECT
Barcode-Based Detection System for Israel-Affiliated Products Using ESP32-CAM
# Problems
The difficulty people face in quickly and practically identifying and filtering out products affiliated with Israel in the marketplace.
# Solutions
Developing an automated barcode scanner using an ESP32-CAM integrated with a local database to match barcode numbers (EAN-13) and display product status (Safe/Boycott) on an OLED screen and via LED indicators.
# Tech Stack
*** Hardware: ESP32-CAM (AI-Thinker), 0.96-inch OLED display, LEDs (Red and Green).
*** Software/Server: Arduino IDE, PHP, MySQL, XAMPP, and the ZXing barcode reading library.
# Features
- [x] Scanning EAN-13 product barcodes using the ESP32-CAM camera.
- [x] Sending the barcode image to the local server using HTTP POST.
- [x] Processing with ZXing and matching the barcode number against a MySQL database.
- [x] Menyalakan lampu merah (boikot)
- [ ] Menyalakan lampu hijau (aman).
- [x] Displaying product name and status on the OLED.
# Results
The system successfully detects and accurately reads EAN-13 barcodes, and is capable of responsively displaying product name information and affiliation status on the OLED and LED.
# Link
https://drive.google.com/file/d/1IiQ_0j7ASXx2VxhRD74Vd8XRXBEwfXoW/view?usp=drivesdk
# Documentation
<img width="905" height="1280" alt="image" src="https://github.com/user-attachments/assets/f9889749-0854-4581-a2a9-90c469d56833" />

<img width="245" height="254" alt="Screenshot 2026-07-27 173611" src="https://github.com/user-attachments/assets/3bad8a4e-a09d-4c18-87ea-e264973253aa" />
<img width="247" height="269" alt="Screenshot 2026-07-27 173534" src="https://github.com/user-attachments/assets/b3e49bf0-8861-42c3-b9be-cce8b27dbcd1" />
