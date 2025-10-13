#ifndef GOTODIALOG_H
#define GOTODIALOG_H

#include <QtWidgets/QDialog>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <vector>

QT_BEGIN_NAMESPACE
class QComboBox;
QT_END_NAMESPACE

class GotoDialog : public QDialog {
    Q_OBJECT
public:
    explicit GotoDialog(const QString &seed,
                        const std::vector<QString> &history,
                        const QStringList &completions = {},
                        QWidget *parent = nullptr);

    [[nodiscard]] QString text() const;

private:
    QComboBox *m_combo = nullptr;
};

#endif // GOTODIALOG_H
