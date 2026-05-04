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
  /** 'input' | 'output' — when set, renders a live VU meter bar inside the rail. */
  meterChannel = null,
  /** Optional: normalized 0–1 after user/JUCE commits */
  onNormalisedChange,
}) {
  const isVertical = orientation === 'vertical'
  const [meterNorm, setMeterNorm] = useState(0)

  useEffect(() => {
    if (!meterChannel) return undefined
    const get = Juce.getNativeFunction?.('safc_getMeters')
    if (!get) return undefined
    let raf = 0
    let cancelled = false
    let inFlight = false
    /** Linear peak we are displaying. C++ side handles the decay. */
    let displayed = 0
    const tick = () => {
      if (cancelled) return
      if (!inFlight) {
        inFlight = true
        get().then((raw) => {
          inFlight = false
          if (cancelled) return
          try {
            const obj = JSON.parse(typeof raw === 'string' ? raw : String(raw ?? ''))
            const peak = Number(obj?.[meterChannel]) || 0
            displayed = peak
          } catch {
            /* ignore */
          }
        })
      }
      const db = displayed > 1e-5 ? 20 * Math.log10(displayed) : -60
      const clamped = Math.max(-60, Math.min(0, db))
      setMeterNorm((clamped + 60) / 60)
      raf = requestAnimationFrame(tick)
    }
    raf = requestAnimationFrame(tick)
    return () => {
      cancelled = true
      cancelAnimationFrame(raf)
    }
  }, [meterChannel])
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
    setNorm(x)
    if (drag.current.hasJuce && drag.current.slider) {
      drag.current.slider.setNormalisedValue(x)
    } else {
      drag.current.local = x
      onNormalisedChange?.(x)
    }
  }, [onNormalisedChange])

  const railRef = useRef(null)

  const positionFromEvent = useCallback((e) => {
    const rail = railRef.current
    if (!rail) return null
    const rect = rail.getBoundingClientRect()
    if (isVertical) {
      const t = (e.clientY - rect.top) / Math.max(rect.height, 1)
      return Math.min(1, Math.max(0, 1 - t))
    }
    const t = (e.clientX - rect.left) / Math.max(rect.width, 1)
    return Math.min(1, Math.max(0, t))
  }, [isVertical])

  // Drag handler: window-level mousemove/mouseup, imperative coords via refs.
  // We update React state via setNormalised on every move so the thumb visually tracks.
  const onMouseDown = (e) => {
    e.preventDefault()
    if (drag.current.slider) drag.current.slider.sliderDragStarted()
    else drag.current.local = norm
    drag.current.active = true
    drag.current.lastX = e.clientX
    drag.current.lastY = e.clientY

    const handleMove = (ev) => {
      if (!drag.current.active) return
      if (isVertical) {
        const dy = drag.current.lastY - ev.clientY
        drag.current.lastY = ev.clientY
        setNormalised(getValue() + dy * sensitivity)
      } else {
        const dx = ev.clientX - drag.current.lastX
        drag.current.lastX = ev.clientX
        setNormalised(getValue() + dx * sensitivity)
      }
    }
    const handleUp = () => {
      drag.current.active = false
      if (drag.current.slider) drag.current.slider.sliderDragEnded()
      window.removeEventListener('mousemove', handleMove)
      window.removeEventListener('mouseup', handleUp)
    }
    window.addEventListener('mousemove', handleMove)
    window.addEventListener('mouseup', handleUp)
  }

  const onPointerDown = () => {}
  const onPointerMove = () => {}
  const onPointerUp = () => {}

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
    const verticalLabelRow =
      label != null && String(label).length > 0 ? (
        <div className="macro-rail-slider__labels">
          <span className="macro-rail-slider__label">{label}</span>
        </div>
      ) : showLabelRow ? (
        <div className="macro-rail-slider__labels macro-rail-slider__labels--spacer" aria-hidden />
      ) : null

    return (
      <div className={rootCls} title={formatNormalized(norm)} onDoubleClick={onDoubleClick}>
        {verticalLabelRow}
        <div
          ref={railRef}
          className="macro-rail-slider__rail"
          onMouseDown={onMouseDown}
          role="presentation"
        >
          <div className="macro-rail-slider__rail-inner">
            <div className="macro-rail-slider__track-bg" aria-hidden />
            {meterChannel ? (
              <div
                className="macro-rail-slider__meter-fill"
                style={{ clipPath: `inset(${(1 - meterNorm) * 100}% 0 0 0)` }}
                aria-hidden
              />
            ) : (
              <div
                className="macro-rail-slider__track-fill"
                style={{ height: `${norm * 100}%` }}
                aria-hidden
              />
            )}
          </div>
          <div
            className="macro-rail-slider__thumb macro-rail-slider__thumb--pill"
            style={{ bottom: `${norm * 100}%` }}
          >
            <span className="macro-rail-slider__thumb-value">{formatNormalized(norm)}</span>
          </div>
        </div>
      </div>
    )
  }

  return (
    <div className={rootCls} title={formatNormalized(norm)} onDoubleClick={onDoubleClick}>
      {labelRow}
      <div
        className="macro-rail-slider__rail"
        onMouseDown={onMouseDown}
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
