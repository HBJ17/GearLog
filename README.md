# GearLog — Two-Wheeler Service System

GearLog is a two-wheeler service center management system built on a hybrid architecture: a Python Flask web server orchestrates a compiled C backend (`service.exe`) that handles all data manipulation. The C core now implements classical data structures and algorithms as part of the project's academic requirements.

## Features

- **Dashboard & Analytics** — Real-time stats: total, pending, in-progress, completed, and overdue jobs.
- **Job Management** — Add vehicles for service with owner details, engine number, service type, and delivery date.
- **Service Tracking** — Priority computed live from delivery date (days remaining); negative = overdue.
- **Undo Support** — Last add or update can be reverted via the `undo` command; state persists across runs in `undo.txt`.
- **FIFO Queue** — Jobs within the same service category are served in the order they arrived (`next_job` command).
- **Customer Portal** — Separate user-facing view at `/user` for customers to search their own job cards by phone or reg number.
- **SSR Frontend** — Jinja2 templates; no client-side JS fetching. Fast page loads, clean server-side flash messages.

## Architecture

| Layer | Technology | Role |
|---|---|---|
| Frontend | HTML + CSS + Jinja2 | Templates rendered server-side by Flask |
| Web Server | Python Flask | Routes, form handling, subprocess calls to C backend |
| Core Engine | C (`service.c` → `service.exe`) | All data structures, file I/O, sort, search |
| Storage | Flat-file (`jobs.txt`, `undo.txt`) | Pipe-delimited, append-only; rebuilt in memory each run |

## Data Structures & Algorithms (C Backend)

| Concept | Implementation |
|---|---|
| Singly Linked List | Per-vehicle chronological job history (`HistoryNode` / `VehicleList`) |
| Stack | Session undo — persisted in `data/undo.txt` (`StackNode`) |
| Queue | FIFO job processing per `service_type` (`QueueNode` / `Queue`) |
| Insertion Sort | Sorts jobs by priority before every write-back |
| Linear Search | Reusable search across `reg_no`, `owner_name`, `engine_no`, `phone` |
| Binary Search | Fast job lookup by ID on a sorted copy |

## Folder Structure

```
files/
├── backend/
│   ├── service.c          # C source — all data structures + algorithms
│   └── service.exe        # Compiled binary called by Flask
├── data/
│   ├── jobs.txt           # Primary pipe-delimited job store
│   └── undo.txt           # Undo stack persistence file
├── frontend/
│   ├── style.css
│   └── templates/
│       ├── base.html
│       ├── index.html
│       ├── jobs.html
│       ├── user_base.html
│       ├── user_search.html
│       └── user_view.html
└── server/
    └── server.py          # Flask routes + subprocess bridge
```

## CLI Commands (`service.exe`)

```bash
service.exe add <reg> <owner> <phone> <engine> <service_type> <date> [extra]
service.exe update <id> <status> [extra]
service.exe undo
service.exe next_job <service_type>
service.exe search <field> <value>     # field: reg_no | owner_name | engine_no | phone
```

## How to Run

1. Install **Python 3.x** and **GCC**.
2. Install Python dependencies:
   ```bash
   pip install flask flask-cors
   ```
3. Compile the C backend (if needed):
   ```bash
   cd files/backend
   gcc service.c -o service.exe
   ```
4. Start the server:
   ```bash
   cd files/server
   python server.py
   ```
5. Open `http://localhost:5000` — mechanic dashboard.  
   Open `http://localhost:5000/user` — customer portal.

## Branches

| Branch | Purpose |
|---|---|
| `main` | Stable, documented baseline |
| `Oreo` | Feature branch — user-facing customer portal |
| `scrap-work` | Data structure integration (linked list, stack, queue, sort, search) |
