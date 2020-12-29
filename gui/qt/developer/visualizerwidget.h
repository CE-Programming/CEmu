#ifndef MEMORYVISUALIZER_H
#define MEMORYVISUALIZER_H

#include "widgets/visualizerlcdwidget.h"

#include <QtWidgets/QWidget>
QT_BEGIN_NAMESPACE
class QCloseEvent;
class QGroupBox;
class QLineEdit;
class QRadioButton;
class QToolButton;
QT_END_NAMESPACE

class VisualizerWidgetList
{
public:
    VisualizerWidgetList();
    virtual ~VisualizerWidgetList();

    VisualizerWidgetList *prev() const { return mPrev; }
    VisualizerWidgetList *next() const { return mNext; }
    bool empty() const { return mPrev == this; }

protected:
    VisualizerWidgetList(VisualizerWidgetList *list);
    VisualizerWidgetList *mPrev, *mNext;
};

class VisualizerWidget : public QWidget, public VisualizerWidgetList
{
    Q_OBJECT

public:
    explicit VisualizerWidget(const QString &config, VisualizerWidgetList *list, QWidget *parent = nullptr);
    QString getConfig();
    void translate();
    void forceUpdate();

private slots:
    void resetView();
    void showConfig();
    void showPresets();

signals:
    void configChanged();

private:
    void closeEvent(QCloseEvent *) override;

    void stringToView();
    void viewToString();

    QStringList mSetup;

    int mScale = 100;
    int mFps = 30;

    QLineEdit *mConfigStr;
    QGroupBox *mGroup;

    QToolButton *mBtnLcd;
    QToolButton *mBtnRefresh;
    QToolButton *mBtnConfig;

    VisualizerWidget *mPrev, *mNext;
    VisualizerLcdWidget *mLcd;
    VisualizerLcdWidgetConfig mLcdConfig;
};

#endif
