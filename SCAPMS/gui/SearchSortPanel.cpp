#include "SearchSortPanel.h"
#include "GuiHelpers.h"

#include "../utils/Exceptions.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <algorithm>
#include <cctype>

namespace scapms {
namespace gui {

using controller::SearchEngine;
using controller::SearchKey;
using controller::SortAlgorithm;

// Search modes offered in the dropdown (index == mode).
enum SearchMode {
    ModeAuto = 0,
    ModeLinearById,
    ModeBinaryById,
    ModeBinaryByName,
    ModeRecursiveByName,
    ModeCategory
};

static bool looksLikeId(const std::string& s)
{
    if (s.size() < 2 || s[0] != 'P')
        return false;
    for (std::size_t i = 1; i < s.size(); ++i)
        if (!std::isdigit(static_cast<unsigned char>(s[i])))
            return false;
    return true;
}

SearchSortPanel::SearchSortPanel(controller::ClinicManager& manager,
                                 QWidget* parent)
    : QWidget(parent), manager_(manager)
{
    buildUi();
    refresh();
}

void SearchSortPanel::buildUi()
{
    // ================= search group =================
    searchEdit_ = new QLineEdit(this);
    searchEdit_->setPlaceholderText("Patient ID / name keyword / category");
    searchEdit_->setToolTip("Enter an ID like P0001, a name fragment, or "
                             "General / Emergency / Follow-Up.");

    modeCombo_ = new QComboBox(this);
    modeCombo_->addItems({
        "Auto (pick best for data state)",
        "Patient ID - Linear Search",
        "Patient ID - Binary Search (needs ID-sorted data)",
        "Patient Name - Binary Search (needs name-sorted data)",
        "Patient Name - Recursive Partial Match",
        "Category - Linear Filter"
    });
    modeCombo_->setToolTip("The system always uses the technique that fits\n"
                           "the current state of the data: binary search\n"
                           "only on sorted data, linear otherwise.");

    auto* searchButton = new QPushButton("Search", this);
    searchButton->setToolTip("Run the selected searching algorithm (Ctrl+F).");
    connect(searchButton, &QPushButton::clicked,
            this, &SearchSortPanel::onSearchClicked);
    connect(modeCombo_, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onModeChanged(int)));

    auto* searchForm = new QFormLayout;
    searchForm->addRow("Keyword:", searchEdit_);
    searchForm->addRow("Algorithm:", modeCombo_);

    auto* searchGroup = new QGroupBox("Search Patient Records", this);
    auto* searchGroupLayout = new QHBoxLayout(searchGroup);
    searchGroupLayout->addLayout(searchForm, 2);
    searchGroupLayout->addWidget(searchButton, 0, Qt::AlignTop);

    resultsInfo_ = new QLabel("No search executed yet.", this);

    results_ = new QTableWidget(this);
    results_->setColumnCount(6);
    QStringList headers;
    headers << "Patient ID" << "Name" << "Age" << "Contact"
            << "Category" << "Details";
    results_->setHorizontalHeaderLabels(headers);
    results_->setSelectionBehavior(QAbstractItemView::SelectRows);
    results_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    results_->verticalHeader()->setVisible(false);
    results_->horizontalHeader()->setStretchLastSection(true);

    // ================= sort group =================
    algoCombo_ = new QComboBox(this);
    algoCombo_->addItems({"Selection Sort", "Insertion Sort", "Merge Sort"});
    algoCombo_->setToolTip("All three algorithms are implemented manually\n"
                           "in SearchEngine - see the explanation below.");

    keyCombo_ = new QComboBox(this);
    keyCombo_->addItems({"Patient ID", "Patient Name", "Age",
                         "Appointment Date"});

    sortButton_ = new QPushButton("SORT", this);
    sortButton_->setToolTip("Reorder the whole patient collection and\n"
                            "refresh the Patient Records table.");
    connect(sortButton_, &QPushButton::clicked,
            this, &SearchSortPanel::onSortClicked);

    sortInfo_ = new QLabel("Data is currently UNSORTED.", this);
    sortInfo_->setWordWrap(true);

    algoExplanation_ = new QLabel(this);
    algoExplanation_->setWordWrap(true);
    algoExplanation_->setStyleSheet("QLabel { color: #336699; }");
    connect(algoCombo_, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onModeChanged(int)));
    onModeChanged(0); // fill the explanation immediately

