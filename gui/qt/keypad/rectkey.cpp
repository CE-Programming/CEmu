#include "rectkey.h"

RectKey::RectKey(KeyCode keycode, const QRect &textGeometry, const QRect &keyGeometry,
                 const QSize &labelSize,
                 int topLeft, int topRight, int bottomLeft, int bottomRight,
                 const QColor &keyColor, const QColor &textColor, const QColor &secondColor, const QColor &alphaColor,
                 const QString &labelText, const QString &secondText, const QString &alphaText,
                 const QFont &labelFont, const QFont &secondFont, const QFont &alphaFont,
                 Qt::Alignment labelAlign, Qt::Alignment secondAlign, Qt::Alignment alphaAlign)
    : Key{keycode, textGeometry, keyGeometry, keyColor},
      mTextColor{textColor}, mSecondColor{secondColor}, mAlphaColor{alphaColor},
      mLabelAlign{labelAlign}, mSecondAlign{secondAlign}, mAlphaAlign{alphaAlign},
      mLabelFont{labelFont}, mSecondFont{secondFont.resolve(labelFont)},
                              mAlphaFont{alphaFont.resolve(labelFont)},
      mSecondText{secondText}, mAlphaText{alphaText} {
    QRect corner;
    mLabelText = labelText;
    mKeyShape.moveTo(keyGeometry.topLeft() + QPointF{0, topLeft * .5});
    corner.setSize({bottomLeft, bottomLeft});
    corner.moveBottomLeft(keyGeometry.bottomLeft());
    mKeyShape.arcTo(corner, 90 * 2, 90);
    corner.setSize({bottomRight, bottomRight});
    corner.moveBottomRight(keyGeometry.bottomRight());
    mKeyShape.arcTo(corner, 90 * 3, 90);
    corner.setSize({topRight, topRight});
    corner.moveTopRight(keyGeometry.topRight());
    mKeyShape.arcTo(corner, 90 * 0, 90);
    corner.setSize({topLeft, topLeft});
    corner.moveTopLeft(keyGeometry.topLeft());
    mKeyShape.arcTo(corner, 90 * 1, 90);

#ifdef _WIN32
    m_labelFont.setPixelSize(labelSize.height());
    m_labelFont.setStretch(1+(labelSize.width() /
                           QFontMetricsF(m_labelFont).size(Qt::TextSingleLine, m_labelText).width()));
#else
    mLabelFont.setPixelSize(labelSize.height());
    mLabelFont.setStretch(labelSize.width() * mLabelFont.stretch() /
                           QFontMetricsF(mLabelFont).size(Qt::TextSingleLine, mLabelText).width());
#endif
}

void RectKey::paint(QPainter &painter) const {
    Key::paint(painter);
    painter.setFont(mLabelFont);
    painter.setPen(mTextColor);
    painter.drawText(keyGeometry(), mLabelAlign, mLabelText);
    if (!mSecondText.isEmpty()) {
        painter.setPen(mSecondColor);
        painter.setFont(mSecondFont);
        painter.drawText(textGeometry(), mSecondAlign, mSecondText);
    }
    if (!mAlphaText.isEmpty()) {
        painter.setPen(mAlphaColor);
        painter.setFont(mAlphaFont);
        painter.drawText(textGeometry(), mAlphaAlign, mAlphaText);
    }
}

bool RectKey::canAccept(const QPointF &point) {
    return mKeyShape.contains(point);
}
