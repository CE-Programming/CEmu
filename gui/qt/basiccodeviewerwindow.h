#ifndef BASICCODEVIEWERWINDOW_H
#define BASICCODEVIEWERWINDOW_H

#include "tivarslib/TypeHandlers/TypeHandlers.h"

#include <QtCore/QString>
#include <QtWidgets/QDialog>

namespace Ui { class BasicCodeViewerWindow; }

class BasicCodeViewerWindow : public QDialog {
    Q_OBJECT

public:
    explicit BasicCodeViewerWindow(QWidget *p = Q_NULLPTR);
    void setVariableName(const QString &name);
    void setOriginalCode(const QString &code) {
        m_originalCode = code;
        m_formattedCode = QString::fromStdString(tivars::TH_0x05::reindentCodeString(m_originalCode.toStdString()));
        showCode();
    }
    ~BasicCodeViewerWindow();

private slots:
    void toggleFormat();

private:
    void showCode();

    Ui::BasicCodeViewerWindow *ui;
    QString m_variableName;
    QString m_originalCode;
    QString m_formattedCode;
    bool m_showingFormatted = false;
};

#endif
