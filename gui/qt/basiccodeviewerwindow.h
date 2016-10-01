#ifndef BASICCODEVIEWERWINDOW_H
#define BASICCODEVIEWERWINDOW_H

#include <QtWidgets/QDialog>
#include <QtCore/QString>
#include "tivarslib/autoloader.h"

namespace Ui { class BasicCodeViewerWindow; }

class BasicCodeViewerWindow : public QDialog {
    Q_OBJECT

public:
    explicit BasicCodeViewerWindow(QWidget *p = Q_NULLPTR);
    void setVariableName(const QString& name);
    void setOriginalCode(const QString& code) {
        originalCode = code;
        formattedCode = QString::fromStdString(tivars::TH_0x05::reindentCodeString(originalCode.toStdString()));
        showCode();
    }
    ~BasicCodeViewerWindow();

private slots:
    void on_pushButton_clicked();

private:
    void showCode();

    Ui::BasicCodeViewerWindow *ui;
    QString variableName;
    QString originalCode;
    QString formattedCode;
    bool showingFormatted = false;
};

#endif // BASICCODEVIEWERWINDOW_H
