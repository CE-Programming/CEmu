#ifndef MEMORYVISUALIZERWIDGET_H
#define MEMORYVISUALIZERWIDGET_H

#include <QtWidgets/QWidget>
#include <QtCore/QTimer>

class MemoryVisualizerWidget : public QWidget {
  Q_OBJECT

public:
    explicit MemoryVisualizerWidget(QWidget *p = Q_NULLPTR);
    ~MemoryVisualizerWidget();
    void setRefreshRate(int rate);
    void setConfig(uint32_t h, uint32_t w, uint32_t u, uint32_t c, uint32_t *d, uint32_t *e);

protected:
    virtual void paintEvent(QPaintEvent*) Q_DECL_OVERRIDE;

private slots:
    void draw();

private:
    QTimer *refreshTimer;
    QImage image;
    int refresh;

    // configuration
    uint32_t height;
    uint32_t width;
    uint32_t upbase;
    uint32_t control;
    uint32_t size;
    uint32_t *data;
    uint32_t *data_end;
};

#endif
