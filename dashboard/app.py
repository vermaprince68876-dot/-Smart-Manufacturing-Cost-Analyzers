
from flask import Flask, jsonify, render_template, request, Response
import csv
import io
import sqlite3
from pathlib import Path

from reportlab.lib import colors
from reportlab.lib.pagesizes import A4
from datetime import datetime
from reportlab.lib.styles import getSampleStyleSheet
from reportlab.lib.units import inch
from reportlab.platypus import (
    SimpleDocTemplate,
    Paragraph,
    Spacer,
    Table,
    TableStyle
)

from reportlab.graphics.shapes import Drawing
from reportlab.graphics.charts.barcharts import VerticalBarChart

app = Flask(__name__)

# Locate the existing project database
PROJECT_DIR = Path(__file__).resolve().parent.parent
DATABASE_PATH = PROJECT_DIR / "factory.db"


# Connect to SQLite in read-only mode
def get_db_connection():
    if not DATABASE_PATH.exists():
        raise FileNotFoundError(
            f"Database not found: {DATABASE_PATH}"
        )

    connection = sqlite3.connect(
        f"file:{DATABASE_PATH.as_posix()}?mode=ro",
        uri=True
    )

    connection.row_factory = sqlite3.Row
    return connection


# Convert SQLite rows into normal dictionaries
def rows_to_dict(rows):
    return [dict(row) for row in rows]


# Dashboard home page
@app.route("/")
def home():
    return render_template("index.html")


