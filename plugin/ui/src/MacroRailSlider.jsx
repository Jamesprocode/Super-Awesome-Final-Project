import { useCallback, useEffect, useRef, useState } from 'react'
import * as Juce from 'juce-framework-frontend-mirror'
import './MacroKnob.css'

/**
 * Horizontal drag slider bound to WebSliderRelay (normalised APVTS range).
 * `formatNormalized` maps 0–1 normalized to a display string.
 */
export function MacroRailSlider({
  relayId,
  label,
  formatNormalized,
  sensitivity = 0.003,
  resetNormalized = null,
  /** Optional: normalized 0–1 after user/JUCE commits */
  onNormalisedChange,
}) {
  const drag = useRef({
    active: false,
    lastX: 0,
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
  }

  const onPointerMove = (e) => {
    if (!drag.current.active) return
    const dx = e.clientX - drag.current.lastX
    drag.current.lastX = e.clientX
    setNormalised(getValue() + dx * sensitivity)
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

  const valueText = formatNormalized(norm)

  return (
    <div className="macro-rail-slider" onDoubleClick={onDoubleClick}>
      <div className="macro-rail-slider__labels">
        <span className="macro-rail-slider__label">{label}</span>
        <span className="macro-rail-slider__value" aria-hidden="true">
          {valueText}
        </span>
      </div>
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
          style={{
            left: `${norm * 100}%`,
          }}
          aria-hidden
        />
      </div>
    </div>
  )
}
