# Smart Village Relief Distribution System

A lightweight web application powered directly by a native **C Backend** (`web_bridge.c`), utilizing file-based binary storage (`citizens.txt` and `inventory.txt`) with an interactive HTML5/CSS3/JavaScript web interface.

---

## 📁 Project Structure

```text
C:\Aashritha\
├── web_bridge.c         # Native C HTTP micro-server & business logic
├── web_bridge.exe       # Compiled C executable
├── index.html           # Frontend structure & user interface
├── style.css            # Responsive UI styling & indicators
├── app.js               # Frontend JavaScript calling C endpoints
├── citizens.txt         # Binary data file storing Citizen structures
└── inventory.txt        # Binary data file storing Inventory structures
```

---

## 🚀 How to Run

### Step 1: Compile the C Backend
If you need to recompile:
```cmd
gcc web_bridge.c -o web_bridge.exe -lws2_32
```

### Step 2: Start the Web Bridge Server
Run the compiled binary:
```cmd
.\web_bridge.exe
```
Output:
```text
=======================================================
  SMART VILLAGE RELIEF WEB BRIDGE SERVER STARTED
  Port: http://localhost:8080
  Serving HTML, CSS, JS and REST API endpoints
=======================================================
```

### Step 3: Open in Browser
Open your browser and navigate to:
```
https://pasupuletiaashritha-lang.github.io/Smart-Village-Relief-Distribution-System/
```

---

## 🎯 Features & Operations
1. **Live Dashboard**: Shows remaining Rice (kg), Wheat (kg), Fertilizer packets, and assistance metrics in real-time.
2. **Register New Citizen/Family**: Adds families with validation directly into binary storage via C `fwrite`.
3. **Check Eligibility & Distribute**:
   - Evaluates Below Poverty Line (BPL) income criterion ($\le$ 15,000 INR).
   - Computes family quota: **5 kg Rice & 3 kg Wheat** per member.
   - Verifies stock and marks record as served in `citizens.txt`.
4. **Update Inventory Stock (Admin)**: Replenishes community warehouse stocks.
5. **Community Audit Report**: Tabulates all households, their quota, status (`SERVED` or `PENDING`), and community assistance statistics.

