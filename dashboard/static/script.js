
// =============================================
// SMART MANUFACTURING COST ANALYZER
// Dashboard JavaScript
// =============================================

let productionChart = null;
let materialChart = null;


// Format numbers using Indian number system
function formatNumber(value) {
    return Number(value || 0).toLocaleString("en-IN", {
        maximumFractionDigits: 2
    });
}


// Format currency
function formatCurrency(value) {
    return "₹" + Number(value || 0).toLocaleString("en-IN", {
        minimumFractionDigits: 2,
        maximumFractionDigits: 2
    });
}


// Update KPI cards
function updateKPIs(kpis) {

    document.getElementById("totalProduced").textContent =
        formatNumber(kpis.total_produced);

    document.getElementById("goodUnits").textContent =
        formatNumber(kpis.good_units);

    document.getElementById("totalCost").textContent =
        formatCurrency(kpis.total_cost);

    document.getElementById("averageCost").textContent =
        formatCurrency(kpis.average_cost);

    document.getElementById("totalRejected").textContent =
        formatNumber(kpis.total_rejected);

    document.getElementById("rejectionRate").textContent =
        Number(kpis.rejection_rate || 0).toFixed(2) + "%";
}


// Create daily production chart
function createProductionChart(data) {

    const labels = data.map(item => item.production_date);
    const produced = data.map(item => item.produced);
    const rejected = data.map(item => item.rejected);

    const ctx = document.getElementById("productionChart");

    if (productionChart) {
        productionChart.destroy();
    }

    productionChart = new Chart(ctx, {
        type: "line",

        data: {
            labels: labels,

            datasets: [
                {
                    label: "Produced Units",
                    data: produced,
                    borderColor: "#3268e8",
                    backgroundColor: "rgba(50,104,232,0.12)",
                    fill: true,
                    tension: 0.35
                },

                {
                    label: "Rejected Units",
                    data: rejected,
                    borderColor: "#ed6b6b",
                    backgroundColor: "rgba(237,107,107,0.10)",
                    fill: true,
                    tension: 0.35
                }
            ]
        },

        options: {
            responsive: true,
            maintainAspectRatio: false,

            plugins: {
                legend: {
                    position: "bottom"
                }
            },

            scales: {
                y: {
                    beginAtZero: true
                }
            }
        }
    });
}


// Create material cost chart
function createMaterialChart(data) {

    const labels = data.map(item => item.material_name);
    const costs = data.map(item => item.material_cost);

    const ctx = document.getElementById("materialChart");

    if (materialChart) {
        materialChart.destroy();
    }

    materialChart = new Chart(ctx, {
        type: "doughnut",

        data: {
            labels: labels,

            datasets: [
                {
                    data: costs,
                    backgroundColor: [
                        "#3268e8",
                        "#21b58a",
                        "#f3a644",
                        "#9466e8",
                        "#ed6b6b",
                        "#20a4bd"
                    ],
                    borderWidth: 2,
                    borderColor: "#ffffff"
                }
            ]
        },

        options: {
            responsive: true,
            maintainAspectRatio: false,

            plugins: {
                legend: {
                    position: "bottom"
                }
            }
        }
    });
}


// Update machine performance table
function updateMachineTable(machines) {

    const table = document.getElementById("machineTable");

    if (!machines.length) {
        table.innerHTML =
            '<tr><td colspan="5">No machine data available.</td></tr>';
        return;
    }

    table.innerHTML = machines.map(machine => `
        <tr>
            <td><strong>${machine.machine_name}</strong></td>
            <td>${formatNumber(machine.produced)}</td>
            <td>${formatNumber(machine.rejected)}</td>
            <td>${formatNumber(machine.machine_hours)} hrs</td>
            <td>${Number(machine.efficiency).toFixed(2)}%</td>
        </tr>
    `).join("");
}


