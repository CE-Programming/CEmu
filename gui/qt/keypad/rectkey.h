#ifndef RECTKEY_H
#define RECTKEY_H

#include "key.h"
#include "keyconfig.h"

#include <QtGui/QFont>

class RectKey : public Key {
public:
    RectKey(KeyConfig &config,
            const QRect &textGeometry, const QRect &keyGeometry, const QSize &labelSize,
            const QColor &keyColor, const QColor &textColor,
            const QString &labelText, const QString &secondText, const QString &alphaText,
            Qt::Alignment labelAlign  = Qt::AlignCenter,
            Qt::Alignment secondAlign = Qt::AlignVCenter | Qt::AlignLeft,
            Qt::Alignment alphaAlign  = Qt::AlignVCenter | Qt::AlignRight)
        : RectKey{config.next(), textGeometry, keyGeometry, labelSize,
                  keyColor, textColor, config.secondColor, config.alphaColor,
                  labelText, secondText, alphaText, config.labelFont, config.secondFont, config.alphaFont,
                  labelAlign, secondAlign, alphaAlign} { }
    RectKey(KeyCode keycode,
            const QRect &textGeometry, const QRect &keyGeometry, const QSize &labelSize,
            const QColor &keyColor, const QColor &textColor, const QColor &secondColor, const QColor &alphaColor,
            const QString &labelText, const QString &secondText, const QString &alphaText,
            const QFont &labelFont, const QFont &secondFont, const QFont &alphaFont,
            Qt::Alignment labelAlign  = Qt::AlignCenter,
            Qt::Alignment secondAlign = Qt::AlignVCenter | Qt::AlignLeft,
            Qt::Alignment alphaAlign  = Qt::AlignVCenter | Qt::AlignRight)
        : RectKey{keycode, textGeometry, keyGeometry, labelSize, 4, 4, 4, 4,
                  keyColor, textColor, secondColor, alphaColor, labelText, secondText, alphaText,
                  labelFont, secondFont, alphaFont, labelAlign, secondAlign, alphaAlign} { }
    RectKey(KeyConfig &config,
            const QRect &textGeometry, const QRect &keyGeometry, const QSize &labelSize,
            int topLeft, int topRight, int bottomLeft, int bottomRight,
            const QColor &keyColor, const QColor &textColor,
            const QString &labelText, const QString &secondText, const QString &alphaText,
            Qt::Alignment labelAlign  = Qt::AlignCenter,
            Qt::Alignment secondAlign = Qt::AlignVCenter | Qt::AlignLeft,
            Qt::Alignment alphaAlign  = Qt::AlignVCenter | Qt::AlignRight)
        : RectKey{config.next(), textGeometry, keyGeometry, labelSize,
                  topLeft, topRight, bottomLeft, bottomRight,
                 keyColor, textColor, config.secondColor, config.alphaColor,
                  labelText, secondText, alphaText, config.labelFont, config.secondFont, config.alphaFont,
                  labelAlign, secondAlign, alphaAlign} { }
    RectKey(KeyCode keycode,
            const QRect &textGeometry, const QRect &keyGeometry, const QSize &labelSize,
            int topLeft, int topRight, int bottomLeft, int bottomRight,
            const QColor &keyColor, const QColor &textColor, const QColor &secondColor, const QColor &alphaColor,
            const QString &labelText, const QString &secondText, const QString &alphaText,
            const QFont &labelFont, const QFont &secondFont, const QFont &alphaFont,
            Qt::Alignment labelAlign  = Qt::AlignCenter,
            Qt::Alignment secondAlign = Qt::AlignVCenter | Qt::AlignLeft,
            Qt::Alignment alphaAlign  = Qt::AlignVCenter | Qt::AlignRight);

    virtual void paint(QPainter &) const;

protected:
    virtual bool canAccept(const QPointF &);

    QColor m_textColor, m_secondColor, m_alphaColor;
    Qt::Alignment m_labelAlign, m_secondAlign, m_alphaAlign;
    QFont m_labelFont, m_secondFont, m_alphaFont;
    const QString m_labelText, m_secondText, m_alphaText;
};

#endif
