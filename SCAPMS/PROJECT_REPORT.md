SCAPMS - Smart Clinic Appointment & Patient Management System
Project Design Document

============================================================
i. PROJECT DESCRIPTION
============================================================

SCAPMS (Smart Clinic Appointment & Patient Management System) is a desktop
application built with Qt 5 (C++) that manages patient registration,
appointment scheduling, a FIFO waiting queue, manual searching/sorting of
patient records, undoable administrative operations, and CSV-based
persistence.

The system is architected in three clean layers so business logic stays
independent of the GUI framework:

Layer        Directory      Responsibility
-------      ---------      --------------
Model        model/         Pure C++ data classes (patients, appointments,
                            database). No Qt dependency.
Controller   controller/    Business logic: ClinicManager,
                            AppointmentController, SearchEngine.
GUI          gui/           Qt Widgets tabs/panels that call controllers
                            only, never each other directly.

The entry point is main.cpp, which constructs a QApplication, shows
MainWindow, and runs the Qt event loop.

============================================================
ii. SYSTEM FEATURES
============================================================

1. Patient Registration (CRUD) - Add general, emergency, and follow-up
   patients with validation (name non-empty, age 0-120, contact non-empty).

2. Appointment Management - Schedule, cancel, and mark appointments
   complete; double-booking is rejected; emergency patients with high
   severity may refuse cancellation.

3. Waiting Queue (FIFO) - std::queue backed waiting list with enqueue,
   call-next, live waiting-time display, and over-10-minute alerts.

4. Undo Stack - std::stack of administrative operations (delete, modify,
   cancel appointment) with snapshot-based restoration.

5. Manual Searching - Linear search, binary search (sorted data only),
   recursive partial-match name search, category filter.

6. Manual Sorting - Selection sort, insertion sort, merge sort
   (no std::sort).

7. CSV Persistence - Save/load patients and appointments to/from
   patients.csv and appointments.csv with header versioning.

8. Clinic Dashboard & Reports - Live statistics, frequently-accessed
   patient cache (fixed array of 10), polymorphic clinic report,
   today's appointment reminders.

9. Event-Driven UI - Panels emit dataChanged() signals; MainWindow
   refreshes all tabs (no direct panel-to-panel coupling).

10. Qt-Free Business Logic - Model/controller layers compile and test
    without linking Qt (tests/logic_test.cpp).

============================================================
iii. CLASS HIERARCHY
============================================================

Model Layer:

Appointable (abstract interface)
  - scheduleAppointment(...)
  - cancelAppointment(...)

PatientRecord : public Appointable (abstract base)
  - patientId_, name_, age_, contact_
  - appointmentIds_ (composition: vector of linked appointment IDs)
  - getTypeName()       [pure virtual]
  - getPriority()       [pure virtual]
  - displayInfo(os)     [pure virtual]
  - serialize()         [pure virtual]
  - clone()             [pure virtual]
  - PatientPtr = unique_ptr<PatientRecord>

  - GeneralPatient : public PatientRecord
      - reasonForVisit_
      - priority = 1
      - cancels freely

  - EmergencyPatient : public PatientRecord
      - severityLevel_ (1-5)
      - description_
      - priority = 3 (+1 if severity >= 4)
      - refuses cancellation if severity > 2

  - FollowUpPatient : public PatientRecord
      - lastVisitDate_
      - reviewNote_
      - followUpCount_
      - priority = 2

Appointment
  - id_, patientId_, date_, time_, service_
  - status_ (Scheduled / Completed / Cancelled)
  - serialize() / deserialize() for CSV

ClinicDatabase
  - vector<PatientPtr> patients_   (ownership via unique_ptr)
  - vector<Appointment> appointments_

Controller Layer:

ClinicManager
  - ClinicDatabase db_
  - queue<string> waitQueue_              (FIFO)
  - map<string, long long> arrivalEpoch_  (enqueue timestamps)
  - stack<UndoOperation> undoStack_       (LIFO undo)
  - PatientRecord* recentPatients_[10]    (fixed array MRU cache)
  - int sortedByKey_                      (binary-search validity flag)
  - addPatient, editPatient, deletePatient, undoLast,
    saveFiles, loadFiles, ...

AppointmentController : uses ClinicManager
  - scheduleAppointment, cancelAppointment, completeAppointment
  - isValidDate (YYYY-MM-DD), isValidTime (HH:MM)
  - appointmentsOf / appointmentsOn queries

SearchEngine (static utility class)
  - linearSearchById
  - binarySearchByKey (requires pre-sorted data)
  - recursiveNameSearch
  - selectionSort / insertionSort / mergeSort
  - keyOf / keyName helpers

