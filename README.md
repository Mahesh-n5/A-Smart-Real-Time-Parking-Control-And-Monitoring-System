# Smart Parking System for Employees 🚗💻

## 📌 Overview
An IoT-based smart parking solution designed for company employees that combines web booking with automated license plate recognition and violation detection using raspberry pi.

## ✨ Features
- **Employee Portal**: 
  - Secure registration/login
  - Real-time slot booking
  - Booking history tracking
- **Smart Detection**:
  - Ultrasonic sensors detect vehicle presence
  - ESP32-CAM captures license plate images
- **Automated Processing**:
  - Raspberry Pi performs OCR on license plates
  - Database cross-verification
- **Notification System**:
  - Email alerts for successful parking
  - Violation notifications (wrong slot, overtime, etc.)

## 🛠️ Tech Stack
| Component          | Technology Used |
|--------------------|-----------------|
| Frontend           | React.js, Bootstrap, HTML, CSS |
| Backend            | javascript, python |
| Database           | MySQL |
| Hardware           | ESP32-CAM, Raspberry Pi 4 |
| Sensors            | HC-SR04 Ultrasonic |
| OCR                | Tesseract.js |
| Notification       | smtplib |

## 🚀 Installation
Set up Raspberry Pi with :
  - sudo apt install tesseract-ocr -
  - pip install -r requirements.txt -

## Flow Diagram
https://drive.google.com/file/d/1DfQe-CaKi-7DmI863brh2IabOOGh7Kbo/view?usp=sharing

## Web Application
Clone the repository
Install dependencies: npm install
Configure environment variables (database connection, email service)
Start server: npm start

## Hardware setup
Connect ultrasonic sensors to ESP32 boards
Configure ESP32-CAM modules for image capture
Set up Raspberry Pi with:
  - OCR software (Tesseract) -
  - Database connection -
  - Image processing scripts -

