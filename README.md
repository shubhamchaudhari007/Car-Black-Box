# 🚗 Car Black Box System – Embedded C Project

## 🚀 Overview

This project implements a **Car Black Box System** using **PIC microcontroller and Embedded C**.

The system records vehicle events such as **speed, gear changes, and timestamp**, stores them in EEPROM, and provides a user interface to **view, download, and manage logs**.

---

## 🎯 Features

### 📊 Dashboard

* ✅ Displays real-time speed
* ✅ Displays gear position
* ✅ Shows current time using RTC
* ✅ Detects gear change events

### 🧾 Event Logging

* ✅ Logs time, gear, and speed
* ✅ Stores data in EEPROM
* ✅ Implements circular buffer (last 10 records)
* ✅ Avoids duplicate logging for same gear

### 📋 Menu System

* ✅ View Logs
* ✅ Clear Logs
* ✅ Download Logs (UART)
* ✅ Set Time

### ⌨️ User Interface

* ✅ Matrix keypad for navigation
* ✅ CLCD display for output
* ✅ UART for log download

---

## 🧠 Concepts Used

* Embedded C Programming
* State Machine Design
* EEPROM Data Storage
* RTC (DS1307) Communication via I2C
* ADC for sensor input
* UART Communication
* Circular Buffer Implementation
* Keypad Interfacing

---

## ⚙️ Technologies Used

* **Microcontroller:** PIC18 Series
* **Language:** Embedded C
* **Compiler:** MPLAB XC8
* **Peripherals:**

  * ADC
  * EEPROM
  * DS1307 RTC
  * I2C
  * UART
  * CLCD
  * Matrix Keypad

---

## 🏗️ Project Structure

```id="bbstruct"
Car_Black_Box/
│── main.c
│── black_box.h
│── adc.c / adc.h
│── clcd.c / clcd.h
│── ds1307.c / ds1307.h
│── eeprom.c / eeprom.h
│── i2c.c / i2c.h
│── uart.c / uart.h
│── matrix_keypad.c / matrix_keypad.h
│── Makefile
```

---

## ▶️ Working Principle

1. System reads speed using ADC
2. Reads current time from RTC (DS1307)
3. Detects gear changes using keypad
4. Logs event (time + gear + speed) into EEPROM
5. Stores only last 10 records using circular buffer
6. User can:

   * View logs
   * Download logs via UART
   * Clear logs
   * Set system time

---

## 💻 Key Functionalities

* ADC-based speed calculation 
* RTC time handling using DS1307 
* EEPROM logging with circular buffer 
* Menu-driven state machine system 

---

## ⚠️ Limitations

* ❌ Limited EEPROM storage (only last 10 events)
* ❌ No crash detection sensor integration
* ❌ No external storage (SD card)

---

## 🔮 Future Enhancements

* 🔹 Add crash detection using accelerometer
* 🔹 Store logs in SD card
* 🔹 Add GPS tracking
* 🔹 Wireless data transmission

---

## 🧩 Challenges Faced

* Managing EEPROM memory efficiently
* Implementing circular buffer correctly
* Designing user-friendly menu system
* Handling real-time clock synchronization

---

## 📚 Learning Outcomes

* Strong understanding of embedded system design
* Practical experience with RTC, EEPROM, and I2C
* Implementation of real-time logging systems
* Improved debugging and system integration skills

---

## 📌 Author

**Shubham Chaudhari**