GUI Layer:

MainWindow (QMainWindow, owns tabs + controllers)
  - PatientPanel
  - AppointmentPanel
  - QueuePanel
  - SearchSortPanel
  - AdminPanel

GuiHelpers (inline helpers)
  - toQt / toStd conversions
  - colorForPatientType / colorForAppointmentStatus
  - plainItem / paintRow / patientDetails

============================================================
iv. DATA STRUCTURES USED
============================================================

1. std::vector<PatientPtr>       ClinicDatabase         - Patient collection;
                                                          indices act as table rows.
2. std::vector<Appointment>      ClinicDatabase         - Appointment store.
3. std::queue<string>            ClinicManager::waitQueue_   - FIFO waiting queue
                                                          (first come, first served).
4. std::stack<UndoOperation>     ClinicManager::undoStack_   - Undo stack; LIFO so
                                                          newest operation reverses first.
5. PatientRecord*[10]            ClinicManager::recentPatients_ - Fixed-size array
                                                          MRU cache of up to 10
                                                          recently accessed patients.
6. std::map<string, long long>   ClinicManager::arrivalEpoch_ - Enqueue timestamps
                                                          for computing waiting times.

============================================================
v. SEARCHING ALGORITHMS USED
============================================================

All implemented manually (no std::find / std::binary_search):

1. Linear Search by ID - SearchEngine::linearSearchById()
   Scans left to right; works on sorted or unsorted data. O(n).

2. Binary Search by Key - SearchEngine::binarySearchByKey()
   Halves the interval each step. O(log n).
   Pre-condition: data must already be sorted by the searched key; the GUI
   checks isSortedBy() before allowing it.

3. Recursive Name Search - SearchEngine::recursiveNameSearch()
   Recurses on index; collects every patient whose name contains the
   keyword (case-insensitive). Works on unsorted data.

4. Category Filter (linear scan) - SearchSortPanel::onSearchClicked()
   Mode ModeCategory. Compares getTypeName() against the query
   (case-insensitive).

============================================================
vi. SORTING ALGORITHMS USED
============================================================

All implemented manually (no std::sort):

Algorithm          File                         Complexity      Reason
--------           ----                         ----------      ------
Selection Sort     SearchEngine::selectionSort  O(n^2) always   Performs the
                                                          fewest swaps; good
                                                          when record objects are
                                                          heavy (unique_ptr)
                                                          and dataset is small.
Insertion Sort     SearchEngine::insertionSort  O(n^2) worst,   Stable; ideal
                                                          nearly O(n) when     after loading a sorted file and
                                                          nearly sorted          appending a few new patients.
Merge Sort         SearchEngine::mergeSort      O(n log n)      Best for large
                                                          guaranteed             collections where quadratic
                                                          algorithms would slow
                                                          the GUI.

============================================================
vii. RECURSIVE ALGORITHM USED
============================================================

SearchEngine::recursiveNameSearch() (controller/SearchEngine.cpp)

A helper recursiveNameSearchHelper() processes index i, checks if
data[i]->getName() contains the lower-cased keyword, then recurses to
i + 1. Terminates when index >= data.size(). Returns a vector<int> of
matching indices. Used by the GUI for partial-match name search that
works regardless of sort state.

============================================================
viii. GUI DESIGN
============================================================

Built with Qt 5 Widgets, using no fixed geometry - everything is
layout-driven:

- MainWindow - QMainWindow with a QTabWidget central widget
  (QVBoxLayout). Five tabs: Patient Records, Appointments, Waiting
  Queue, Search & Sort, Admin & Reports. Menu bar (File/Edit/View/Help),
  status bar with permanent stats label, and a 5-second QTimer for
  dashboard/reminder refresh (every 6th tick = 30s for reminders).

- Layouts used - QVBoxLayout, QHBoxLayout, QGridLayout, QFormLayout,
  QStackedWidget (for type-specific patient fields).

- QTableWidget - Used in Patient, Appointment, Queue, Search, and Cache
  tables with non-editable items, stretch last section, alternating
  colors, and custom row painting.

- Event-driven communication - Panels never talk directly; they emit
  dataChanged() and MainWindow::onDataChanged() calls refreshAll() on
  every panel. This satisfies the requirement for decoupled UI
  components.

- Visual encoding - Row background color encodes patient category
  (green=General, red=Emergency, blue=Follow-Up) and appointment status
  (yellow=Scheduled, green=Completed, grey=Cancelled) via GuiHelpers.

- Input validation - QSpinBox range 0-120 for age, QDateEdit/QTimeEdit
  for date/time, QComboBox for severity 1-5.

