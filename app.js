// Smart Village Relief Management - Frontend JavaScript

const API_BASE = "http://localhost:8080";

// Tab Switching
function switchTab(tabId) {
  document.querySelectorAll(".tab-btn").forEach(btn => btn.classList.remove("active"));
  document.querySelectorAll(".tab-content").forEach(content => content.classList.remove("active"));

  const targetTab = document.getElementById(`tab-${tabId}`);
  if (targetTab) {
    targetTab.classList.add("active");
  }

  // Highlight button
  const buttons = document.querySelectorAll(".tab-btn");
  buttons.forEach(btn => {
    if (btn.getAttribute("onclick").includes(tabId)) {
      btn.classList.add("active");
    }
  });

  // Load contextual data
  if (tabId === "dashboard" || tabId === "inventory") {
    loadInventory();
  }
  if (tabId === "report" || tabId === "dashboard") {
    loadAuditReport();
  }
}

// Notification System
function showAlert(message, type = "success") {
  const alertBox = document.getElementById("alert-box");
  alertBox.className = `alert alert-${type}`;
  alertBox.textContent = message;
  alertBox.classList.remove("hidden");

  setTimeout(() => {
    alertBox.classList.add("hidden");
  }, 4500);
}

// Load Inventory
async function loadInventory() {
  try {
    const res = await fetch(`${API_BASE}/api/inventory`);
    const data = await res.json();

    if (data.status === "success") {
      document.getElementById("dash-rice").textContent = `${data.riceStock} kg`;
      document.getElementById("dash-wheat").textContent = `${data.wheatStock} kg`;
      document.getElementById("dash-fertilizer").textContent = `${data.fertilizerPackets} pkts`;
    }
  } catch (err) {
    console.error("Failed to fetch inventory:", err);
  }
}

// Register Citizen
async function handleRegister(e) {
  e.preventDefault();

  const citizen = {
    name: document.getElementById("reg-name").value.trim(),
    id: document.getElementById("reg-id").value.trim(),
    familySize: parseInt(document.getElementById("reg-size").value, 10),
    monthlyIncome: parseFloat(document.getElementById("reg-income").value)
  };

  try {
    const res = await fetch(`${API_BASE}/api/register`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(citizen)
    });

    const data = await res.json();

    if (data.status === "success") {
      showAlert(data.message, "success");
      document.getElementById("form-register").reset();
      loadAuditReport();
    } else {
      showAlert(data.message || "Registration failed.", "error");
    }
  } catch (err) {
    showAlert("Could not communicate with the C backend server.", "error");
  }
}

// Check Eligibility & Distribute
async function handleDistribute(e) {
  e.preventDefault();
  const id = document.getElementById("dist-id").value.trim();
  const resultDiv = document.getElementById("distribute-result");

  try {
    const res = await fetch(`${API_BASE}/api/distribute`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ id })
    });

    const data = await res.json();
    resultDiv.classList.remove("hidden");

    if (data.status === "success") {
      resultDiv.className = "result-container result-success";
      resultDiv.innerHTML = `
        <h4 style="color: #2e7d32; margin-bottom: 6px;">🎉 Quota Approved & Disbursed</h4>
        <p><strong>Citizen:</strong> ${data.name} (ID: ${data.id})</p>
        <p><strong>Monthly Income:</strong> ₹${data.income} (Eligible &le; ₹15,000)</p>
        <p><strong>Family Members:</strong> ${data.familySize}</p>
        <hr style="margin: 10px 0; border: none; border-top: 1px solid #c8e6c9;">
        <p style="font-weight: 600; color: #1e6f5c;">Disbursed: ${data.disbursedRice} kg Rice | ${data.disbursedWheat} kg Wheat</p>
        <p style="font-size: 12px; color: #555;">Remaining Stock: ${data.remainingRice} kg Rice, ${data.remainingWheat} kg Wheat</p>
      `;
      showAlert(`Disbursed successfully to ${data.name}!`, "success");
      loadInventory();
      loadAuditReport();
    } else if (data.status === "denied") {
      resultDiv.className = "result-container result-denied";
      resultDiv.innerHTML = `
        <h4 style="color: #ef6c00; margin-bottom: 6px;">⚠️ Relief Denied</h4>
        <p>${data.message}</p>
        ${data.income ? `<p><strong>Registered Income:</strong> ₹${data.income}</p>` : ""}
      `;
      showAlert(data.message, "warning");
    } else {
      resultDiv.className = "result-container result-error";
      resultDiv.innerHTML = `
        <h4 style="color: #c62828; margin-bottom: 6px;">❌ Operation Incomplete</h4>
        <p>${data.message}</p>
      `;
      showAlert(data.message, "error");
    }
  } catch (err) {
    showAlert("Error connecting to backend.", "error");
  }
}

// Update Inventory
async function handleUpdateInventory(e) {
  e.preventDefault();

  const payload = {
    rice: parseInt(document.getElementById("inv-rice").value, 10) || 0,
    wheat: parseInt(document.getElementById("inv-wheat").value, 10) || 0,
    fertilizer: parseInt(document.getElementById("inv-fert").value, 10) || 0
  };

  try {
    const res = await fetch(`${API_BASE}/api/update-inventory`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(payload)
    });

    const data = await res.json();

    if (data.status === "success") {
      showAlert(data.message, "success");
      document.getElementById("form-inventory").reset();
      loadInventory();
    } else {
      showAlert(data.message, "error");
    }
  } catch (err) {
    showAlert("Failed to update inventory via backend.", "error");
  }
}

// Audit Report & Stats
async function loadAuditReport() {
  try {
    const res = await fetch(`${API_BASE}/api/report`);
    const data = await res.json();

    if (data.status === "success") {
      document.getElementById("rep-total").textContent = data.totalFamilies;
      document.getElementById("rep-served").textContent = data.servedFamilies;
      document.getElementById("rep-pending").textContent = data.pendingFamilies;

      document.getElementById("dash-assisted").textContent = `${data.servedFamilies} / ${data.totalFamilies}`;

      const tbody = document.getElementById("audit-tbody");
      tbody.innerHTML = "";

      if (!data.citizens || data.citizens.length === 0) {
        tbody.innerHTML = `<tr><td colspan="6" style="text-align: center; color: #888;">No citizen records found. Register households first.</td></tr>`;
        return;
      }

      data.citizens.forEach(c => {
        const tr = document.createElement("tr");
        const statusBadge = c.isServed
          ? `<span class="badge badge-served">SERVED</span>`
          : `<span class="badge badge-pending">PENDING</span>`;

        const riceQuota = c.familySize * 5;
        const wheatQuota = c.familySize * 3;

        tr.innerHTML = `
          <td><strong>${c.name}</strong></td>
          <td><code>${c.id}</code></td>
          <td>${c.familySize}</td>
          <td>₹${c.monthlyIncome.toFixed(2)}</td>
          <td>${riceQuota} kg / ${wheatQuota} kg</td>
          <td>${statusBadge}</td>
        `;
        tbody.appendChild(tr);
      });
    }
  } catch (err) {
    console.error("Failed to load audit report:", err);
  }
}

// Initialize on Load
document.addEventListener("DOMContentLoaded", () => {
  loadInventory();
  loadAuditReport();
});

