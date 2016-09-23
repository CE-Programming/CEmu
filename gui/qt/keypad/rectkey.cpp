#include "rectkey.h"

RectKey::RectKey(KeyCode keycode, const QRect &textGeometry, const QRect &keyGeometry,
                 const QSize &labelSize,
                 int topLeft, int topRight, int bottomLeft, int bottomRight,
                 const QColor &keyColor, const QColor &textColor, const QColor &secondColor, const QColor &alphaColor,
                 const QString &labelText, const QString &secondText, const QString &alphaText,
                 const QFont &labelFont, const QFont &secondFont, const QFont &alphaFont,
                 Qt::Alignment labelAlign, Qt::Alignment secondAlign, Qt::Alignment alphaAlign)
    : Key{keycode, textGeometry, keyGeometry, keyColor},
      m_textColor{textColor}, m_secondColor{secondColor}, m_alphaColor{alphaColor},
      m_labelAlign{labelAlign}, m_secondAlign{secondAlign}, m_alphaAlign{alphaAlign},
      m_labelFont{labelFont}, m_secondFont{secondFont.resolve(labelFont)},
                              m_alphaFont{alphaFont.resolve(labelFont)},
      m_labelText{labelText}, m_secondText{secondText}, m_alphaText{alphaText} {
    QRect corner;
    m_keyShape.moveTo(keyGeometry.topLeft() + QPointF{0, topLeft * .5});
    corner.setSize({bottomLeft, bottomLeft});
    corner.moveBottomLeft(keyGeometry.bottomLeft());
    m_keyShape.arcTo(corner, 90 * 2, 90);
    corner.setSize({bottomRight, bottomRight});
    corner.moveBottomRight(keyGeometry.bottomRight());
    m_keyShape.arcTo(corner, 90 * 3, 90);
    corner.setSize({topRight, topRight});
    corner.moveTopRight(keyGeometry.topRight());
    m_keyShape.arcTo(corner, 90 * 0, 90);
    corner.setSize({topLeft, topLeft});
    corner.moveTopLeft(keyGeometry.topLeft());
    m_keyShape.arcTo(corner, 90 * 1, 90);

#ifdef _WIN32
    m_labelFont.setPixelSize(labelSize.height());
    m_labelFont.setStretch(1+(labelSize.width() /
                           QFontMetricsF(m_labelFont).size(Qt::TextSingleLine, m_labelText).width()));
#else
    m_labelFont.setPixelSize(labelSize.height());
    m_labelFont.setStretch(labelSize.width() * m_labelFont.stretch() /
                           QFontMetricsF(m_labelFont).size(Qt::TextSingleLine, m_labelText).width());
#endif
}

void RectKey::paint(QPainter &painter) const {
    Key::paint(painter);
    painter.setFont(m_labelFont);
    painter.setPen(m_textColor);
    painter.drawText(keyGeometry(), m_labelAlign, m_labelText);
    if (!m_secondText.isEmpty()) {
        painter.setPen(m_secondColor);
        painter.setFont(m_secondFont);
        painter.drawText(textGeometry(), m_secondAlign, m_secondText);
    }
    if (!m_alphaText.isEmpty()) {
        painter.setPen(m_alphaColor);
        painter.setFont(m_alphaFont);
        painter.drawText(textGeometry(), m_alphaAlign, m_alphaText);
    }
}

bool RectKey::canAccept(const QPointF &point) {
    return m_keyShape.contains(point);
}
