# AutoTrack — Two-Wheeler Service Management System
## Project Report

---

## 1. Project Overview

**AutoTrack** is a full-stack web application built to digitize and streamline the job management operations of a two-wheeler service center. Instead of maintaining manual job cards on paper or in disconnected spreadsheets, mechanics and service managers can use AutoTrack to:

- Register every incoming vehicle and its service requirement
- Track the status of each job (Pending → In Progress → Completed)
- Automatically prioritize jobs by their delivery deadlines
- Search for any job using partial registration numbers, owner names, phone numbers, or engine numbers
- Let customers view the real-time status of their own vehicle

---

## 2. Motivation & Problem Statement

Small two-wheeler service centers face a common set of operational challenges:

| Problem | Real-world Impact |
|---|---|
| Paper job cards get lost | Service records become irretrievable |
| No priority system | Urgent deliveries get delayed |
| No search capability | Finding a specific vehicle takes minutes |
| Customers call in to check status | Adds to front-desk workload |
| No history tracking | Cannot look up past service records |

AutoTrack solves all of these with a lightweight, dependency-minimal system that can run on even a basic Windows laptop at the service center — no internet connection required.

---

## 3. Key Features

### 3.1 Admin Panel
- **Dashboard** — Live statistics panel showing total, pending, in-progress, completed, and overdue job counts.
- **Priority Queue View** — Active jobs are always displayed sorted by urgency (most overdue first). This sorting is done by the C backend using a Min-Heap.
- **Add Job** — A structured form to create a new job card, recording all vehicle and owner details.
- **Update Job** — Edit the status, expected delivery date, and notes for any existing job.
- **All Jobs View** — A paginated table of all jobs with search and status filter support.

### 3.2 Customer Panel
- **Self-Service Search** — Customers log in with their name and phone number. The system automatically filters and shows only their vehicle's jobs.
- **Status Visibility** — Customers can see whether their bike is pending, being worked on, or ready for delivery.

### 3.3 Smart Search (Suffix Tree)
Any substring of a registration number, owner name, phone number, or engine number instantly retrieves matching jobs. For example, searching `9876` will find all jobs where `9876` appears anywhere in the phone number.

### 3.4 Priority-Based Sorting (Min-Heap)
Jobs are never displayed in raw file order. The C backend uses a Min-Heap that extracts jobs in ascending order of their delivery deadlines — the most urgently due job always appears at the top.

### 3.5 Persistent Flat-File Storage
All job records are stored in a plain-text file (`jobs.txt`) using a `|`-delimited format. This makes backups trivial (just copy the file) and eliminates any database server dependency.

---

## 4. Technology Stack

| Layer | Technology | Why |
|---|---|---|
| **Core Data Engine** | C (compiled to `service.exe`) | Maximum speed for data manipulation and algorithmic structures |
| **Web Framework** | Python 3 + Flask | Rapid development, excellent templating |
| **Template Engine** | Jinja2 (built into Flask) | Server-Side Rendering, no JavaScript framework needed |
| **Styling** | Vanilla CSS (`style.css`) | No external dependencies, full control |
| **Process Bridge** | Python `subprocess` module | Enables Python to talk to the C executable |
| **Data Storage** | Flat-file (`jobs.txt`) | Zero database server needed |
| **Cross-Origin Support** | Flask-CORS | Allows flexible deployment |

---

## 5. System Architecture

AutoTrack uses a **three-layer hybrid architecture**:

```
┌──────────────────────────────────────────────────────┐
│                   USER (Browser)                     │
│         HTML pages rendered by Flask/Jinja2          │
└───────────────────────┬──────────────────────────────┘
                        │  HTTP Requests
                        ▼
┌──────────────────────────────────────────────────────┐
│           FLASK WEB SERVER  (server.py)              │
│  • Routes (/, /jobs, /job/add, /job/<id>/update)     │
│  • Session management (admin / user roles)           │
│  • Calls send_command() to talk to C backend         │
│  • Renders Jinja2 HTML templates                     │
└───────────────────────┬──────────────────────────────┘
                        │  stdin/stdout pipe (subprocess)
                        ▼
┌──────────────────────────────────────────────────────┐
│         C BACKEND DAEMON  (service.exe daemon)       │
│  • Holds all jobs in memory (array of JobCard)       │
│  • Builds a Suffix Tree index for fast search        │
│  • Uses a Min-Heap to sort by priority               │
│  • Reads/writes jobs.txt for persistence             │
└───────────────────────┬──────────────────────────────┘
                        │  File I/O
                        ▼
┌──────────────────────────────────────────────────────┐
│          DATA STORE  (data/jobs.txt)                 │
│  Pipe-delimited plain text, one job per line         │
└──────────────────────────────────────────────────────┘
```

**Key architectural decision:** The C process is launched **once** when Flask starts and kept alive as a long-running daemon. Commands are sent over `stdin` and responses are read from `stdout`. This avoids the overhead of spawning a new process for every web request.

---

## 6. Folder Structure