    auto* sortForm = new QFormLayout;
    sortForm->addRow("Sort Algorithm:", algoCombo_);
    sortForm->addRow("Sort By:", keyCombo_);

    auto* sortGroup = new QGroupBox("Sort Records (manual algorithms)", this);
    auto* sortGroupLayout = new QHBoxLayout(sortGroup);
    sortGroupLayout->addLayout(sortForm, 2);
    sortGroupLayout->addWidget(sortButton_, 0, Qt::AlignTop);

    auto* sortText = new QVBoxLayout;
    sortText->addWidget(sortGroup);
    sortText->addWidget(sortInfo_);
    sortText->addWidget(algoExplanation_);

    // ================= overall vertical layout =================
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(8, 8, 8, 8);
    outer->addWidget(searchGroup);
    outer->addWidget(resultsInfo_);
    outer->addWidget(results_, 1);
    outer->addLayout(sortText);
}

void SearchSortPanel::focusSearch()
{
    searchEdit_->setFocus();
    searchEdit_->selectAll();
}

void SearchSortPanel::onModeChanged(int /*index*/)
{
    // Live explanation of the selected sorting algorithm
    // (assignment requirement 5 - justify the algorithm choice).
    static const char* explanations[] = {
        "Selection Sort - O(n^2) always, but performs the minimum number of "
        "swaps. Appropriate for this clinic because record objects are heavy "
        "(polymorphic unique_ptr) and the data set is small, so cheap swaps "
        "matter more than fewer comparisons.",
        "Insertion Sort - O(n^2) worst case but nearly O(n) when the data is "
        "already almost sorted, which is the normal situation after loading a "
        "sorted file and appending a few new patients. It is also stable.",
        "Merge Sort - guaranteed O(n log n) with extra memory. The right "
        "choice when the record collection grows large (thousands of "
        "patients) and the quadratic algorithms would slow the GUI down."
    };
    algoExplanation_->setText(
        toQt(std::string("Why this algorithm: ") +
             explanations[algoCombo_->currentIndex()]));
}

void SearchSortPanel::onSearchClicked()
{
    const std::string query = toStd(searchEdit_->text());
    if (query.empty()) {
        QMessageBox::warning(this, "Invalid Search Input",
                             "Please enter a search keyword first.");
        return;
    }

    try {
        const auto& data = manager_.database().patients();
        const int mode = modeCombo_->currentIndex();
        std::vector<int> hits;
        QString how;

        if (mode == ModeAuto) {
            // Choose the technique that fits the CURRENT STATE of the data.
            if (looksLikeId(query)) {
                if (manager_.isSortedBy(SearchKey::PatientId)) {
                    int i = SearchEngine::binarySearchByKey(
                        data, query, SearchKey::PatientId, manager_.database());
                    if (i >= 0) hits.push_back(i);
                    how = "AUTO: Binary Search (data is sorted by Patient ID)";
                } else {
                    int i = SearchEngine::linearSearchById(data, query);
                    if (i >= 0) hits.push_back(i);
                    how = "AUTO: Linear Search (data is unsorted - binary "
                          "search would be invalid)";
                }
            } else {
                if (manager_.isSortedBy(SearchKey::PatientName)) {
                    int i = SearchEngine::binarySearchByKey(
                        data, query, SearchKey::PatientName, manager_.database());
                    if (i >= 0) {
                        hits.push_back(i);
                        how = "AUTO: Binary Search on sorted names, "
                              "expanded to partial matches";
                    }
                }
                // Partial-match results are always included as well.
                std::vector<int> rec =
                    SearchEngine::recursiveNameSearch(data, query);
                for (int i : rec)
                    if (std::find(hits.begin(), hits.end(), i) == hits.end())
                        hits.push_back(i);
                if (hits.empty())
                    how = "AUTO: Recursive name search found nothing";
                else if (how.isEmpty())
                    how = "AUTO: Recursive name search (unsorted data)";
            }
        } else if (mode == ModeLinearById) {
            const int i = SearchEngine::linearSearchById(data, query);
            if (i >= 0) hits.push_back(i);
            how = "LINEAR SEARCH by Patient ID - works on any data (O(n))";
        } else if (mode == ModeBinaryById) {
            if (!manager_.isSortedBy(SearchKey::PatientId)) {
                QMessageBox::warning(
                    this, "Binary Search Unavailable",
                    "Binary search requires the records to be sorted by "
                    "Patient ID.\nUse the SORT box below first (or choose "
                    "the Auto mode).");
                return;
            }
            const int i = SearchEngine::binarySearchByKey(
                data, query, SearchKey::PatientId, manager_.database());
            if (i >= 0) hits.push_back(i);
            how = "BINARY SEARCH by Patient ID on sorted data (O(log n))";
        } else if (mode == ModeBinaryByName) {
            if (!manager_.isSortedBy(SearchKey::PatientName)) {
                QMessageBox::warning(
                    this, "Binary Search Unavailable",
                    "Binary search requires the records to be sorted by "
                    "Patient Name.\nSort first, or use the Auto mode.");
                return;
            }
            const int i = SearchEngine::binarySearchByKey(
                data, query, SearchKey::PatientName, manager_.database());
            if (i >= 0) hits.push_back(i);
            how = "BINARY SEARCH by Patient Name on sorted data (O(log n))";
        } else if (mode == ModeRecursiveByName) {
            hits = SearchEngine::recursiveNameSearch(data, query);
            how = "RECURSIVE partial-match name search (works unsorted)";
        } else if (mode == ModeCategory) {
            std::string lowerQuery = toStd(QString::fromStdString(query).toLower());
            for (std::size_t i = 0; i < data.size(); ++i) {
                std::string t = data[i]->getTypeName();
                if (toStd(QString::fromStdString(t).toLower()) == lowerQuery)
                    hits.push_back(static_cast<int>(i));
            }
            how = "LINEAR category filter";
        }

        showResults(hits, how);
    } catch (const utils::ClinicError& e) {
        QMessageBox::critical(this, "Search Error", toQt(e.what()));
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Unexpected Error", toQt(e.what()));
    }
}