# Dashboard data API
@app.route("/api/dashboard")
def dashboard_data():

    start_date = request.args.get("start_date")
    end_date = request.args.get("end_date")

    date_filter = ""
    params = []

    if start_date and end_date:
        date_filter = "WHERE date(r.run_date) BETWEEN date(?) AND date(?)"
        params = [start_date, end_date]
    elif start_date:
        date_filter = "WHERE date(r.run_date) >= date(?)"
        params = [start_date]
    elif end_date:
        date_filter = "WHERE date(r.run_date) <= date(?)"
        params = [end_date]

    try:
        connection = get_db_connection()

        # 1. Overall manufacturing statistics
        kpi_query = f"""
            SELECT
                COUNT(r.id) AS total_runs,

                COALESCE(SUM(r.quantity_produced), 0)
                    AS total_produced,

                COALESCE(SUM(r.quantity_rejected), 0)
                    AS total_rejected,

                COALESCE(
                    SUM(r.quantity_produced - r.quantity_rejected),
                    0
                ) AS good_units,

                COALESCE(
                    SUM(
                        r.material_kg_used * mat.cost_per_kg
                        + r.machine_hours * m.hourly_rate
                        + m.setup_cost_per_run
                    ),
                    0
                ) AS total_cost

            FROM production_runs r
JOIN machines m ON m.id = r.machine_id
JOIN materials mat ON mat.id = r.material_id
{date_filter};
        """

        kpi = connection.execute(kpi_query,params).fetchone()

        total_good = kpi["good_units"]
        total_cost = kpi["total_cost"]

        average_cost = (
            total_cost / total_good
            if total_good > 0 else 0
        )

        rejection_rate = (
            kpi["total_rejected"] / kpi["total_produced"] * 100
            if kpi["total_produced"] > 0 else 0
        )

        # 2. Machine-wise efficiency
        machine_date_filter = date_filter.replace("WHERE", "AND", 1)

        machine_query = f"""
            SELECT
                m.id AS machine_id,
                m.name AS machine_name,

                COALESCE(SUM(r.quantity_produced), 0)
                    AS produced,

                COALESCE(SUM(r.quantity_rejected), 0)
                    AS rejected,

                COALESCE(SUM(r.machine_hours), 0)
                    AS machine_hours,

                m.ideal_units_per_hour

            FROM machines m

            LEFT JOIN production_runs r ON r.machine_id = m.id {machine_date_filter}

            GROUP BY m.id, m.name

            ORDER BY m.id;
        """

        machines = rows_to_dict(connection.execute(machine_query, params).fetchall())

        for machine in machines:
            expected = (
                machine["machine_hours"]
                * machine["ideal_units_per_hour"]
            )

            machine["efficiency"] = (
                round(machine["produced"] / expected * 100, 2)
                if expected > 0 else 0
            )

        # 3. Daily production analysis
        daily_query = f"""
            SELECT
                date(run_date) AS production_date,
                SUM(quantity_produced) AS produced,
                SUM(quantity_rejected) AS rejected

            FROM production_runs r
            {date_filter}

            GROUP BY date(run_date)

            ORDER BY production_date;
        """

        daily_production = rows_to_dict(
            connection.execute(daily_query,params).fetchall()
        )

        # 4. Material-wise cost analysis
        material_query = f"""
    SELECT
        mat.name AS material_name,
        SUM(r.material_kg_used) AS total_kg,
        SUM(r.material_kg_used * mat.cost_per_kg) AS material_cost
    FROM production_runs r
    JOIN materials mat ON mat.id = r.material_id
    {date_filter}
    GROUP BY mat.id, mat.name
    ORDER BY material_cost DESC;
"""

        materials = rows_to_dict(
            connection.execute(material_query,params).fetchall()
        )

        # 5. Recent production records
        recent_query = f"""
         SELECT
          r.id,
          r.run_date,
          m.name AS machine_name,
          mat.name AS material_name,
          r.quantity_produced AS produced,
          r.quantity_rejected AS rejected,
          (r.quantity_produced - r.quantity_rejected) AS good_units
        FROM production_runs r
      JOIN machines m ON m.id = r.machine_id
       JOIN materials mat ON mat.id = r.material_id
      {date_filter}
      ORDER BY r.run_date DESC, r.id DESC
      LIMIT 10;
        """

        recent_runs = rows_to_dict(
            connection.execute(recent_query,params).fetchall()
        )

        connection.close()

        return jsonify({
            "success": True,

            "kpis": {
                "total_runs": kpi["total_runs"],
                "total_produced": kpi["total_produced"],
                "total_rejected": kpi["total_rejected"],
                "good_units": total_good,
                "total_cost": round(total_cost, 2),
                "average_cost": round(average_cost, 2),
                "rejection_rate": round(rejection_rate, 2)
            },

            "machines": machines,
            "daily_production": daily_production,
            "materials": materials,
            "recent_runs": recent_runs
        })

    except Exception as error:
        return jsonify({
            "success": False,
            "error": str(error)
        }), 500
       



# CSV export route
@app.route("/api/export/csv")
def export_csv():

    start_date = request.args.get("start_date")
    end_date = request.args.get("end_date")

    date_filter = ""
    params = []

    if start_date and end_date:
        date_filter = "WHERE date(r.run_date) BETWEEN date(?) AND date(?)"
        params = [start_date, end_date]
    elif start_date:
        date_filter = "WHERE date(r.run_date) >= date(?)"
        params = [start_date]
    elif end_date:
        date_filter = "WHERE date(r.run_date) <= date(?)"
        params = [end_date]

    connection = get_db_connection()

    query = f"""
        SELECT
            r.id,
            r.run_date,
            m.name AS machine_name,
            mat.name AS material_name,
            r.quantity_produced,
            r.quantity_rejected,
            (r.quantity_produced - r.quantity_rejected) AS good_units,
            r.machine_hours,
            r.material_kg_used
        FROM production_runs r
        JOIN machines m ON m.id = r.machine_id
        JOIN materials mat ON mat.id = r.material_id
        {date_filter}
        ORDER BY r.run_date DESC, r.id DESC;
    """

    rows = connection.execute(query, params).fetchall()
    connection.close()

    output = io.StringIO()
    writer = csv.writer(output)

    writer.writerow([
        "Run ID", "Date", "Machine", "Material",
        "Produced", "Rejected", "Good Units",
        "Machine Hours", "Material Used (KG)"
    ])

    for row in rows:
        writer.writerow(row)

    return Response(
        output.getvalue(),
        mimetype="text/csv",
        headers={
            "Content-Disposition": "attachment; filename=production_report.csv"
        }
    )

