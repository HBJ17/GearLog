# GearLog - Two-Wheeler Service System

GearLog is a highly efficient two-wheeler service center management system designed to streamline day-to-day operations, tracking, and customer communication. The project utilizes a unique hybrid architecture combining a lightweight Python Flask web server with a high-performance C backend executable.

## Features
- **Dashboard & Analytics:** Real-time metrics for total, pending, in-progress, completed, and overdue jobs.
- **Job Management:** Add new vehicles for service, record owner details, engine number, and service types.
- **Service Tracking:** Easily track vehicle service status with priorities computed dynamically based on the expected delivery date.
- **Server-Side Rendering (SSR):** Fast, template-driven frontend utilizing Jinja2, removing heavy client-side overheads.
- **Robust Storage:** Data is efficiently maintained through flat-file storage handled directly by the compiled C core program.

## Architecture
- **Frontend**: Developed with HTML, CSS, and Jinja2 templating engine for seamless integration with Flask.
- **Web Server Backend**: Python Flask routing user requests, rendering HTML templates, and passing operational commands into the executable backend.
- **Core Processing Engine**: Built in C (`service.c`), it executes I/O operations, ensuring fast and safe read/write processes. Compiled into `service.exe`.
- **Database**: Flat-file architecture (`data/jobs.txt`), parsed and interpreted safely via the backend algorithms.

## Folder Structure
```
├── backend/            # C backend codebase and executable
│   ├── service.c
│   └── service.exe
├── data/               # File-based database
│   └── jobs.txt
├── files/              # Contains all the core source code folders
├── frontend/           # CSS and Jinja2 Templates
│   ├── style.css
│   └── templates/
│       ├── index.html
│       └── jobs.html
└── server/             # Python Flask web server
    └── server.py
```

## How to Run
1. Ensure you have **Python 3.x** and a **GCC compiler** installed.
2. Install necessary libraries from Python pip:
   ```bash
   pip install flask flask-cors
   ```
3. If necessary, recompile the C backend executable:
   ```bash
   cd files/backend
   gcc service.c -o service.exe
   ```
4. Run the Flask server:
   ```bash
   cd files/server
   python server.py
   ```
5. Navigate to `http://localhost:5000` in your web browser to access the platform.





