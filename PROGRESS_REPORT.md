# Progress Report — GearLog

**Date:** 2026-04-08  
**Status:** Active — Data Structure Integration Complete

---

## Completed Milestones

### 1. Initial Architecture & Setup
- Designed hybrid architecture: Python Flask as the web-layer orchestrator, compiled C binary as the data engine.
- Established modular folder layout: `backend/`, `frontend/`, `server/`, `data/`.

### 2. Core C Backend (`service.c`)
- Implemented pipe-delimited flat-file storage (`jobs.txt`) with append-only writes and full rebuild-from-file on each run.
- `jc_add` and `jc_update` form the stable public API consumed by Flask via `subprocess`.
- Compiled to `service.exe`; callable as a CLI tool.
- Auto-calculates job priority from delivery date (days remaining; negative = overdue).

### 3. Flask Web Server (`server.py`)
- Routes: `/` (dashboard), `/job/add`, `/job/<id>/update`, `/jobs` (search & filter), `/user`, `/user/search`.
- `parse_jobs()` reads `jobs.txt` directly in Python and sorts by live priority for display.
- `run_exe()` bridges Flask to `service.exe` via subprocess with timeout and error handling.
- Jinja2 SSR templates — no client-side JSON fetching.

### 4. Frontend (Jinja2 + CSS)
- `base.html` / `index.html` — mechanic dashboard with stats cards, active jobs table, history table.
- `jobs.html` — unified add/update form with search and status filter.
- `user_base.html` / `user_search.html` / `user_view.html` — customer-facing portal with partial phone/reg search, 3-step progress bar, and a payment placeholder.

### 5. Data Structures & Algorithms (Current — `scrap-work` branch)
Integrated directly into `service.c` without touching any other file:

| Structure / Algorithm | Details |
|---|---|
| **Singly Linked List** | Per-vehicle chronological history (`HistoryNode`, `VehicleList`); rebuilt each run from `jobs.txt` top-to-bottom |
| **Stack** | Undo last add or update; stored as linked list nodes; persisted to `data/undo.txt` across subprocess calls |
| **Queue** | FIFO per `service_type`; non-completed jobs enqueued in file order; `next_job` CLI command dequeues |
| **Insertion Sort** | Sorts the full job array by priority before every `fh_write_all` call |
| **Linear Search** | Reusable across `reg_no`, `owner_name`, `engine_no`, `phone`; used internally and via `search` CLI |
| **Binary Search** | Fast job-by-ID lookup on a temporary ID-sorted copy of the jobs array; used in `jc_update` |

New CLI commands added: `undo`, `next_job <service_type>`, `search <field> <value>`.  
Existing `add` and `update` commands, `jobs.txt` format, and all Flask routes are unchanged.

---

## Current Branch Status

| Branch | Status |
|---|---|
| `main` | Stable baseline |
| `Oreo` | Customer portal feature, pushed |
| `scrap-work` | Data structure integration, pushed (2026-04-08) |

---

## Remaining / Next Steps

1. **Merge `scrap-work` into `main`** after review and testing.
2. **Integration testing** — full add → update → undo lifecycle via Flask UI (not just CLI).
3. **Form validation** — client-side or server-side guards on `/job/add` (empty fields, date format).
4. **Razorpay payment integration** — the customer portal has a placeholder button ready.
5. **Scalability review** — evaluate flat-file vs SQLite if concurrent users become a concern.
6. **Final UX polish** — responsive layout audit, mobile breakpoints, accessibility.
