// Smart Village Relief Management - Frontend JavaScript
// Supports both:
// 1. Native C Backend (http://localhost:8080 when running web_bridge.exe locally)
// 2. Client-side In-Memory / LocalStorage Mock Mode (for GitHub Pages / Cloud Hosting)

const API_BASE = "http://localhost:8080";
let isBackendOnline = false;

// Admin Session State
const ADMIN_USER = "admin123";
const ADMIN_PASS = "Administrator@10";
let isAdminLoggedIn = false;

// Default initial state for client-side storage
const DEFAULT_INVENTORY = {
  riceStock: 500,
  wheatStock: 300,
  fertilizerPackets: 100
};

// LocalStorage helpers
function getLocalInventory() {
  const data = localStorage.getItem("svr_inventory");
  if (!data) {
    localStorage.setItem("svr_inventory", JSON.stringify(DEFAULT_INVENTORY));
    return { ...DEFAULT_INVENTORY };
  }
  return JSON.parse(data);
}

function setLocalInventory(inv) {
  localStorage.setItem("svr_inventory", JSON.stringify(inv));
}

function getLocalCitizens() {
  const data = localStorage.getItem("svr_citizens");
  if (!data) return [];
  return JSON.parse(data);
}

function setLocalCitizens(citizens) {
  localStorage.setItem("svr_citizens", JSON.stringify(citizens));
}

// Check Backend Connection with timeout
async function checkBackendStatus() {
  const badge = document.querySelector(".backend-badge");
  try {
    const controller = new AbortController();
    const timeoutId = setTimeout(() => controller.abort(), 1500);

    const res = await fetch(`${API_BASE}/api/inventory`, { signal: controller.signal });
    clearTimeout(timeoutId);

    if (res.ok) {
      isBackendOnline = true;
      if (badge) {
        badge.innerHTML = `<span class="pulse-dot"></span> C Backend Connected (Local)`;
        badge.style.background = "#e8f5e9";
        badge.style.color = "#2e7d32";
      }
      return;
    }
  } catch (err) {
    // Backend offline or running on GitHub Pages
  }

  isBackendOnline = false;
  if (badge) {
    badge.innerHTML = `<span class="pulse-dot" style="background:#0284c7;"></span> Web Mode (GitHub Pages)`;
    badge.style.background = "#e0f2fe";
    badge.style.color = "#0369a1";
  }
}

// Tab Switching
function switchTab(tabId) {
  document.querySelectorAll(".tab-btn").forEach(btn => btn.classList.remove("active"));
  document.querySelectorAll(".tab-content").forEach(content => content.classList.remove("active"));

  const targetTab = document.getElementById(`tab-${tabId}`);
  if (targetTab) {
    targetTab.classList.add("active");
  }

  const buttons = document.querySelectorAll(".tab-btn");
  buttons.forEach(btn => {
    if (btn.getAttribute("onclick") && btn.getAttribute("onclick").includes(tabId)) {
      btn.classList.add("active");
    }
  });

  if (tabId === "dashboard") {
    loadInventory();
  }
  if (tabId === "admin" || tabId === "dashboard") {
    loadAuditReport();
  }
}

// Notification System
function showAlert(message, type = "success") {
  const alertBox = document.getElementById("alert-box");
  if (!alertBox) return;
  alertBox.className = `alert alert-${type}`;
  alertBox.textContent = message;
  alertBox.classList.remove("hidden");

  setTimeout(() => {
    alertBox.classList.add("hidden");
  }, 4500);
}

// Load Inventory
async function loadInventory() {
  if (isBackendOnline) {
    try {
      const res = await fetch(`${API_BASE}/api/inventory`);
      const data = await res.json();
      if (data.status === "success") {
        updateInventoryUI(data.riceStock, data.wheatStock, data.fertilizerPackets);
        return;
      }
    } catch (e) {
      // Fallback below
    }
  }

  const inv = getLocalInventory();
  updateInventoryUI(inv.riceStock, inv.wheatStock, inv.fertilizerPackets);
}

