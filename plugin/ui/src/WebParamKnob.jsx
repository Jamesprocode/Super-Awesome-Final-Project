import { useCallback, useEffect, useId, useRef, useState } from 'react'
import * as Juce from 'juce-framework-frontend-mirror'

/**
 * Smaller rotary: vertical-drag → normalised slider via JUCE WebSliderRelay `{relayId}`.
 */
export function WebParamKnob({ relayId, label, sensitivity = 0.004 }) {
  const id = useId()
  const drag = useRef({
    active: false,
    lastY: 0,
    local: 0,
    hasJuce: false,
    slider: null,
  })
  const [norm, setNorm] = useState(0)

  useEffect(() => {
    const s = Juce.getSliderState?.(relayId)
    if (!s) {
      drag.current.hasJuce = false
      setNorm(0)
      return
    }
    drag.current.hasJuce = true
    drag.current.slider = s
    setNorm(s.getNormalisedValue())
    const lid = s.valueChangedEvent.addListener(() => {
      setNorm(s.getNormalisedValue())
    })
    return () => s.valueChangedEvent.removeListener(lid)
  }, [relayId])

  const getValue = useCallback(() => {
    if (drag.current.hasJuce && drag.current.slider)
      return drag.current.slider.getNormalisedValue()
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
    e.preventDefault()
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
    setNormalised(getValue() - dy * sensitivity)
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

  const rot = -135 + norm * 270

  return (
    <div className="safc-web-knob-cell">
      <div
        className="safc-web-knob"
        onPointerDown={onPointerDown}
        onPointerMove={onPointerMove}
        onPointerUp={onPointerUp}
        onPointerCancel={onPointerUp}
        role="presentation"
      >
        <svg className="safc-web-knob-svg" viewBox="0 0 140 140" aria-hidden="true">
          <defs>
            <radialGradient id={`${id}-face`} cx="35%" cy="30%" r="75%">
              <stop offset="0%" stopColor="#4f46e5" />
              <stop offset="55%" stopColor="#312e81" />
              <stop offset="100%" stopColor="#1e1b4b" />
            </radialGradient>
          </defs>
          <circle cx="70" cy="70" r="62" className="safc-web-knob-rim" fill="none" />
          <circle cx="70" cy="70" r="54" fill={`url(#${id}-face)`} />
          <g transform={`rotate(${rot} 70 70)`}>
            <line
              x1="70"
              y1="70"
              x2="70"
              y2="28"
              stroke="#e0e7ff"
              strokeWidth="2.5"
              strokeLinecap="round"
            />
          </g>
          <circle cx="70" cy="70" r="5" fill="#0f172a" stroke="#a5b4fc" strokeWidth="1" />
        </svg>
      </div>
      <span className="safc-web-knob-label">{label}</span>
    </div>
  )
}