```
College_pro/
├── documentation/          ← This documentation folder
├── files/
│   ├── backend/
│   │   ├── service.c       ← All C source code (523 lines)
│   │   └── service.exe     ← Compiled Windows executable
│   ├── data/
│   │   └── jobs.txt        ← Flat-file database
│   ├── frontend/
│   │   ├── style.css       ← All CSS styling
│   │   └── templates/
│   │       ├── base.html       ← Navigation layout template
│   │       ├── index.html      ← Admin dashboard
│   │       ├── jobs.html       ← Job list / Add / Update forms
│   │       ├── login.html      ← Login page
│   │       ├── user_base.html  ← Customer-facing base layout
│   │       ├── user_search.html← Customer search page
│   │       └── user_view.html  ← Customer job view
│   └── server/
│       └── server.py       ← Flask application (289 lines)
├── README.md
└── PROGRESS_REPORT.md
```

---

## 7. Data Flow — End-to-End Example

**Scenario: Admin adds a new job for a motorcycle.**

1. **Admin fills the form** at `/job/add` and clicks "Create Job".
2. **Browser sends** an HTTP POST request to Flask with the form fields.
3. **Flask route `add_job()`** extracts the form data and calls `send_command("ADD|MH01AB1234|Ravi|9876543210|ENG001|oil|2026-06-01|Optional note")`.
4. **`send_command()`** writes that string to the C daemon's `stdin` pipe.
5. **C daemon's `main()` loop** reads the line, parses the `|`-separated fields, creates a new `JobCard` struct, calculates its priority using `calc_priority()`, inserts it into the in-memory `mem_jobs[]` array, and also indexes it in the Suffix Tree via `st_index_job()`.
6. **C daemon prints** `"Added job 8 successfully.\n"` to `stdout`.
7. **Flask reads** the response, sees "successfully", sets a flash message, and redirects to the dashboard.
8. **Dashboard loads**, Flask calls `send_command("GET_ALL")`, C daemon dumps all jobs through the Min-Heap (priority-sorted), Flask parses them, and renders `index.html` with the updated table.

---

## 8. Authentication Model

| Role | Credentials | Access |
|---|---|---|
| **Admin** | Username: `admin` / Password: `admin123` | Full dashboard, add, update, search all jobs |
| **Customer (User)** | Username: their full name / Password: their phone number | Only sees their own vehicle's jobs |

Sessions are managed server-side via Flask's signed cookie session. The `admin_required` and `user_required` decorators protect every sensitive route.

---

## 9. Algorithms Used

### 9.1 Suffix Tree (for Search)
- Every string field of a job (reg_no, owner_name, phone, engine_no) is inserted into a **Suffix Tree** character by character.
- Every suffix of every string is inserted, meaning any substring can be found.
- Time complexity for search: **O(m)** where `m` is the query length.

### 9.2 Min-Heap (for Priority Sorting)
- When listing jobs, all matching `JobCard` pointers are inserted into a **Min-Heap** keyed by `priority` (days until due date).
- Jobs are extracted in ascending priority order, so the most urgent (or overdue) job appears first.
- Time complexity: **O(n log n)** for full sort.

### 9.3 Priority Calculation
- Priority = **number of days from today until the delivery date**.
- Negative value = overdue.
- Zero = due today.
- Positive = days remaining.

---

## 10. How to Run the Project

### Prerequisites
- Python 3.x installed
- GCC compiler (for recompiling C if needed)
- Windows OS (for `.exe`; Linux users rename to `service`)

### Steps
```bash
# Step 1: Install Python dependencies
pip install flask flask-cors

# Step 2: (Optional) Recompile C backend
cd files/backend
gcc service.c -o service.exe

# Step 3: Start Flask server
cd files/server
python server.py

# Step 4: Open browser
# Navigate to: http://localhost:5000
# Admin login: username=admin, password=admin123
```

---

## 11. Strengths & Design Decisions

| Decision | Reasoning |
|---|---|
| **C for the data engine** | Raw speed; no garbage collector pauses; direct memory control |
| **Daemon process model** | One C process serves all requests; no per-request spawn overhead |
| **Flat-file storage** | No database server needed; runs offline; easy backup |
| **Server-Side Rendering** | Pages load fast; no React/Vue overhead; works without client-side JS |
| **Suffix Tree for search** | O(m) search time regardless of how many jobs exist |
| **Min-Heap for display** | Jobs always sorted in priority order, automatically |

---

## 12. Limitations & Future Improvements

| Limitation | Proposed Fix |
|---|---|
| Single-user C daemon (not thread-safe) | Use mutex locks or switch to SQLite |
| Admin password hardcoded | Move to a config file or environment variable |
| Flat-file has no concurrent write protection | Add file locking |
| No pagination on large job lists | Implement server-side pagination |
| No email/SMS notifications | Integrate Twilio or SMTP |

---

*Report prepared for: AutoTrack — Two-Wheeler Service Management System*  
*Date: May 2026*
