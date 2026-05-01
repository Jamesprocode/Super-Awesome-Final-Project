import { useCallback, useEffect, useId, useRef, useState } from 'react'
import * as Juce from 'juce-framework-frontend-mirror'
import './MacroKnob.css'

const kSweepDeg = 270
const TRACK_R = 91
const TRACK_ARC_LEN = (kSweepDeg / 360) * 2 * Math.PI * TRACK_R
const kReset = 0.5

function sanitizeSvgFragmentId(reactId) {
  const slug = reactId.replace(/[^a-zA-Z0-9_-]/g, '')
  return `mkmini-${slug || 'knob'}`
}

function pointerDeg(normalised01) {
  return -135 + normalised01 * kSweepDeg
}

/** Compact duplicate of the Macro page dial; shares APVTS "macro" / relay "macro". */
export function MiniMacroKnob() {
  const id = useId()
  const drag = useRef({
    active: false,
    lastY: 0,
    local: kReset,
    hasJuce: false,
    slider: null,
  })
  const [norm, setNorm] = useState(kReset)

  useEffect(() => {
    const s = Juce.getSliderState?.('macro')
    if (!s) return

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
      /* ignore */
    }
    if (drag.current.slider) drag.current.slider.sliderDragEnded()
  }

  const onDoubleClick = (e) => {
    e.preventDefault()
    setNormalised(kReset)
  }

  const rot = pointerDeg(norm)
  const fid = sanitizeSvgFragmentId(id)
  const pctDisplay = Math.round(norm * 100)

  return (
    <div className="safc-mini-macro">
      <p className="safc-mini-macro__pct" aria-hidden>
        Macro {pctDisplay}%
      </p>
      <div
        className="macro-knob macro-knob--compact"
        onPointerDown={onPointerDown}
        onPointerMove={onPointerMove}
        onPointerUp={onPointerUp}
        onPointerCancel={onPointerUp}
        onDoubleClick={onDoubleClick}
        role="slider"
        id={id}
        aria-label="Macro"
        aria-valuemin={0}
        aria-valuemax={100}
        aria-valuenow={pctDisplay}
        aria-valuetext={`${pctDisplay} percent`}
        tabIndex={0}
      >
        <div className="macro-knob-halo macro-knob-halo--compact" aria-hidden />
        <svg className="macro-knob-dial macro-knob-dial--compact" viewBox="0 0 200 200" aria-hidden>
          <defs>
            <radialGradient id={`${fid}-face`} cx="32%" cy="28%" r="78%">
              <stop offset="0%" stopColor="#6366f1" />
              <stop offset="45%" stopColor="#4338ca" />
              <stop offset="100%" stopColor="#1e1b4b" />
            </radialGradient>
            <linearGradient id={`${fid}-cap`} x1="0%" y1="0%" x2="100%" y2="100%">
              <stop offset="0%" stopColor="#64748b" />
              <stop offset="55%" stopColor="#0f172a" />
              <stop offset="100%" stopColor="#334155" />
            </linearGradient>
            <radialGradient id={`${fid}-sheen`} cx="38%" cy="32%" r="55%">
              <stop offset="0%" stopColor="#ffffff" stopOpacity={0.22} />
              <stop offset="45%" stopColor="#c7d2fe" stopOpacity={0.06} />
              <stop offset="100%" stopColor="#1e1b4b" stopOpacity={0} />
            </radialGradient>
            <filter id={`${fid}-soft`} x="-40%" y="-40%" width="180%" height="180%">
              <feGaussianBlur stdDeviation="0.85" result="b" />
              <feMerge>
                <feMergeNode in="b" />
                <feMergeNode in="SourceGraphic" />
              </feMerge>
            </filter>
          </defs>

          <g className="macro-knob-static-bezel">
            <circle
              cx="100"
              cy="100"
              r={TRACK_R}
              className="macro-knob-track-bg"
              fill="none"
              strokeLinecap="round"
              strokeDasharray={`${TRACK_ARC_LEN} 999`}
              transform="rotate(135 100 100)"
            />
            <circle
              cx="100"
              cy="100"
              r={TRACK_R}
              className="macro-knob-track-fill"
              fill="none"
              strokeLinecap="round"
              strokeDasharray={`${TRACK_ARC_LEN * norm} 999`}
              transform="rotate(135 100 100)"
            />
            <g stroke="#64748b" strokeWidth={2} strokeLinecap="round">
              <line transform="rotate(-135 100 100)" x1="100" y1="7" x2="100" y2="17" />
              <line transform="rotate(-90 100 100)" x1="100" y1="7" x2="100" y2="17" />
              <line transform="rotate(-45 100 100)" x1="100" y1="7" x2="100" y2="17" />
              <line transform="rotate(0 100 100)" x1="100" y1="7" x2="100" y2="17" />
              <line transform="rotate(45 100 100)" x1="100" y1="7" x2="100" y2="17" />
              <line transform="rotate(90 100 100)" x1="100" y1="7" x2="100" y2="17" />
              <line transform="rotate(135 100 100)" x1="100" y1="7" x2="100" y2="17" />
            </g>
            <circle cx="100" cy="100" r="88" className="macro-knob-rim" fill="none" />
          </g>

          <g transform={`rotate(${rot} 100 100)`} className="macro-knob-spin">
            <circle cx="100" cy="100" r="80" fill={`url(#${fid}-face)`} className="macro-knob-face" />
            <circle cx="100" cy="100" r="74" fill={`url(#${fid}-sheen)`} />
            <path
              d="M100 42 L114 116 L86 116 Z"
              className="macro-knob-pointer"
              fill="#f8fafc"
              stroke="#a5b4fc"
              strokeWidth={1}
              strokeLinejoin="round"
              filter={`url(#${fid}-soft)`}
            />
            <circle cx="100" cy="100" r="11" fill={`url(#${fid}-cap)`} stroke="#94a3b8" strokeWidth={1} />
            <circle
              cx="100"
              cy="100"
              r="6"
              fill="#020617"
              stroke="#334155"
              strokeWidth={1}
              opacity={0.85}
            />
          </g>
        </svg>
      </div>
    </div>
  )
}
