import { useCallback, useEffect, useId, useRef, useState } from 'react'
import * as Juce from 'juce-framework-frontend-mirror'
import './MacroKnob.css'

const kReset = 0.5

/**
 * 0..1 macro control bound to JUCE WebSliderRelay "macro" and APVTS "macro".
 * In `npm run dev` (no JUCE), uses local state only.
 */
export function MacroKnob() {
  const id = useId()
  const rootRef = useRef(null)
  const drag = useRef({
    active: false,
    lastY: 0,
    local: kReset,
    hasJuce: false,
    slider: null,
  })
  const [norm, setNorm] = useState(kReset)
  /** @type {null | boolean} */
  const [hasJuce, setHasJuce] = useState(null)

  useEffect(() => {
    const s = Juce.getSliderState?.('macro')
    if (!s) {
      setHasJuce(false)
      return
    }

    setHasJuce(true)
    drag.current.hasJuce = true
    drag.current.slider = s
    setNorm(s.getNormalisedValue())

    const listenerId = s.valueChangedEvent.addListener(() => {
      setNorm(s.getNormalisedValue())
    })

    return () => {
      s.valueChangedEvent.removeListener(listenerId)
    }
  }, [])

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
    }
  }, [])

  const onPointerDown = (e) => {
    e.currentTarget.setPointerCapture(e.pointerId)
    if (drag.current.slider) drag.current.slider.sliderDragStarted()
    else drag.current.local = norm

    drag.current.active = true
    drag.current.lastY = e.clientY
  }

  const onPointerMove = (e) => {
    if (!drag.current.active) return
    const dy = e.clientY - drag.current.lastY
    drag.current.lastY = e.clientY
    const next = getValue() - dy * 0.004
    setNormalised(next)
  }

  const onPointerUp = (e) => {
    if (!drag.current.active) return
    drag.current.active = false
    try {
      e.currentTarget.releasePointerCapture(e.pointerId)
    } catch {
      // ignore
    }
    if (drag.current.slider) drag.current.slider.sliderDragEnded()
  }

  const onDoubleClick = (e) => {
    e.preventDefault()
    setNormalised(kReset)
  }

  // 7 o’clock → 5 o’clock (270°)
  const rot = -135 + norm * 270

  return (
    <div className="macro-knob-card">
      <h1 className="macro-knob-title">Macro</h1>
      <p className="macro-knob-hint">Drag vertically · Double-click: 0.5</p>

      <div
        ref={rootRef}
        className="macro-knob"
        onPointerDown={onPointerDown}
        onPointerMove={onPointerMove}
        onPointerUp={onPointerUp}
        onPointerCancel={onPointerUp}
        onDoubleClick={onDoubleClick}
        role="slider"
        id={id}
        aria-label="Macro"
        aria-valuemin={0}
        aria-valuemax={1}
        aria-valuenow={Number(norm.toFixed(4))}
        tabIndex={0}
      >
        <div className="macro-knob-halo" aria-hidden />
        <svg className="macro-knob-dial" viewBox="0 0 200 200" aria-hidden>
          <defs>
            <radialGradient id={`${id}-face`} cx="30%" cy="30%" r="80%">
              <stop offset="0%" stopColor="#4f46e5" />
              <stop offset="55%" stopColor="#312e81" />
              <stop offset="100%" stopColor="#1e1b4b" />
            </radialGradient>
            <filter id={`${id}-glow`} x="-30%" y="-30%" width="160%" height="160%">
              <feGaussianBlur stdDeviation="1.2" result="b" />
              <feMerge>
                <feMergeNode in="b" />
                <feMergeNode in="SourceGraphic" />
              </feMerge>
            </filter>
          </defs>
          <circle cx="100" cy="100" r="88" className="macro-knob-rim" fill="none" />
          <circle cx="100" cy="100" r="80" fill={`url(#${id}-face)`} className="macro-knob-face" />
          <g transform={`rotate(${rot} 100 100)`} filter={`url(#${id}-glow)`}>
            <line
              x1="100"
              y1="100"
              x2="100"
              y2="40"
              stroke="#e0e7ff"
              strokeWidth="3.5"
              strokeLinecap="round"
            />
          </g>
          <circle cx="100" cy="100" r="7" fill="#0f172a" stroke="#a5b4fc" strokeWidth="1.5" />
        </svg>
        <div className="macro-knob-readout" aria-hidden>
          {norm.toFixed(3)}
        </div>
      </div>

      {hasJuce === false && (
        <p className="macro-knob-standalone">Preview (no JUCE) — use Standalone to hear DSP changes.</p>
      )}
    </div>
  )
}
