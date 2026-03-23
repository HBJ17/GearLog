## JOB CARD MANAGEMENT SYSTEM - WEEK 1 PROGRESS - (18.3.26 - 25.3.26)

1. PROJECT OVERVIEW
   This project is a simple Job Card Management System developed using Flask (Python) and C programming.
   It allows users to add and view vehicle job card details through a web interface.

The system combines:

* Frontend (HTML)
* Backend (Flask)
* System-level file handling (C programs)

---

2. FEATURES IMPLEMENTED

• Add Job Card
Users can enter:

* Registration Number
* Vehicle Make
* Model
* Owner Name

• Store Data
Data is passed from Flask to a C program which stores it in a binary file (vehicles.dat).

• View Job Cards
Another C program reads the stored data and displays all job cards.

---

3. TECHNOLOGIES USED

* Python (Flask Framework)
* C Programming
* HTML (Frontend)
* Binary File Handling

---

4. PROJECT STRUCTURE

* app.py → Flask application
* templates/

  * index.html
  * add.html
* file_handler.c → Writes data to file
* read_b.c → Reads data from file
* file_handler.exe → Compiled writer program
* read_b.exe → Compiled reader program
* vehicles.dat → Binary data file

---

5. HOW THE SYSTEM WORKS

Step 1: User opens the web app
Step 2: User fills the job card form
Step 3: Flask collects form data
Step 4: Flask calls file_handler.exe
Step 5: C program writes data into vehicles.dat
Step 6: User clicks "View Jobcards"
Step 7: Flask calls read_b.exe
Step 8: Data is read and displayed in browser

---

6. HOW TO RUN THE PROJECT

Step 1: Compile C files
gcc file_handler.c -o file_handler.exe
gcc read_b.c -o read_b.exe

Step 2: Install Flask
pip install flask

Step 3: Run Flask app
python app.py

Step 4: Open browser
http://127.0.0.1:5000

---

7. IMPORTANT NOTES

* vehicles.dat is a binary file storing structured records
* Each record is of type "struct jobcard"
* Data is appended using "ab" mode
* Data is read using "rb" mode

---

8. LIMITATIONS

* No input validation
* No error handling in Flask
* Output is plain text (not formatted HTML)
* Uses fixed-size character arrays
* Works only on systems supporting .exe

---

9. FUTURE IMPROVEMENTS

* Add input validation
* Improve UI design
* Convert output to HTML format
* Add edit and delete features
* Replace file system with database (SQLite/MySQL)

---

10. CONCLUSION

This project demonstrates integration between Python and C, along with file handling using binary files.
It successfully implements a basic full-stack system with cross-language communication.

---

