// Throwaway headless test for the SCAPMS model/controller layers.
// The model/controller/utils layers are Qt-free by design, so this test
// compiles against the plain C++ objects without pulling in the GUI:
//
//   mkdir -p build && cd build && qmake ../SCAPMS.pro && make
//   g++ -std=c++17 -I.. ../tests/logic_test.cpp \
//       model*.o controller*.o utils*.o -o /tmp/logic_test   (see run script)
//
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <sstream>

#include "controller/ClinicManager.h"
#include "controller/AppointmentController.h"
#include "utils/Exceptions.h"

using namespace scapms::controller;
using namespace scapms::model;
using namespace scapms::utils;

int main()
{
    ClinicManager mgr;
    AppointmentController appt(mgr);

    // 1) add patients + validation exception
    std::string id1 = mgr.addPatient("GENERAL", "Zara Ali", 30, "c1");
    std::string id2 = mgr.addPatient("EMERGENCY", "Ben Camp", 40, "c2", "5", "fracture");
    std::string id3 = mgr.addPatient("FOLLOWUP", "Amy Call", 50, "c3", "2026-01-01", "note");
    std::cout << "IDs: " << id1 << " " << id2 << " " << id3 << "\n";
    assert(id1 == "P0001" && id3 == "P0003");

    bool threw = false;
    try { mgr.addPatient("GENERAL", "Bad Age", 200, "x"); }
    catch (const InvalidPatientData& e) { threw = true; std::cout << "EX: " << e.what() << "\n"; }
    assert(threw);

    // 2) polymorphic processing
    std::ostringstream os;
    mgr.processAllPatients(os);
    std::cout << os.str();
    assert(os.str().find("[EMERGENCY]") != std::string::npos);

    // 3) sorting + binary search
    mgr.sortPatients(SortAlgorithm::Merge, SearchKey::PatientName);
    assert(mgr.isSortedBy(SearchKey::PatientName));
    int idx = SearchEngine::binarySearchByKey(mgr.database().patients(), "Amy Call",
                                              SearchKey::PatientName, mgr.database());
    std::cout << "binary index of Amy = " << idx << "\n";
    assert(mgr.database().patients()[idx]->getName() == "Amy Call");

    idx = SearchEngine::linearSearchById(mgr.database().patients(), id2);
    assert(mgr.database().patients()[idx]->getPatientId() == id2);

    auto hits = SearchEngine::recursiveNameSearch(mgr.database().patients(), "a");
    std::cout << "recursive 'a' hits = " << hits.size() << "\n";
    assert(hits.size() == 3);

    // merge-by-name order: Amy, Ben, Zara
    assert(mgr.database().patients()[0]->getName() == "Amy Call");
    assert(mgr.database().patients()[2]->getName() == "Zara Ali");

    // selection & insertion produce consistent orders
    mgr.sortPatients(SortAlgorithm::Selection, SearchKey::PatientId);
    assert(mgr.database().patients()[0]->getPatientId() == "P0001");
    mgr.sortPatients(SortAlgorithm::Insertion, SearchKey::Age);
    assert(mgr.database().patients()[0]->getAge() == 30);

    // sorted state is invalidated after a mutation
    mgr.addPatient("GENERAL", "New One", 20, "c4");
    assert(!mgr.isSortedBy(SearchKey::Age));

    // 4) queue (FIFO)
    mgr.enqueuePatient(id1);
    mgr.enqueuePatient(id3);
    threw = false;
    try { mgr.enqueuePatient(id1); } catch (const ClinicError&) { threw = true; }
    assert(threw); // duplicate rejected
    assert(mgr.queueSnapshot().size() == 2);
    assert(mgr.callNextPatient() == id1); // FIFO
    assert(mgr.callNextPatient() == id3);
    threw = false;
    try { mgr.callNextPatient(); } catch (const QueueEmptyError&) { threw = true; }
    assert(threw);

    // 5) appointments via Appointable + cancel semantics
    std::string a1 = appt.scheduleAppointment(id1, "2026-12-01", "09:00", "Check-up");
    assert(a1 == "A0001");
    threw = false;
    try { appt.scheduleAppointment(id2, "2026-12-01", "09:00", "Clash"); }
    catch (const InvalidAppointmentData& e) { threw = true; std::cout << "EX: " << e.what() << "\n"; }
    assert(threw); // double booking

    appt.cancelAppointment(a1);
    assert(mgr.database().findAppointment(a1)->getStatus() == AppointmentStatus::Cancelled);

    // emergency patient with severity 5 refuses cancellation
    std::string a2 = appt.scheduleAppointment(id2, "2026-12-02", "10:00", "Surgery");
    threw = false;
    try { appt.cancelAppointment(a2); }
    catch (const ClinicError& e) { std::cout << "EX: " << e.what() << "\n"; threw = true; }
    assert(threw);

    // undo the successful cancellation of a1
    std::string undone = mgr.undoLast();
    std::cout << "undone: " << undone << "\n";
    assert(mgr.database().findAppointment(a1)->getStatus() == AppointmentStatus::Scheduled);

    // 6) delete patient + undo
    size_t before = mgr.database().patientCount();
    mgr.deletePatient(id3);
    assert(mgr.database().patientCount() == before - 1);
    mgr.undoLast(); // restores the patient record
    assert(mgr.database().patientCount() == before);

    // 7) edit + undo
    mgr.editPatient(id1, "Zara Ali", 31, "c1", "updated reason", "");
    assert(mgr.database().findPatientRaw(id1)->getAge() == 31);
    mgr.undoLast();
    assert(mgr.database().findPatientRaw(id1)->getAge() == 30);

    // 8) fixed-array cache (MRU)
    mgr.getPatient(id1); mgr.getPatient(id2);
    auto rec = mgr.recentRecords();
    std::cout << "cache size = " << rec.size() << "\n";
    assert(!rec.empty() && rec[0]->getPatientId() == id2);

    // 9) persistence round trip
    std::system("mkdir -p testoutput");
    mgr.saveFiles("testoutput");
    ClinicManager mgr2;
    auto res = mgr2.loadFiles("testoutput");
    std::cout << "loaded patients=" << res.patientCount
              << " appointments=" << res.appointmentCount
              << " warnings=" << res.warnings.size() << "\n";
    assert(res.patientCount == (int)mgr.database().patientCount());
    assert(res.warnings.empty());
    // IDs generated after a load must not collide with loaded records
    std::string nid = mgr2.addPatient("GENERAL", "X Y", 5, "z");
    assert(nid != "P0001");
    assert(mgr2.database().findPatientRaw(nid) != nullptr);

    // 10) statistics
    auto s = mgr.computeStatistics();
    std::cout << "stats: total=" << s.totalPatients
              << " gen=" << s.generalPatients
              << " emerg=" << s.emergencyPatients
              << " follow=" << s.followUpPatients
              << " appts=" << s.totalAppointments << "\n";
    assert(s.totalPatients == (int)mgr.database().patientCount());

    std::cout << "ALL LOGIC TESTS PASSED\n";
    return 0;
}
