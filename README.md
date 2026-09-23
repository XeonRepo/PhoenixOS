# 🐦 PhoenixOS — Real-Time Kernel for Embedded Systems

![License](https://img.shields.io/badge/License-All%20Rights%20Reserved-red.svg)
![Platform](https://img.shields.io/badge/Platform-ARM%20Cortex--M3-orange.svg)
![Language](https://img.shields.io/badge/Language-C%20%7C%20Assembly-blue.svg)
![Lines](https://img.shields.io/badge/Lines-600%2B-yellow.svg)
![Status](https://img.shields.io/badge/Status-Kernel%20Complete-brightgreen.svg)

A **preemptive, priority-based Real-Time Operating System (RTOS) kernel** built from scratch for ARM Cortex-M3 microcontrollers — no external RTOS libraries used.

---

## 📌 About the Project

PhoenixOS is a lightweight RTOS kernel developed as a B.Tech final year project. It provides deterministic task scheduling, fast context switching, and synchronization primitives for embedded systems used in automotive, medical, industrial, and IoT applications.

The kernel is written in **C** and **ARM Assembly**, and is tested on the **QEMU mps2-an385** emulator (ARM Cortex-M3).

---

## ✨ Features

- ✅ **Preemptive Priority Scheduler** — O(1) task selection using bitmap + `__builtin_ctz`
- ✅ **Context Switching** — via PendSV exception (ARM Cortex-M native mechanism)
- ✅ **Counting Semaphores** — task-to-task signaling
- ✅ **Mutexes** — shared resource locking with owner tracking and recursive-lock detection
- ✅ **SysTick Timer** — 100 Hz tick for delays and round-robin scheduling
- ✅ **UART Output** — works on both QEMU and real hardware
- ✅ **Idle Task** — uses `WFI` (Wait For Interrupt) for low power consumption
- ✅ **Portable** — runs on any ARM Cortex-M3 based chip

---

## 🎯 Project Objectives

| # | Objective | Status |
|---|-----------|--------|
| 1 | Preemptive Priority RTOS Kernel | ✅ Complete |
| 2 | Dual-Bank Bootloader with OTA + Rollback | 🔄 Future |
| 3 | Custom Communication Protocol Stack | 🔄 Future |
| 4 | GDB-based Fault Injection Framework | 🔄 Future |