function updateInventoryUI(rice, wheat, fertilizer) {
  const elRice = document.getElementById("dash-rice");
  const elWheat = document.getElementById("dash-wheat");
  const elFert = document.getElementById("dash-fertilizer");

  if (elRice) elRice.textContent = `${rice} kg`;
  if (elWheat) elWheat.textContent = `${wheat} kg`;
  if (elFert) elFert.textContent = `${fertilizer} pkts`;
}

// Register Citizen
async function handleRegister(e) {
  e.preventDefault();

  const citizen = {
    name: document.getElementById("reg-name").value.trim(),
    isFarmer: parseInt(document.getElementById("reg-farmer").value, 10),
    id: document.getElementById("reg-id").value.trim(),
    familySize: parseInt(document.getElementById("reg-size").value, 10),
    monthlyIncome: parseFloat(document.getElementById("reg-income").value)
  };

  if (!citizen.name || !citizen.id || isNaN(citizen.familySize) || isNaN(citizen.monthlyIncome)) {
    showAlert("Please fill out all fields properly.", "error");
    return;
  }

  if (isBackendOnline) {
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
        return;
      } else {
        showAlert(data.message || "Registration failed.", "error");
        return;
      }
    } catch (err) {
      // Fallback
    }
  }

  // Client-side fallback
  const citizens = getLocalCitizens();
  if (citizens.some(c => c.id === citizen.id)) {
    showAlert(`Citizen ID '${citizen.id}' is already registered!`, "error");
    return;
  }

  citizens.push({
    ...citizen,
    isServed: 0
  });
  setLocalCitizens(citizens);

  showAlert(`Citizen '${citizen.name}' registered successfully!`, "success");
  document.getElementById("form-register").reset();
  loadAuditReport();
}

// Check Eligibility & Distribute
async function handleDistribute(e) {
  e.preventDefault();
  const id = document.getElementById("dist-id").value.trim();
  const resultDiv = document.getElementById("distribute-result");

  if (isBackendOnline) {
    try {
      const res = await fetch(`${API_BASE}/api/distribute`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ id })
      });
      const data = await res.json();
      renderDistributeResult(data);
      return;
    } catch (err) {
      // Fallback
    }
  }

  // Client-side fallback logic
  const citizens = getLocalCitizens();
  const citizen = citizens.find(c => c.id === id);

  if (!citizen) {
    renderDistributeResult({
      status: "not_found",
      message: `No record exists for ID: ${id}`
    });
    return;
  }

  if (citizen.isServed === 1) {
    renderDistributeResult({
      status: "denied",
      message: `Family '${citizen.name}' has already received their relief quota for this month.`,
      income: citizen.monthlyIncome
    });
    return;
  }

  if (citizen.monthlyIncome <= 15000) {
    const inv = getLocalInventory();
    const requiredRice = citizen.familySize * 5;
    const requiredWheat = citizen.familySize * 3;
    const requiredFertilizer = citizen.isFarmer ? 5 : 0;

    if (inv.riceStock >= requiredRice && inv.wheatStock >= requiredWheat && inv.fertilizerPackets >= requiredFertilizer) {
      inv.riceStock -= requiredRice;
      inv.wheatStock -= requiredWheat;
      inv.fertilizerPackets -= requiredFertilizer;
      citizen.isServed = 1;

      setLocalInventory(inv);
      setLocalCitizens(citizens);

      renderDistributeResult({
        status: "success",
        name: citizen.name,
        id: citizen.id,
        income: citizen.monthlyIncome,
        familySize: citizen.familySize,
        isFarmer: citizen.isFarmer,
        disbursedRice: requiredRice,
        disbursedWheat: requiredWheat,
        disbursedFertilizer: requiredFertilizer,
        remainingRice: inv.riceStock,
        remainingWheat: inv.wheatStock,
        remainingFertilizer: inv.fertilizerPackets
      });
      loadInventory();
      loadAuditReport();
    } else {
      renderDistributeResult({
        status: "error",
        message: `Insufficient inventory (Needed: ${requiredRice} kg Rice, ${requiredWheat} kg Wheat, ${requiredFertilizer} pkts Fertilizer).`
      });
    }
  } else {
    renderDistributeResult({
      status: "denied",
      message: `Income (₹${citizen.monthlyIncome.toFixed(2)}) exceeds BPL threshold (₹15,000). Relief denied.`,
      income: citizen.monthlyIncome
    });
  }
}

