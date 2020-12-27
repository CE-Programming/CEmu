#include "dockwidget.h"

#include <kddockwidgets/FrameworkWidgetFactory.h>
#include <kddockwidgets/private/widgets/TitleBarWidget_p.h>
#include <kddockwidgets/private/widgets/FrameWidget_p.h>
#include <kddockwidgets/private/multisplitter/Separator_qwidget.h>

#include <QApplication>

class DockTitleBar : public KDDockWidgets::TitleBarWidget
{
public:
    explicit DockTitleBar(KDDockWidgets::Frame *frame)
        : KDDockWidgets::TitleBarWidget(frame),
          isEditable{true}
    {
        init();
    }

    explicit DockTitleBar(KDDockWidgets::FloatingWindow *fw)
        : KDDockWidgets::TitleBarWidget(fw),
          isEditable{true}
    {
        init();
    }

    void init()
    {
        if (!isEditable)
        {
            setContentsMargins(0, 0, 0, 0);
            setFixedHeight(0);
        }
    }

    void paintEvent(QPaintEvent *event) override
    {
        KDDockWidgets::TitleBarWidget::paintEvent(event);
    }

private:
    bool isEditable;
};

class DockSeparator : public Layouting::SeparatorWidget
{
public:
    explicit DockSeparator(Layouting::Widget *parent)
        : Layouting::SeparatorWidget(parent)
    {
        setContentsMargins(0, 0, 0, 0);
    }
};

class DockFrame : public KDDockWidgets::FrameWidget
{
public:
    explicit DockFrame(QWidget *parent, KDDockWidgets::FrameOptions options)
        : KDDockWidgets::FrameWidget(parent, options),
          isEditable{true}
    {
        setContentsMargins(0, 0, 0, 0);

        setAttribute(Qt::WA_NoSystemBackground, true);
        setAttribute(Qt::WA_TranslucentBackground, true);
    }

    void paintEvent(QPaintEvent *event) override
    {
        if (isEditable || isFloating())
        {
            KDDockWidgets::FrameWidget::paintEvent(event);
        }
    }

private:
    bool isEditable;
};

KDDockWidgets::Frame *DockWidgetFactory::createFrame(KDDockWidgets::QWidgetOrQuick *parent, KDDockWidgets::FrameOptions options) const
{
    return new DockFrame(parent, options);
}

KDDockWidgets::TitleBar *DockWidgetFactory::createTitleBar(KDDockWidgets::Frame *frame) const
{
    return new DockTitleBar(frame);
}

KDDockWidgets::TitleBar *DockWidgetFactory::createTitleBar(KDDockWidgets::FloatingWindow *fw) const
{
    return new DockTitleBar(fw);
}

Layouting::Separator *DockWidgetFactory::createSeparator(Layouting::Widget *parent) const
{
    return new DockSeparator(parent);
}

