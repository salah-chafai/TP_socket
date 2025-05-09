# Binary Operator with CRC Verification

This project implements a simple client-server system for evaluating binary operations (e.g., addition, subtraction). 

## Overview

- **Client:** Sends a binary operation (e.g., `3 + 5`) to the server for evaluation.
- **Middle Man:** Intercepts the operation and may introduce intentional errors or modifications to simulate a compromised communication channel.
- **Server:** Receives the operation and uses a CRC (Cyclic Redundancy Check) to verify the integrity of the data before evaluating it.

If the CRC check passes, the server performs the requested operation and returns the result. If it fails, the server returns an error.

## Purpose

This setup demonstrates the use of CRC to detect data corruption in a networked environment with potential interference.

## Components

- `client`: Sends operations.
- `middleman`: Intercepts and potentially corrupts data.
- `server`: Verifies and evaluates operations using CRC.

## Usage

Compile and run each component in separate terminals or network nodes. Ensure they communicate in the correct order: `server → middleman → client`.

### Note

The attached Makefile is for linux compilation

---