@app.route("/api/export/pdf")
def export_pdf():
    start_date = request.args.get("start_date")
    end_date = request.args.get("end_date")

    date_filter = ""
    params = []

    if start_date and end_date:
        date_filter = "WHERE date(r.run_date) BETWEEN date(?) AND date(?)"
        params = [start_date, end_date]
    elif start_date:
        date_filter = "WHERE date(r.run_date) >= date(?)"
        params = [start_date]
    elif end_date:
        date_filter = "WHERE date(r.run_date) <= date(?)"
        params = [end_date]

    machine_date_filter = date_filter.replace("WHERE", "AND", 1)

    conn = sqlite3.connect(
        f"file:{DATABASE_PATH}?mode=ro",
        uri=True
    )
    conn.row_factory = sqlite3.Row

    try:
        # KPI summary
        kpi = conn.execute(f"""
            SELECT
                COUNT(r.id) AS total_runs,
                COALESCE(SUM(r.quantity_produced), 0) AS total_produced,
                COALESCE(SUM(r.quantity_rejected), 0) AS total_rejected,
                COALESCE(SUM(r.quantity_produced - r.quantity_rejected), 0) AS good_units,
                COALESCE(SUM(
                    r.material_kg_used * mat.cost_per_kg
                    + r.machine_hours * m.hourly_rate
                    + m.setup_cost_per_run
                ), 0) AS total_cost
            FROM production_runs r
            JOIN machines m ON m.id = r.machine_id
            JOIN materials mat ON mat.id = r.material_id
            {date_filter}
        """, params).fetchone()

        total_runs = kpi["total_runs"]
        good_units = kpi["good_units"]
        average_cost = kpi["total_cost"] / good_units if good_units else 0
        rejection_rate = (
            kpi["total_rejected"] / kpi["total_produced"] * 100
            if kpi["total_produced"] else 0
        )

        # Machine-wise efficiency
        machines = conn.execute(f"""
            SELECT
                m.name,
                COALESCE(SUM(r.quantity_produced), 0) AS produced,
                COALESCE(SUM(r.quantity_rejected), 0) AS rejected,
                COALESCE(SUM(r.machine_hours), 0) AS hours,
                m.ideal_units_per_hour
            FROM machines m
            LEFT JOIN production_runs r
                ON r.machine_id = m.id {machine_date_filter}
            GROUP BY m.id, m.name, m.ideal_units_per_hour
            ORDER BY m.name
        """, params).fetchall()

        # Material cost analysis
        materials = conn.execute(f"""
            SELECT
                mat.name,
                COALESCE(SUM(r.material_kg_used), 0) AS kg_used,
                mat.cost_per_kg,
                COALESCE(SUM(r.material_kg_used * mat.cost_per_kg), 0) AS total_material_cost
            FROM materials mat
            LEFT JOIN production_runs r
                ON r.material_id = mat.id {machine_date_filter}
            GROUP BY mat.id, mat.name, mat.cost_per_kg
            ORDER BY mat.name
        """, params).fetchall()

        # Production records
        records = conn.execute(f"""
            SELECT
                r.run_date,
                m.name AS machine_name,
                mat.name AS material_name,
                r.quantity_produced,
                r.quantity_rejected,
                r.machine_hours
            FROM production_runs r
            JOIN machines m ON m.id = r.machine_id
            JOIN materials mat ON mat.id = r.material_id
            {date_filter}
            ORDER BY r.run_date DESC, r.id DESC
        """, params).fetchall()

    finally:
        conn.close()

    # Build PDF in memory
    buffer = io.BytesIO()

    doc = SimpleDocTemplate(
        buffer,
        pagesize=A4,
        rightMargin=30,
        leftMargin=30,
        topMargin=35,
        bottomMargin=35
    )

    styles = getSampleStyleSheet()
    story = []

    story.append(Paragraph("Smart Manufacturing Cost Analyzer", styles["Title"]))
    story.append(Spacer(1, 8))
    story.append(Paragraph("Manufacturing Performance Report", styles["Heading2"]))

    if start_date or end_date:
        period = f"Date range: {start_date or 'Beginning'} to {end_date or 'Latest'}"
    else:
        period = "Date range: All available records"

    story.append(Paragraph(period, styles["Normal"]))
    story.append(Spacer(1, 15))

    # KPI table
    story.append(Paragraph("1. Manufacturing KPI Summary", styles["Heading2"]))

    kpi_data = [
        ["Metric", "Value"],
        ["Total Production Runs", str(total_runs)],
        ["Total Units Produced", f"{kpi['total_produced']:,}"],
        ["Rejected Units", f"{kpi['total_rejected']:,}"],
        ["Good Units", f"{good_units:,}"],
        ["Total Manufacturing Cost", f"INR {kpi['total_cost']:,.2f}"],
        ["Average Cost per Good Unit", f"INR {average_cost:,.2f}"],
        ["Rejection Rate", f"{rejection_rate:.2f}%"]
    ]

    table = Table(kpi_data, colWidths=[260, 250], repeatRows=1)
    table.setStyle(TableStyle([
        ("BACKGROUND", (0, 0), (-1, 0), colors.HexColor("#263b5e")),
        ("TEXTCOLOR", (0, 0), (-1, 0), colors.white),
        ("GRID", (0, 0), (-1, -1), 0.5, colors.grey),
        ("PADDING", (0, 0), (-1, -1), 7),
        ("FONTNAME", (0, 0), (-1, 0), "Helvetica-Bold")
    ]))
    story.append(table)
    story.append(Spacer(1, 18))

    # Machine efficiency table
    story.append(Paragraph("2. Machine-wise Efficiency", styles["Heading2"]))

    machine_data = [[
        "Machine", "Produced", "Rejected", "Hours", "Efficiency"
    ]]

    for machine in machines:
        capacity = machine["ideal_units_per_hour"] * machine["hours"]
        efficiency = (
            machine["produced"] / capacity * 100 if capacity else 0
        )

        machine_data.append([
            machine["name"],
            f"{machine['produced']:,}",
            f"{machine['rejected']:,}",
            f"{machine['hours']:.2f}",
            f"{efficiency:.2f}%"
        ])

    table = Table(machine_data, colWidths=[145, 85, 75, 75, 80], repeatRows=1)
    table.setStyle(TableStyle([
        ("BACKGROUND", (0, 0), (-1, 0), colors.HexColor("#263b5e")),
        ("TEXTCOLOR", (0, 0), (-1, 0), colors.white),
        ("GRID", (0, 0), (-1, -1), 0.5, colors.grey),
        ("PADDING", (0, 0), (-1, -1), 6),
        ("FONTNAME", (0, 0), (-1, 0), "Helvetica-Bold")
    ]))
    story.append(table)
    story.append(Spacer(1, 18))

    # Machine efficiency chart
    story.append(Paragraph(
        "Machine-wise Production Efficiency Chart",
        styles["Heading2"]
    ))

    drawing = Drawing(500, 230)
    chart = VerticalBarChart()
    chart.x = 45
    chart.y = 35
    chart.height = 160
    chart.width = 420

    chart.data = [[
        machine["produced"] /
        (machine["ideal_units_per_hour"] * machine["hours"]) * 100
        if machine["hours"] > 0 else 0
        for machine in machines
    ]]

    chart.categoryAxis.categoryNames = [machine["name"] for machine in machines]
    chart.valueAxis.valueMin = 0
    chart.valueAxis.valueMax = 120
    chart.valueAxis.valueStep = 20

    drawing.add(chart)
    story.append(drawing)
    story.append(Spacer(1, 18))

    # Material cost table
    story.append(Paragraph("3. Material Cost Analysis", styles["Heading2"]))

    material_data = [["Material", "Used (kg)", "Rate/kg", "Total Cost"]]

    for material in materials:
        material_data.append([
            material["name"],
            f"{material['kg_used']:.2f}",
            f"INR {material['cost_per_kg']:.2f}",
            f"INR {material['total_material_cost']:,.2f}"
        ])

    table = Table(material_data, colWidths=[145, 90, 105, 120], repeatRows=1)
    table.setStyle(TableStyle([
        ("BACKGROUND", (0, 0), (-1, 0), colors.HexColor("#263b5e")),
        ("TEXTCOLOR", (0, 0), (-1, 0), colors.white),
        ("GRID", (0, 0), (-1, -1), 0.5, colors.grey),
        ("PADDING", (0, 0), (-1, -1), 6),
        ("FONTNAME", (0, 0), (-1, 0), "Helvetica-Bold")
    ]))
    story.append(table)
    story.append(Spacer(1, 18))

    # Material cost comparison chart
    story.append(Paragraph(
        "Material-wise Cost Comparison Chart",
        styles["Heading2"]
    ))

    drawing = Drawing(500, 230)
    chart = VerticalBarChart()
    chart.x = 45
    chart.y = 35
    chart.height = 160
    chart.width = 420

    chart.data = [[material["total_material_cost"] for material in materials]]
    chart.categoryAxis.categoryNames = [material["name"] for material in materials]
    chart.valueAxis.valueMin = 0

    max_cost = max(
        [material["total_material_cost"] for material in materials],
        default=0
    )

    chart.valueAxis.valueMax = max_cost * 1.2 if max_cost > 0 else 1
    chart.valueAxis.valueStep = max_cost * 1.2 / 5 if max_cost > 0 else 1

    drawing.add(chart)
    story.append(drawing)
    story.append(Spacer(1, 18))

    # Production records table
    story.append(Paragraph("4. Production Records", styles["Heading2"]))

    record_data = [[
        "Date", "Machine", "Material", "Produced", "Rejected", "Hours"
    ]]

    for record in records:
        record_data.append([
            str(record["run_date"]),
            record["machine_name"],
            record["material_name"],
            f"{record['quantity_produced']:,}",
            f"{record['quantity_rejected']:,}",
            f"{record['machine_hours']:.2f}"
        ])

    table = Table(record_data, colWidths=[75, 105, 95, 65, 65, 55], repeatRows=1)
    table.setStyle(TableStyle([
        ("BACKGROUND", (0, 0), (-1, 0), colors.HexColor("#263b5e")),
        ("TEXTCOLOR", (0, 0), (-1, 0), colors.white),
        ("GRID", (0, 0), (-1, -1), 0.5, colors.grey),
        ("PADDING", (0, 0), (-1, -1), 5),
        ("FONTNAME", (0, 0), (-1, 0), "Helvetica-Bold"),
        ("FONTSIZE", (0, 0), (-1, -1), 8)
    ]))
    story.append(table)

    def add_page_number(canvas, doc):
        canvas.saveState()
        canvas.setFont("Helvetica", 9)
        canvas.drawString(40, 25, "Smart Manufacturing Cost Analyzer")
        canvas.drawRightString(
            A4[0] - 40,
            25,
            f"Page {doc.page} | Generated: {datetime.now().strftime('%d-%m-%Y %H:%M')}"
        )
        canvas.restoreState()

    doc.build(
        story,
        onFirstPage=add_page_number,
        onLaterPages=add_page_number
    )

    buffer.seek(0)


    return Response(
        buffer.getvalue(),
        mimetype="application/pdf",
        headers={
            "Content-Disposition":
                "attachment; filename=manufacturing_report.pdf"
        }
    )
 # Start Flask server
if __name__ == "__main__":
    print("Starting Smart Manufacturing Dashboard...")
    print(f"Database: {DATABASE_PATH}")

    app.run(
        host="127.0.0.1",
        port=5000,
        debug=True
    )