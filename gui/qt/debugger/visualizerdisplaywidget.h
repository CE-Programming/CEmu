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
    void setConfig(uint32_t bppstep, int w, int h, uint32_t u, uint32_t c, bool g, uint32_t *d, uint32_t *e);

protected:
    virtual void paintEvent(QPaintEvent*) Q_DECL_OVERRIDE;
    virtual void mousePressEvent(QMouseEvent*) Q_DECL_OVERRIDE;

private slots:
    void draw();
    void contextMenu(const QPoint &posa);

private:
    QTimer *m_refreshTimer;
    QImage *m_image;
    int m_refresh;

    // configuration
    int m_height;
    int m_size;
    int m_width;
    bool m_grid;
    uint32_t m_upbase;
    uint32_t m_control;
    uint32_t m_bppstep;
    uint32_t *m_data;
    uint32_t *m_data_end;
};

#endif
