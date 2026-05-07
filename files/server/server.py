import subprocess  
import os          
import threading
import atexit
from flask import Flask, render_template, request, redirect, url_for, flash, session
from flask_cors import CORS
from functools import wraps
import datetime as _dt

BASE_DIR    = os.path.dirname(os.path.abspath(__file__))          
TMPL_DIR    = os.path.join(BASE_DIR, "..", "frontend", "templates") 
STATIC_DIR  = os.path.join(BASE_DIR, "..", "frontend")             
BACKEND_DIR = os.path.join(BASE_DIR, "..", "backend")               
DATA_FILE   = os.path.join(BASE_DIR, "..", "data", "jobs.txt") 

backend_proc = None
backend_lock = threading.Lock()

def init_backend():
    global backend_proc
    exe_path = os.path.join(BACKEND_DIR, "service.exe")
    if not os.path.exists(exe_path) and os.path.exists(os.path.join(BACKEND_DIR, "service")):
        exe_path = os.path.join(BACKEND_DIR, "service")
    try:
        backend_proc = subprocess.Popen(
            [exe_path, "daemon"],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            text=True,           
            cwd=BACKEND_DIR       
        )
    except FileNotFoundError:
        pass

def close_backend():
    global backend_proc
    if backend_proc:
        try:
            backend_proc.stdin.write("EXIT\n")
            backend_proc.stdin.flush()
            backend_proc.wait(timeout=2)
        except:
            pass

init_backend()
atexit.register(close_backend)

def send_command(cmd_str):
    global backend_proc
    if not backend_proc or backend_proc.poll() is not None:
        init_backend()
        
    with backend_lock:
        if not backend_proc: return False, "Backend not running"
        try:
            backend_proc.stdin.write(cmd_str + "\n")
            backend_proc.stdin.flush()
            if cmd_str == "GET_ALL" or cmd_str.startswith("SEARCH|"):
                lines = []
                end_marker = "END_GET_ALL" if cmd_str == "GET_ALL" else "END_SEARCH"
                while True:
                    line = backend_proc.stdout.readline()
                    if not line or line.strip() == end_marker:
                        break
                    lines.append(line)
                return True, lines
            else:
                out = backend_proc.stdout.readline().strip()
                return "successfully" in out.lower(), out
        except Exception as e:
            return False, str(e)


app = Flask(
    __name__,
    template_folder=TMPL_DIR,      
    static_folder=STATIC_DIR,      
    static_url_path="/static"
)
app.secret_key = "super_secret_moto_key"
CORS(app)

def admin_required(f):
    @wraps(f)
    def decorated_function(*args, **kwargs):
        if session.get("role") != "admin":
            flash("Please log in as an administrator to access this page.", "error")
            return redirect(url_for('login'))
        return f(*args, **kwargs)
    return decorated_function

def user_required(f):
    @wraps(f)
    def decorated_function(*args, **kwargs):
        if session.get("role") not in ["user", "admin"]:
            flash("Please log in to access this page.", "error")
            return redirect(url_for('login'))
        return f(*args, **kwargs)
    return decorated_function




def parse_jobs(cmd="GET_ALL"):
    today = _dt.date.today()  
    jobs  = []
    
    ok, lines = send_command(cmd)
    if not ok:
        return []
    
    jobs = []
    
    for line in lines:
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
            "id":            int(parts[0]),  # unique job number
            "reg_no":        parts[1],       # vehicle registration number
            "owner_name":    parts[2],       # owner's full name
            "phone":         parts[3],       # owner's phone number
            "engine_no":     parts[4],       # engine number
            "service_type":  parts[5],       # oil / brake / maintenance / full
            "delivery_date": parts[6],       # expected delivery date (YYYY-MM-DD)
            "status":        parts[7],       # pending / in_progress / completed
            "priority":      priority,       # live days until due (negative = overdue)
            "extra":         parts[9],       # optional notes
        })

    # The C Backend Min-Heap automatically yields completely sorted jobs!
    return jobs


