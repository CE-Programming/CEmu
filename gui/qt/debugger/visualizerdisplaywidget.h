#ifndef MEMORYVISUALIZERWIDGET_H
#define MEMORYVISUALIZERWIDGET_H

#include <QtCore/QTimer>
#include <QtWidgets/QWidget>
#include <QtGui/QClipboard>

class VisualizerDisplayWidget : public QWidget {
  Q_OBJECT

public:
    explicit VisualizerDisplayWidget(QWidget *p = Q_NULLPTR);
    ~VisualizerDisplayWidget();
    void setRefreshRate(int rate);
    void setConfig(float bppstep, uint32_t h, uint32_t w, uint32_t u, uint32_t c, uint32_t *d, uint32_t *e);

protected:
    virtual void paintEvent(QPaintEvent*) Q_DECL_OVERRIDE;

private slots:
    void draw();
    void contextMenu(const QPoint &posa);

private:
    QTimer *m_refreshTimer;
    QImage m_image;
    int m_refresh;

    // configuration
    uint32_t m_height;
    uint32_t m_size;
    uint32_t m_width;
    uint32_t m_upbase;
    uint32_t m_control;
    float m_bppstep;
    uint32_t *m_data;
    uint32_t *m_data_end;
};

#endif
