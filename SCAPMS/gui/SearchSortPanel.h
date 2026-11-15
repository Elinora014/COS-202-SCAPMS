#ifndef SCAPMS_GUI_SEARCHSORTPANEL_H
#define SCAPMS_GUI_SEARCHSORTPANEL_H

#include <QWidget>

#include "../controller/ClinicManager.h"

class QTableWidget;
class QLineEdit;
class QComboBox;
class QPushButton;
class QLabel;

namespace scapms {
namespace gui {

/**
 * SearchSortPanel - manual searching and sorting operations.
 * QFormLayout inside QGroupBox sections, QVBoxLayout overall.
 */
class SearchSortPanel : public QWidget {
    Q_OBJECT
public:
    explicit SearchSortPanel(controller::ClinicManager& manager,
                             QWidget* parent = nullptr);

    void refresh();
    void focusSearch(); // Ctrl+F shortcut entry point

signals:
    void dataChanged();

private slots:
    void onSearchClicked();
    void onSortClicked();
    void onModeChanged(int index);

private:
    void buildUi();
    void showResults(const std::vector<int>& indices, const QString& how);

    controller::ClinicManager& manager_;

    QLineEdit* searchEdit_ = nullptr;
    QComboBox* modeCombo_  = nullptr;
    QTableWidget* results_ = nullptr;
    QLabel* resultsInfo_   = nullptr;

    QComboBox* algoCombo_ = nullptr;
    QComboBox* keyCombo_  = nullptr;
    QPushButton* sortButton_ = nullptr;
    QLabel* sortInfo_ = nullptr;
    QLabel* algoExplanation_ = nullptr;
};

} // namespace gui
} // namespace scapms

#endif // SCAPMS_GUI_SEARCHSORTPANEL_H
