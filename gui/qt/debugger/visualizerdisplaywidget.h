#ifndef MEMORYVISUALIZERWIDGET_H
#define MEMORYVISUALIZERWIDGET_H

#include <QtCore/QTimer>
#include <QtCore/QVector>
#include <QtWidgets/QWidget>
#include <QtGui/QClipboard>

enum class VisualizerTransform {
    Rotate90,
    Rotate180,
    Rotate270,
    FlipHorizontal,
    FlipVertical,
    Transpose,
    Transverse,
};

class VisualizerDisplayWidget : public QWidget {
  Q_OBJECT

public:
    explicit VisualizerDisplayWidget(QWidget *p = Q_NULLPTR);
    ~VisualizerDisplayWidget();
    void setRefreshRate(int rate);
    void setConfig(uint32_t bppstep, int w, int h, uint32_t u, uint32_t c, bool g,
                   const QVector<VisualizerTransform> &transforms, uint32_t *d, uint32_t *e);

protected:
    virtual void paintEvent(QPaintEvent*) Q_DECL_OVERRIDE;
    virtual void mousePressEvent(QMouseEvent*) Q_DECL_OVERRIDE;

private slots:
    void draw();
    void contextMenu(const QPoint &posa) const;

private:
    void updateDisplayImage();

    QTimer *m_refreshTimer;
    QImage *m_image;
    QImage m_displayImage;
    int m_refresh;

    // configuration
    int m_height;
    int m_size;
    int m_width;
    bool m_grid;
    QVector<VisualizerTransform> m_transforms;
    uint32_t m_upbase;
    uint32_t m_control;
    uint32_t m_bppstep;
    uint32_t *m_data;
    uint32_t *m_data_end;
};

#endif