@app.route("/")
@admin_required
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
@admin_required
def add_job():
    if request.method == "POST":
        reg_no = request.form.get("reg_no", "").strip()
        owner = request.form.get("owner_name", "").strip()
        phone = request.form.get("phone", "").strip()
        engine = request.form.get("engine_no", "").strip()
        service = request.form.get("service_type", "").strip()
        delivery = request.form.get("delivery_date", "").strip()
        extra = request.form.get("extra", "").strip()
        
        cmd = f"ADD|{reg_no}|{owner}|{phone}|{engine}|{service}|{delivery}|{extra}"
        ok, msg = send_command(cmd)
        if ok:
            flash("Job added successfully!", "success")
            return redirect(url_for("dashboard"))
        else:
            flash(f"Error adding job: {msg}", "error")
            return redirect(url_for("add_job"))
            
    return render_template("jobs.html", action="add")

@app.route("/job/<int:job_id>/update", methods=["GET", "POST"])
@admin_required
def update_job(job_id):
    jobs = parse_jobs()
    job = next((j for j in jobs if j["id"] == job_id), None)
    if not job:
        flash("Job not found", "error")
        return redirect(url_for("dashboard"))

    if request.method == "POST":
        status = request.form.get("status", "").strip()
        delivery_date = request.form.get("delivery_date", "").strip()
        extra = request.form.get("extra", "").strip()
        
        cmd = f"UPDATE|{job_id}|{status}|{delivery_date}|{extra}"
        ok, msg = send_command(cmd)
        if ok:
            flash(f"Job J-{job_id} updated successfully!", "success")
            return redirect(url_for("dashboard"))
        else:
            flash(f"Error updating job: {msg}", "error")
            
    return render_template("jobs.html", action="update", job=job)

    
@app.route("/jobs")
@admin_required
def list_jobs():
    search = request.args.get("search", "").strip().lower()
    status_filter = request.args.get("status_filter", "").strip().lower()

    if search:
        jobs = parse_jobs(f"SEARCH|{search}")
    else:
        jobs = parse_jobs()

    if status_filter:
        jobs = [j for j in jobs if j["status"].lower() == status_filter]

    return render_template("jobs.html", jobs=jobs, search=search, status_filter=status_filter)

@app.route("/user")
@user_required
def user_dashboard():
    return redirect(url_for("user_search_results"))

@app.route("/user/search", methods=["GET"])
@user_required
def user_search_results():
    name = session.get("name", "").strip().lower()
    phone = session.get("phone", "").strip()
    
    query = request.args.get("query", phone).strip().lower()
    if session.get("role") == "user":
        query = phone

    if not query:
        return redirect(url_for("user_dashboard"))

    matching_jobs = parse_jobs(f"SEARCH|{query}")
    
    if session.get("role") == "user":
        matching_jobs = [j for j in matching_jobs if j["owner_name"].lower() == name]
    
    active_jobs = [j for j in matching_jobs if j["status"] != "completed"]
    history_jobs = [j for j in matching_jobs if j["status"] == "completed"]
    
    return render_template("user_view.html", query=name.title() if name else query, active=active_jobs, history=history_jobs)

@app.route("/login", methods=["GET", "POST"])
def login():
    if request.method == "POST":
        identifier = request.form.get("identifier", "").strip()
        secret = request.form.get("secret", "").strip()

        if identifier == "admin" and secret == "admin123":
            session["role"] = "admin"
            session["name"] = "Admin"
            return redirect(url_for("dashboard"))

        jobs = parse_jobs()
        user_jobs = [j for j in jobs if j["owner_name"].lower() == identifier.lower() and j["phone"] == secret]
        if user_jobs:
            session["role"] = "user"
            session["name"] = user_jobs[0]["owner_name"]
            session["phone"] = user_jobs[0]["phone"]
            return redirect(url_for("user_dashboard"))
        
        flash("Invalid credentials. Please try again.", "error")
        return redirect(url_for("login"))
    return render_template("login.html")

@app.route("/logout")
def logout():
    session.clear()
    return redirect(url_for("login"))

if __name__ == "__main__":
    app.run(debug=True, port=5000)
