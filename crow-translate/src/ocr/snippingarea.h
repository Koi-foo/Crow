/*
 *  Copyright © 2018 Ambareesh "Amby" Balaji <ambareeshbalaji@gmail.com>
 *  Copyright © 2018-2021 Hennadii Chernyshchyk <genaloner@gmail.com>
 *
 *  This file is part of Crow Translate.
 *
 *  Crow Translate is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Crow Translate is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a get of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef SNIPPINGAREA_H
#define SNIPPINGAREA_H

#include "settings/appsettings.h"

#include <QStaticText>
#include <QWidget>

class QMouseEvent;
class QPainter;
class QKeyEvent;
class ComparableQPoint;

// This class was heavy edited from Spectacle's QuickEditor
class SnippingArea : public QWidget
{
    Q_OBJECT

public:
    explicit SnippingArea(bool ignoreDevicePixelRatio, QWidget *parent = nullptr);
    ~SnippingArea() override;

    AppSettings::RegionRememberType regionRememberType() const;

    void setCaptureOnRelese(bool onRelease);
    void setShowMagnifier(bool show);
    void setApplyLightMask(bool apply);
    void setRegionRememberType(AppSettings::RegionRememberType type);
    void setCropRegion(QRect region);

    void snip(QMap<const QScreen *, QImage> images);

signals:
    void snipped(const QPixmap &pixmap, int dpi);
    void cancelled();

private:
    enum MouseState : short {
        None = 0, // 0000
        Inside = 1 << 0, // 0001
        Outside = 1 << 1, // 0010
        TopLeft = 5, //101
        Top = 17, // 10001
        TopRight = 9, // 1001
        Right = 33, // 100001
        BottomRight = 6, // 110
        Bottom = 18, // 10010
        BottomLeft = 10, // 1010
        Left = 34, // 100010
        TopLeftOrBottomRight = TopLeft & BottomRight, // 100
        TopRightOrBottomLeft = TopRight & BottomLeft, // 1000
        TopOrBottom = Top & Bottom, // 10000
        RightOrLeft = Right & Left // 100000
    };

    void changeEvent(QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *) override;

    int boundsLeft(int newTopLeftX, bool mouse = true);
    int boundsRight(int newTopLeftX, bool mouse = true);
    int boundsUp(int newTopLeftY, bool mouse = true);
    int boundsDown(int newTopLeftY, bool mouse = true);

    void drawBottomHelpText(QPainter &painter) const;
    void drawDragHandles(QPainter &painter);
    void drawMagnifier(QPainter &painter) const;
    void drawMidHelpText(QPainter &painter) const;
    void drawSelectionSizeTooltip(QPainter &painter, bool dragHandlesVisible) const;

    void setMouseCursor(QPointF pos);
    MouseState mouseLocation(QPointF pos) const;

    QRect scaledCropRegion() const;
    QPixmap selectedPixmap() const;
    void createPixmapFromScreens();
    void setGeometryToScreenPixmap();
    void prepare(QStaticText &text) const;
    void setBottomHelpText();
    void layoutBottomHelpText();

    void preparePaint();

    void acceptSelection();
    void cancelSelection();

    static QMap<ComparableQPoint, ComparableQPoint> computeCoordinatesAfterScaling(const QMap<ComparableQPoint, QPair<qreal, QSize>> &outputsRect);
    static bool isPointInsideCircle(QPointF circleCenter, qreal radius, QPointF point);
    static bool isInRange(qreal low, qreal high, qreal value);
    static bool isWithinThreshold(qreal offset, qreal threshold);

    static constexpr int s_handleRadiusMouse = 9;
    static constexpr int s_handleRadiusTouch = 12;
    static constexpr qreal s_increaseDragAreaFactor = 2.0;
    static constexpr int s_minSpacingBetweenHandles = 20;
    static constexpr int s_borderDragAreaSize = 10;

    static constexpr int s_selectionSizeThreshold = 100;

    static constexpr int s_selectionBoxPaddingX = 5;
    static constexpr int s_selectionBoxPaddingY = 4;
    static constexpr int s_selectionBoxMarginY = 5;

    static constexpr int s_bottomHelpOnReleaseLength = 3;
    static constexpr int s_bottomHelpNormalLength = 6;
    static constexpr int s_bottomHelpMaxLength = s_bottomHelpNormalLength;

    static constexpr int s_bottomHelpBoxPaddingX = 12;
    static constexpr int s_bottomHelpBoxPaddingY = 8;
    static constexpr int s_bottomHelpBoxPairSpacing = 6;
    static constexpr int s_bottomHelpBoxMarginBottom = 5;
    static constexpr int s_midHelpTextFontSize = 12;

    static constexpr int s_magnifierLargeStep = 15;

    static constexpr int s_magZoom = 5;
    static constexpr int s_magPixels = 16;
    static constexpr int s_magOffset = 32;

    const qreal m_devicePixelRatio;
    const qreal m_devicePixelRatioI = 1.0 / m_devicePixelRatio;
    const QColor m_strokeColor = palette().highlight().color();
    const QColor m_crossColor = QColor::fromRgbF(m_strokeColor.redF(), m_strokeColor.greenF(), m_strokeColor.blueF(), 0.7);
    const QColor m_labelForegroundColor = palette().windowText().color();
    const QColor m_labelBackgroundColor = QColor::fromRgbF(palette().light().color().redF(),
                                                     palette().light().color().greenF(),
                                                     palette().light().color().blueF(),
                                                     0.85);

    QColor m_maskColor;
    QRect m_selection;
    QPointF m_startPos;
    QPointF m_initialTopLeft;
    QRect m_screensRect;

    QMap<const QScreen *, QImage> m_images;
    QMap<const QScreen *, qreal> m_screenToDpr;

    QVector<QStaticText> m_bottomLeftHelpText;
    QVector<QVector<QStaticText>> m_bottomRightHelpText;
    QRect m_bottomHelpBorderBox;
    QPoint m_bottomHelpContentPos;
    int m_bottomHelpGridLeftWidth = 0;

    bool m_showMagnifier = AppSettings::defaultShowMagnifier();
    bool m_confirmOnRelease = AppSettings::defaultConfirmOnRelease();
    AppSettings::RegionRememberType m_regionRememberType = AppSettings::defaultRegionRememberType();

    QPixmap m_screenPixmap;
    QPointF m_mousePos;
    MouseState m_mouseDragState = MouseState::None;

    bool m_magnifierAllowed = false;
    bool m_toggleMagnifier = false;
    bool m_disableArrowKeys = false;

    int m_handleRadius = s_handleRadiusMouse; // Radius of handles is either s_handleRadiusMouse or s_handleRadiusTouch
    QVector<QPointF> m_handlePositions = QVector<QPointF>{8}; // Midpoints of handles
};

#endif // SNIPPINGAREA_H
