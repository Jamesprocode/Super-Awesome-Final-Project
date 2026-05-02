import { useCallback, useEffect, useRef, useState } from 'react'
import * as Juce from 'juce-framework-frontend-mirror'
import './MacroKnob.css'

/**
 * Drag slider bound to WebSliderRelay (normalised APVTS range).
 * `orientation` selects horizontal (default) or vertical layout.
 * `formatNormalized` maps 0–1 normalized to a display string.
 */
export function MacroRailSlider({
  relayId,
  label,
  showLabelRow = true,
  showFormattedValue = false,
  narrowRail = false,
  formatNormalized,
  sensitivity = 0.003,
  resetNormalized = null,
  orientation = 'horizontal',
  /** Optional: normalized 0–1 after user/JUCE commits */
  onNormalisedChange,
}) {
  const isVertical = orientation === 'vertical'
  const drag = useRef({
    active: false,
    lastX: 0,
    lastY: 0,
    local: 0.5,
    hasJuce: false,
    slider: null,
  })
  const [norm, setNorm] = useState(0)

  useEffect(() => {
    const s = Juce.getSliderState?.(relayId)
    if (!s) {
      drag.current.hasJuce = false
      setNorm(0)
      return undefined
    }
    drag.current.hasJuce = true
    drag.current.slider = s
    setNorm(s.getNormalisedValue())
    const lid = s.valueChangedEvent.addListener(() => {
      const n = s.getNormalisedValue()
      setNorm(n)
      onNormalisedChange?.(n)
    })
    return () => s.valueChangedEvent.removeListener(lid)
  }, [relayId, onNormalisedChange])

  const getValue = useCallback(() => {
    if (drag.current.hasJuce && drag.current.slider) return drag.current.slider.getNormalisedValue()
    return drag.current.local
  }, [])

  const setNormalised = useCallback((v) => {
    const x = Math.min(1, Math.max(0, v))
    if (drag.current.hasJuce && drag.current.slider) {
      drag.current.slider.setNormalisedValue(x)
    } else {
      drag.current.local = x
      setNorm(x)
      onNormalisedChange?.(x)
    }
  }, [onNormalisedChange])

  const onPointerDown = (e) => {
    e.preventDefault()
    e.currentTarget.setPointerCapture(e.pointerId)
    if (drag.current.slider) drag.current.slider.sliderDragStarted()
    else drag.current.local = norm
    drag.current.active = true
    drag.current.lastX = e.clientX
    drag.current.lastY = e.clientY
  }

  const onPointerMove = (e) => {
    if (!drag.current.active) return
    if (isVertical) {
      const dy = drag.current.lastY - e.clientY
      drag.current.lastY = e.clientY
      setNormalised(getValue() + dy * sensitivity)
    } else {
      const dx = e.clientX - drag.current.lastX
      drag.current.lastX = e.clientX
      setNormalised(getValue() + dx * sensitivity)
    }
  }

  const onPointerUp = (e) => {
    if (!drag.current.active) return
    drag.current.active = false
    try {
      e.currentTarget.releasePointerCapture(e.pointerId)
    } catch {
      /* ignore */
    }
    if (drag.current.slider) drag.current.slider.sliderDragEnded()
  }

  const onDoubleClick = (e) => {
    if (resetNormalized == null) return
    e.preventDefault()
    setNormalised(resetNormalized)
  }

  const labelRow =
    label != null && String(label).length > 0 ? (
      <div className="macro-rail-slider__labels">
        <span className="macro-rail-slider__label">{label}</span>
        {showFormattedValue ? (
          <span className="macro-rail-slider__value" aria-hidden="true">
            {formatNormalized(norm)}
          </span>
        ) : null}
      </div>
    ) : showLabelRow ? (
      <div className="macro-rail-slider__labels macro-rail-slider__labels--spacer" aria-hidden />
    ) : null

  const rootCls =
    `macro-rail-slider ${isVertical ? 'macro-rail-slider--vertical' : ''}${narrowRail ? ' macro-rail-slider--narrow-rail' : ''}`.trim()

  if (isVertical) {
    return (
      <div className={rootCls} title={formatNormalized(norm)} onDoubleClick={onDoubleClick}>
        {labelRow}
        <div
          className="macro-rail-slider__rail"
          onPointerDown={onPointerDown}
          onPointerMove={onPointerMove}
          onPointerUp={onPointerUp}
          onPointerCancel={onPointerUp}
          role="presentation"
        >
          <div className="macro-rail-slider__rail-inner">
            <div className="macro-rail-slider__track-bg" aria-hidden />
            <div
              className="macro-rail-slider__track-fill"
              style={{ height: `${norm * 100}%` }}
              aria-hidden
            />
          </div>
          <div
            className="macro-rail-slider__thumb"
            style={{ bottom: `${norm * 100}%` }}
            aria-hidden
          />
        </div>
      </div>
    )
  }

  return (
    <div className={rootCls} title={formatNormalized(norm)} onDoubleClick={onDoubleClick}>
      {labelRow}
      <div
        className="macro-rail-slider__rail"
        onPointerDown={onPointerDown}
        onPointerMove={onPointerMove}
        onPointerUp={onPointerUp}
        onPointerCancel={onPointerUp}
        role="presentation"
      >
        <div className="macro-rail-slider__rail-inner">
          <div className="macro-rail-slider__track-bg" aria-hidden />
          <div
            className="macro-rail-slider__track-fill"
            style={{ width: `${norm * 100}%` }}
            aria-hidden
          />
        </div>
        <div
          className="macro-rail-slider__thumb"
          style={{ left: `${norm * 100}%` }}
          aria-hidden
        />
      </div>
    </div>
  )
}
