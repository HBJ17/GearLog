import subprocess
import os
from flask import Flask, render_template, request, redirect, url_for, flash
from flask_cors import CORS
import datetime as _dt

BASE_DIR    = os.path.dirname(os.path.abspath(__file__))
TMPL_DIR    = os.path.join(BASE_DIR, "..", "frontend", "templates")
STATIC_DIR  = os.path.join(BASE_DIR, "..", "frontend")
BACKEND_DIR = os.path.join(BASE_DIR, "..", "backend")
DATA_FILE   = os.path.join(BASE_DIR, "..", "data", "jobs.txt")

app = Flask(
    __name__,
    template_folder=TMPL_DIR,
    static_folder=STATIC_DIR,
    static_url_path="/static"
)
app.secret_key = "super_secret_moto_key"
CORS(app)


def parse_jobs():
    today = _dt.date.today()
    jobs  = []
    try:
        with open(DATA_FILE, "r") as fh:
            for line in fh:
                line = line.strip()

                if not line or line.startswith("#"):
                    continue

                parts = line.split("|")

                if len(parts) < 10:
                    continue

                try:
                    due = _dt.date.fromisoformat(parts[6])
                    priority = (due - today).days
                except ValueError:
                    priority = int(parts[8])

                jobs.append({
                    "id":            int(parts[0]),
                    "reg_no":        parts[1],
                    "owner_name":    parts[2],
                    "phone":         parts[3],
                    "engine_no":     parts[4],
                    "service_type":  parts[5],
                    "delivery_date": parts[6],
                    "status":        parts[7],
                    "priority":      priority,
                    "extra":         parts[9],
                })

    except FileNotFoundError:
        pass

    return sorted(jobs, key=lambda j: j["priority"])


def run_exe(exe_name, *args):
    try:
        exe_path = os.path.join(BACKEND_DIR, exe_name)
        result = subprocess.run(
            [exe_path] + [str(a) for a in args],
            capture_output=True,
            text=True,
            timeout=10,
            cwd=BACKEND_DIR
        )

        output = (result.stdout + result.stderr).strip()
        return result.returncode == 0, output

    except FileNotFoundError:
        return False, f"{exe_name} not found at: {exe_path}"
    except subprocess.TimeoutExpired:
        return False, f"{exe_name} timed out (took too long)"
    except Exception as e:
        return False, str(e)


@app.route("/")
def dashboard():
    jobs = parse_jobs()

    active = [j for j in jobs if j["status"] != "completed"]

    history = [j for j in jobs if j["status"] == "completed"]

    stats = {
        "total":       len(jobs),
        "pending":     sum(1 for j in jobs if j["status"] == "pending"),
        "in_progress": sum(1 for j in jobs if j["status"] == "in_progress"),
        "completed":   sum(1 for j in jobs if j["status"] == "completed"),
        "overdue":     sum(1 for j in jobs if j["priority"] == 0 and j["status"] != "completed"),
    }

    return render_template("index.html", stats=stats, active=active, history=history)

@app.route("/job/add", methods=["GET", "POST"])
def add_job():
    if request.method == "POST":
        reg_no   = request.form.get("reg_no", "").strip()
        owner    = request.form.get("owner_name", "").strip()
        phone    = request.form.get("phone", "").strip()
        engine   = request.form.get("engine_no", "").strip()
        service  = request.form.get("service_type", "").strip()
        delivery = request.form.get("delivery_date", "").strip()
        extra    = request.form.get("extra", "").strip()

        ok, msg = run_exe("service.exe", "add", reg_no, owner, phone, engine, service, delivery, extra)
        if ok:
            flash("Job added successfully!", "success")
            return redirect(url_for("dashboard"))
        else:
            flash(f"Error adding job: {msg}", "error")
            return redirect(url_for("add_job"))

    return render_template("jobs.html", action="add")

@app.route("/job/<int:job_id>/update", methods=["GET", "POST"])
def update_job(job_id):
    jobs = parse_jobs()
    job = next((j for j in jobs if j["id"] == job_id), None)
    if not job:
        flash("Job not found", "error")
        return redirect(url_for("dashboard"))

    if request.method == "POST":
        status = request.form.get("status", "").strip()
        extra  = request.form.get("extra", "").strip()

        ok, msg = run_exe("service.exe", "update", job_id, status, extra)
        if ok:
            flash(f"Job J-{job_id} updated successfully!", "success")
            return redirect(url_for("dashboard"))
        else:
            flash(f"Error updating job: {msg}", "error")

    return render_template("jobs.html", action="update", job=job)


@app.route("/jobs")
def list_jobs():
    jobs          = parse_jobs()
    search        = request.args.get("search", "").strip().lower()
    status_filter = request.args.get("status_filter", "").strip().lower()

    if search:
        jobs = [j for j in jobs if any(
            search in j[field].lower()
            for field in ("reg_no", "owner_name", "engine_no", "phone")
        )]

    if status_filter:
        jobs = [j for j in jobs if j["status"].lower() == status_filter]

    return render_template("jobs.html", jobs=jobs, search=search, status_filter=status_filter)

@app.route("/user")
def user_dashboard():
    return render_template("user_search.html")

@app.route("/user/search", methods=["GET"])
def user_search_results():
    query = request.args.get("query", "").strip().lower()
    if not query:
        return redirect(url_for("user_dashboard"))

    jobs = parse_jobs()

    matching_jobs = [j for j in jobs if query in j["phone"].lower() or query in j["reg_no"].lower()]

    active_jobs  = [j for j in matching_jobs if j["status"] != "completed"]
    history_jobs = [j for j in matching_jobs if j["status"] == "completed"]

    return render_template("user_view.html", query=query, active=active_jobs, history=history_jobs)

if __name__ == "__main__":
    app.run(debug=True, port=5000)