function renderDistributeResult(data) {
  const resultDiv = document.getElementById("distribute-result");
  resultDiv.classList.remove("hidden");

  if (data.status === "success") {
    resultDiv.className = "result-container result-success";
    resultDiv.innerHTML = `
      <h4 style="color: #2e7d32; margin-bottom: 6px;">🎉 Quota Approved & Disbursed</h4>
      <p><strong>Citizen:</strong> ${data.name} (ID: ${data.id})</p>
      <p><strong>Household Type:</strong> ${data.isFarmer ? "🌾 Farmer Household" : "Standard Household"}</p>
      <p><strong>Monthly Income:</strong> ₹${data.income} (Eligible &le; ₹15,000)</p>
      <p><strong>Family Members:</strong> ${data.familySize}</p>
      <hr style="margin: 10px 0; border: none; border-top: 1px solid #c8e6c9;">
      <p style="font-weight: 600; color: #1e6f5c;">Disbursed: ${data.disbursedRice} kg Rice | ${data.disbursedWheat} kg Wheat${data.disbursedFertilizer > 0 ? ` | <strong>${data.disbursedFertilizer} pkts Fertilizer</strong>` : ""}</p>
      <p style="font-size: 12px; color: #555;">Remaining Stock: ${data.remainingRice} kg Rice, ${data.remainingWheat} kg Wheat, ${data.remainingFertilizer} pkts Fertilizer</p>
    `;
    showAlert(`Disbursed successfully to ${data.name}!`, "success");
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
      <h4 style="color: #c62828; margin-bottom: 6px;">❌ Notice</h4>
      <p>${data.message}</p>
    `;
    showAlert(data.message, "error");
  }
}

// Admin Authentication Handlers
function handleAdminLogin(e) {
  e.preventDefault();
  const u = document.getElementById("admin-user").value.trim();
  const p = document.getElementById("admin-pass").value;

  if (u === ADMIN_USER && p === ADMIN_PASS) {
    isAdminLoggedIn = true;
    sessionStorage.setItem("svr_admin_logged", "true");
    updateAdminUI();
    showAlert("Admin authentication successful!", "success");
  } else {
    showAlert("Invalid admin credentials! Please check your username and password.", "error");
  }
}

function handleAdminLogout() {
  isAdminLoggedIn = false;
  sessionStorage.removeItem("svr_admin_logged");
  updateAdminUI();
  showAlert("Logged out of admin panel.", "warning");
}

function updateAdminUI() {
  const authContainer = document.getElementById("admin-auth-container");
  const loggedInBar = document.getElementById("admin-logged-in-bar");
  const stockCard = document.getElementById("admin-stock-card");

  if (isAdminLoggedIn) {
    if (authContainer) authContainer.classList.add("hidden");
    if (loggedInBar) loggedInBar.classList.remove("hidden");
    if (stockCard) stockCard.classList.remove("hidden");
  } else {
    if (authContainer) authContainer.classList.remove("hidden");
    if (loggedInBar) loggedInBar.classList.add("hidden");
    if (stockCard) stockCard.classList.add("hidden");
    const loginForm = document.getElementById("form-admin-login");
    if (loginForm) loginForm.reset();
  }
}

// Update Inventory (Admin Only)
async function handleUpdateInventory(e) {
  e.preventDefault();

  if (!isAdminLoggedIn) {
    showAlert("Admin authentication required to update stock!", "error");
    return;
  }

  const payload = {
    username: ADMIN_USER,
    password: ADMIN_PASS,
    rice: parseInt(document.getElementById("inv-rice").value, 10) || 0,
    wheat: parseInt(document.getElementById("inv-wheat").value, 10) || 0,
    fertilizer: parseInt(document.getElementById("inv-fert").value, 10) || 0
  };

  if (isBackendOnline) {
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
        return;
      }
    } catch (err) {
      // Fallback
    }
  }

  // Client-side fallback
  const inv = getLocalInventory();
  inv.riceStock += payload.rice;
  inv.wheatStock += payload.wheat;
  inv.fertilizerPackets += payload.fertilizer;
  setLocalInventory(inv);

  showAlert("Inventory updated successfully!", "success");
  document.getElementById("form-inventory").reset();
  loadInventory();
}

// Audit Report & Stats (Visible to everyone)
async function loadAuditReport() {
  let citizens = [];
  let totalFamilies = 0;
  let servedFamilies = 0;

  if (isBackendOnline) {
    try {
      const res = await fetch(`${API_BASE}/api/report`);
      const data = await res.json();
      if (data.status === "success") {
        citizens = data.citizens || [];
        totalFamilies = data.totalFamilies;
        servedFamilies = data.servedFamilies;
      }
    } catch (e) {
      // Fallback
    }
  }

  if (!isBackendOnline) {
    citizens = getLocalCitizens();
    totalFamilies = citizens.length;
    servedFamilies = citizens.filter(c => c.isServed === 1).length;
  }

  const pendingFamilies = totalFamilies - servedFamilies;

  const elTotal = document.getElementById("rep-total");
  const elServed = document.getElementById("rep-served");
  const elPending = document.getElementById("rep-pending");
  const elAssisted = document.getElementById("dash-assisted");

  if (elTotal) elTotal.textContent = totalFamilies;
  if (elServed) elServed.textContent = servedFamilies;
  if (elPending) elPending.textContent = pendingFamilies;
  if (elAssisted) elAssisted.textContent = `${servedFamilies} / ${totalFamilies}`;

  const tbody = document.getElementById("audit-tbody");
  if (!tbody) return;
  tbody.innerHTML = "";

  if (citizens.length === 0) {
    tbody.innerHTML = `<tr><td colspan="7" style="text-align: center; color: #888;">No citizen records found. Register households first.</td></tr>`;
    return;
  }

  citizens.forEach(c => {
    const tr = document.createElement("tr");
    const statusBadge = c.isServed
      ? `<span class="badge badge-served">SERVED</span>`
      : `<span class="badge badge-pending">PENDING</span>`;

    const farmerBadge = c.isFarmer
      ? `<span class="badge badge-farmer">🌾 Farmer</span>`
      : `<span class="badge badge-nonfarmer">Non-Farmer</span>`;

    const riceQuota = c.familySize * 5;
    const wheatQuota = c.familySize * 3;
    const fertQuota = c.isFarmer ? " + 5 pkts Fert." : "";

    tr.innerHTML = `
      <td><strong>${c.name}</strong></td>
      <td><code>${c.id}</code></td>
      <td>${farmerBadge}</td>
      <td>${c.familySize}</td>
      <td>₹${Number(c.monthlyIncome).toFixed(2)}</td>
      <td>${riceQuota} kg Rice / ${wheatQuota} kg Wheat${fertQuota}</td>
      <td>${statusBadge}</td>
    `;
    tbody.appendChild(tr);
  });
}

// Initialize on Load
document.addEventListener("DOMContentLoaded", async () => {
  if (sessionStorage.getItem("svr_admin_logged") === "true") {
    isAdminLoggedIn = true;
  }
  updateAdminUI();

  await checkBackendStatus();
  loadInventory();
  loadAuditReport();
});