============================================================
ix. EXCEPTION HANDLING APPROACH
============================================================

A custom exception hierarchy derived from std::runtime_error, all under
scapms::utils:

ClinicError : public std::runtime_error
  - InvalidPatientData        (bad name/age/contact/severity)
  - DuplicateIdError          (ID already in database)
  - InvalidAppointmentData    (bad date/time/service/double-booking)
  - FileOperationError        (cannot open/read/write CSV)
  - InvalidSearchInput        (empty search keyword)
  - QueueEmptyError           (callNext on empty queue)

Handling pattern:
- Model/Controller throw exceptions on invalid input or failed operations.
- GUI catches utils::ClinicError (and std::exception as fallback) and
  shows QMessageBox::critical / warning / information with e.what().
- Callers never let exceptions propagate to the Qt event loop uncaught.

============================================================
x. ALGORITHMS CHOSEN AND REASONS
============================================================

| Algorithm             | Choice              | Justification |
|-----------------------|---------------------|---------------|
| Search (unsorted)     | Linear              | Simple, correct on any data state; clinic datasets are small. |
| Search (sorted)       | Binary              | O(log n); only enabled when isSortedBy() is true, preventing     |
|                       |                     | undefined behavior on unsorted data. |
| Partial-name search   | Recursive           | Demonstrates recursion; works unsorted; returns all matches. |
| Sort (small/few swaps)| Selection Sort      | Minimal swaps matter when moving heavy unique_ptr objects. |
| Sort (nearly-sorted)  | Insertion Sort      | Nearly O(n) on data loaded sorted + a few new records; stable. |
| Sort (large data)     | Merge Sort          | Guaranteed O(n log n); scales to thousands of patients. |
| Waiting line          | std::queue          | Natural FIFO semantics for triage/call-next. |
| Undo                  | std::stack          | LIFO matches reverse-the-most-recent-operation semantics. |
| Recent cache          | Fixed array [10]    | Bounded MRU cache with O(1) access; shift-on-touch keeps most      |
|                       |                     | recent at index 0. |

============================================================
xi. CHALLENGES ENCOUNTERED
============================================================

During code review and testing, two real bugs were identified:

1. FollowUpPatient::followUpCount_ was lost on save/load.
   ClinicDatabase::serialize() wrote the 8th CSV field for FOLLOWUP
   patients, but ClinicManager::loadFiles() never read f[7] to restore
   followUpCount_. Reloaded data reset every follow-up patient's count
   to 0.

2. Delete-patient undo broke appointment links.
   ClinicManager::deletePatient() pushed CancelAppointment undo entries
   after DeletePatient on the stack. On undo, DeletePatient was popped
   first and restored the patient, but the CancelAppointment entries
   beneath it tried to re-link appointments to a patient that did not
   yet exist, so findPatientRaw() returned null and the patient-to-
   appointment association was never re-established.

Additionally, deletePatient() used the raw pointer p after
db_.removePatient(patientId), making p a dangling pointer when
constructing the undo snapshot (p->getName(), p->clone()).

============================================================
xii. HOW THE CHALLENGES WERE RESOLVED
============================================================

1. followUpCount persistence - Added setFollowUpCount(int) to
   FollowUpPatient (model/FollowUpPatient.h). In
   ClinicManager::loadFiles(), after constructing a FOLLOWUP patient,
   if f.size() > 7, the 8th field is parsed with std::stoul and passed
   to setFollowUpCount() (wrapped in try/catch so a malformed count
   does not abort the whole load).

2. Delete-patient undo ordering - Rewrote ClinicManager::deletePatient():
   - Capture patientName and patientSnapshot = p->clone() BEFORE
     db_.removePatient() (avoiding the dangling pointer).
   - Push CancelAppointment undo entries FIRST, then push
     DeletePatient LAST.
   - Because the stack is LIFO, undoLast() pops DeletePatient first
     (restoring the patient), then each CancelAppointment entry
     re-links its appointment to the now-restored patient via
     p->addAppointmentId().

3. Verification - Rebuilt with g++ -Wall -Wextra -Wpedantic
   -Wconversion (zero warnings) and re-ran tests/logic_test.cpp; all
   assertions pass, including the delete+undo round-trip and persistence
   round-trip with followUpCount intact.

============================================================
BUILD & RUN
============================================================

cd /home/wizzol/Desktop/SCAPMS/build
qmake ../SCAPMS.pro   (or just make if Makefile exists)
make
./SCAPMS              (GUI application)
./logic_test          (headless model/controller tests)

The app auto-loads data/patients.csv and data/appointments.csv on
startup if present.