// Update material table
function updateMaterialTable(materials) {

    const table = document.getElementById("materialTable");

    if (!materials.length) {
        table.innerHTML =
            '<tr><td colspan="3">No material data available.</td></tr>';
        return;
    }

    table.innerHTML = materials.map(material => `
        <tr>
            <td><strong>${material.material_name}</strong></td>
            <td>${formatNumber(material.total_kg)}</td>
            <td>${formatCurrency(material.material_cost)}</td>
        </tr>
    `).join("");
}


// Update recent production records
function updateRecentTable(records) {

    const table = document.getElementById("recentTable");

    if (!records.length) {
        table.innerHTML =
            '<tr><td colspan="7">No production records available.</td></tr>';
        return;
    }

    table.innerHTML = records.map(record => `
        <tr>
            <td>${record.id}</td>
            <td>${record.run_date}</td>
            <td>${record.machine_name}</td>
            <td>${record.material_name}</td>
            <td>${formatNumber(record.produced)}</td>
            <td>${formatNumber(record.rejected)}</td>
            <td>${formatNumber(record.good_units)}</td>
        </tr>
    `).join("");
}


// Load dashboard data from Flask API
async function loadDashboard() {

    const startDate = document.getElementById("startDate").value;
const endDate = document.getElementById("endDate").value;

if (startDate && endDate && startDate > endDate) {
    alert("Start date cannot be after end date.");
    return;
}

const params = new URLSearchParams();

if (startDate) {
    params.append("start_date", startDate);
}

if (endDate) {
    params.append("end_date", endDate);
}

const apiUrl = `/api/dashboard?${params.toString()}`;

    const refreshButton = document.getElementById("refreshBtn");

    refreshButton.disabled = true;
    refreshButton.textContent = "Loading...";

    try {

        const response = await fetch(apiUrl);
        if (!response.ok) {
            throw new Error("Server returned HTTP " + response.status);
        }

        const data = await response.json();

        if (!data.success) {
            throw new Error(data.error || "Unable to load dashboard data");
        }

        // Update all dashboard components
        updateKPIs(data.kpis);

        createProductionChart(data.daily_production);

        createMaterialChart(data.materials);

        updateMachineTable(data.machines);

        updateMaterialTable(data.materials);

        updateRecentTable(data.recent_runs);

        console.log("Dashboard data loaded successfully!");

    } catch (error) {

        console.error("Dashboard error:", error);

        alert("Unable to load dashboard data: " + error.message);

    } finally {

        refreshButton.disabled = false;
        refreshButton.textContent = "↻ Refresh Data";

    }
}


// Refresh button
document.getElementById("refreshBtn").addEventListener(
    "click",
    loadDashboard
);

// Apply date filter
document.getElementById("filterBtn").addEventListener("click", () => {
    loadDashboard();
});

// Reset date filter
document.getElementById("resetFilterBtn").addEventListener("click", () => {
    document.getElementById("startDate").value = "";
    document.getElementById("endDate").value = "";

    loadDashboard();
});


// Load dashboard when page opens
document.addEventListener("DOMContentLoaded", loadDashboard);

// Export CSV report
document.getElementById("exportBtn").addEventListener("click", () => {

    const startDate = document.getElementById("startDate").value;
    const endDate = document.getElementById("endDate").value;

    if (startDate && endDate && startDate > endDate) {
        alert("Start date cannot be after end date.");
        return;
    }

    const params = new URLSearchParams();

    if (startDate) {
        params.append("start_date", startDate);
    }

    if (endDate) {
        params.append("end_date", endDate);
    }

    window.location.href = `/api/export/csv?${params.toString()}`;
});

// Export PDF report
document.getElementById("exportPdfBtn").addEventListener("click", () => {

    const startDate = document.getElementById("startDate").value;
    const endDate = document.getElementById("endDate").value;

    if (startDate && endDate && startDate > endDate) {
        alert("Start date cannot be after end date.");
        return;
    }

    const params = new URLSearchParams();

    if (startDate) {
        params.append("start_date", startDate);
    }

    if (endDate) {
        params.append("end_date", endDate);
    }

    window.location.href = `/api/export/pdf?${params.toString()}`;
});
