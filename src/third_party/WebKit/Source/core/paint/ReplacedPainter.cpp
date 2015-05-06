// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"
#include "core/paint/ReplacedPainter.h"

#include "core/layout/Layer.h"
#include "core/layout/LayoutReplaced.h"
#include "core/layout/PaintInfo.h"
#include "core/paint/BoxPainter.h"
#include "core/paint/GraphicsContextAnnotator.h"
#include "core/paint/ObjectPainter.h"
#include "core/paint/RenderDrawingRecorder.h"
#include "core/paint/RoundedInnerRectClipper.h"

namespace blink {

void ReplacedPainter::paint(const PaintInfo& paintInfo, const LayoutPoint& paintOffset)
{
    ANNOTATE_GRAPHICS_CONTEXT(paintInfo, &m_layoutReplaced);

    if (!m_layoutReplaced.shouldPaint(paintInfo, paintOffset))
        return;

    LayoutPoint adjustedPaintOffset = paintOffset + m_layoutReplaced.location();
    LayoutRect paintRect(adjustedPaintOffset, m_layoutReplaced.size());

    if (m_layoutReplaced.hasBoxDecorationBackground() && (paintInfo.phase == PaintPhaseForeground || paintInfo.phase == PaintPhaseSelection))
        m_layoutReplaced.paintBoxDecorationBackground(paintInfo, adjustedPaintOffset);

    if (paintInfo.phase == PaintPhaseMask) {
        RenderDrawingRecorder renderDrawingRecorder(paintInfo.context, m_layoutReplaced, paintInfo.phase, paintRect);
        m_layoutReplaced.paintMask(paintInfo, adjustedPaintOffset);
        return;
    }

    if (paintInfo.phase == PaintPhaseClippingMask && (!m_layoutReplaced.hasLayer() || !m_layoutReplaced.layer()->hasCompositedClippingMask()))
        return;

    if ((paintInfo.phase == PaintPhaseOutline || paintInfo.phase == PaintPhaseSelfOutline) && m_layoutReplaced.style()->outlineWidth())
        ObjectPainter(m_layoutReplaced).paintOutline(paintInfo, paintRect);

    if (paintInfo.phase != PaintPhaseForeground && paintInfo.phase != PaintPhaseSelection && !m_layoutReplaced.canHaveChildren() && paintInfo.phase != PaintPhaseClippingMask)
        return;

    if (!paintInfo.shouldPaintWithinRoot(&m_layoutReplaced))
        return;

    bool drawSelectionTint = m_layoutReplaced.selectionState() != LayoutObject::SelectionNone && !m_layoutReplaced.document().printing();
    if (paintInfo.phase == PaintPhaseSelection) {
        if (m_layoutReplaced.selectionState() == LayoutObject::SelectionNone)
            return;
        drawSelectionTint = false;
    }

    // FIXME(crbug.com/444591): Refactor this to not create a drawing recorder for renderers with children.
    OwnPtr<RenderDrawingRecorder> renderDrawingRecorder;
    if (!m_layoutReplaced.isSVGRoot())
        renderDrawingRecorder = adoptPtr(new RenderDrawingRecorder(paintInfo.context, m_layoutReplaced, paintInfo.phase, paintRect));
    if (renderDrawingRecorder && renderDrawingRecorder->canUseCachedDrawing())
        return;

    {
        OwnPtr<RoundedInnerRectClipper> clipper;
        bool completelyClippedOut = false;
        if (m_layoutReplaced.style()->hasBorderRadius()) {
            LayoutRect borderRect = LayoutRect(adjustedPaintOffset, m_layoutReplaced.size());

            if (borderRect.isEmpty()) {
                completelyClippedOut = true;
            } else {
                // Push a clip if we have a border radius, since we want to round the foreground content that gets painted.
                FloatRoundedRect roundedInnerRect = m_layoutReplaced.style()->getRoundedInnerBorderFor(paintRect,
                    m_layoutReplaced.paddingTop() + m_layoutReplaced.borderTop(), m_layoutReplaced.paddingBottom() + m_layoutReplaced.borderBottom(), m_layoutReplaced.paddingLeft() + m_layoutReplaced.borderLeft(), m_layoutReplaced.paddingRight() + m_layoutReplaced.borderRight(), true, true);

                clipper = adoptPtr(new RoundedInnerRectClipper(m_layoutReplaced, paintInfo, paintRect, roundedInnerRect, ApplyToContext));
            }
        }

        if (!completelyClippedOut) {
            if (paintInfo.phase == PaintPhaseClippingMask) {
                m_layoutReplaced.paintClippingMask(paintInfo, adjustedPaintOffset);
            } else {
                m_layoutReplaced.paintReplaced(paintInfo, adjustedPaintOffset);
            }
        }
    }

    // The selection tint never gets clipped by border-radius rounding, since we want it to run right up to the edges of
    // surrounding content.
    if (drawSelectionTint) {
        LayoutRect selectionPaintingRect = m_layoutReplaced.localSelectionRect();
        selectionPaintingRect.moveBy(adjustedPaintOffset);
        paintInfo.context->fillRect(pixelSnappedIntRect(selectionPaintingRect), m_layoutReplaced.selectionBackgroundColor());
    }
}

} // namespace blink