void SearchSortPanel::showResults(const std::vector<int>& indices,
                                  const QString& how)
{
    results_->setRowCount(0);
    const auto& data = manager_.database().patients();

    for (int idx : indices) {
        if (idx < 0 || idx >= static_cast<int>(data.size()))
            continue;
        const model::PatientRecord& p = *data[idx];
        const int row = results_->rowCount();
        results_->insertRow(row);
        results_->setItem(row, 0, plainItem(toQt(p.getPatientId())));
        results_->setItem(row, 1, plainItem(toQt(p.getName())));
        results_->setItem(row, 2, plainItem(QString::number(p.getAge())));
        results_->setItem(row, 3, plainItem(toQt(p.getContact())));
        results_->setItem(row, 4, plainItem(toQt(p.getTypeName())));
        results_->setItem(row, 5, plainItem(toQt(patientDetails(&p))));
        paintRow(results_, row, colorForPatientType(p.getTypeName()));
    }
    results_->resizeColumnsToContents();

    resultsInfo_->setText(QString("%1 record(s) found  |  %2")
                              .arg(static_cast<int>(indices.size())).arg(how));
}

void SearchSortPanel::onSortClicked()
{
    try {
        const int algoIdx = algoCombo_->currentIndex();
        SortAlgorithm algo = SortAlgorithm::Selection;
        if (algoIdx == 1) algo = SortAlgorithm::Insertion;
        else if (algoIdx == 2) algo = SortAlgorithm::Merge;

        const int keyIdx = keyCombo_->currentIndex();
        SearchKey key = SearchKey::PatientId;
        if (keyIdx == 1) key = SearchKey::PatientName;
        else if (keyIdx == 2) key = SearchKey::Age;
        else if (keyIdx == 3) key = SearchKey::AppointmentDate;

        manager_.sortPatients(algo, key);
        sortInfo_->setText(toQt("Data is now SORTED. " +
                                manager_.lastSortDescription() +
                                "\nBinary search is available for this field."));
        refresh();
        emit dataChanged(); // let the other tabs show the new order
    } catch (const utils::ClinicError& e) {
        QMessageBox::critical(this, "Sort Failed", toQt(e.what()));
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Unexpected Error", toQt(e.what()));
    }
}

void SearchSortPanel::refresh()
{
    const auto& data = manager_.database().patients();
    results_->setRowCount(0);
    // The sort must be visible immediately: show the current vector order.
    std::vector<int> all;
    for (std::size_t i = 0; i < data.size(); ++i)
        all.push_back(static_cast<int>(i));
    showResults(all, "Current table order (sorted data - binary search ready)");
}

} // namespace gui
} // namespace scapms
