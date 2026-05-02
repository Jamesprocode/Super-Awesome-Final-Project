import { useCallback, useEffect, useId, useRef, useState } from 'react'
import * as Juce from 'juce-framework-frontend-mirror'
import './MacroKnob.css'

const kSweepDeg = 270
const TRACK_R = 75
const TRACK_ARC_LEN = (kSweepDeg / 360) * 2 * Math.PI * TRACK_R
const kReset = 0.5
const pctFromNorm = (x) => Math.round(x * 100)

function sanitizeSvgFragmentId(reactId) {
  const slug = reactId.replace(/[^a-zA-Z0-9_-]/g, '')
  return `mkmini-${slug || 'knob'}`
}

/** Mapping-page macro dial — same SVG language as Macro page hero knob, compact size. */
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
    const dy = drag.current.lastY - e.clientY
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

  const fid = sanitizeSvgFragmentId(id)
  const pctDisplay = pctFromNorm(norm)

  return (
    <div className="safc-mini-macro">
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
            <radialGradient id={`${fid}-face`} cx="32%" cy="26%" r="80%">
              <stop offset="0%" stopColor="#7c83ff" />
              <stop offset="42%" stopColor="#4338ca" />
              <stop offset="100%" stopColor="#13102e" />
            </radialGradient>
            <radialGradient id={`${fid}-sheen`} cx="30%" cy="22%" r="28%">
              <stop offset="0%" stopColor="#ffffff" stopOpacity={0.28} />
              <stop offset="55%" stopColor="#c7d2fe" stopOpacity={0.05} />
              <stop offset="100%" stopColor="#1e1b4b" stopOpacity={0} />
            </radialGradient>
            <radialGradient id={`${fid}-vignette`} cx="50%" cy="50%" r="50%">
              <stop offset="72%" stopColor="#000000" stopOpacity={0} />
              <stop offset="100%" stopColor="#000000" stopOpacity={0.7} />
            </radialGradient>
            <linearGradient id={`${fid}-bezel`} x1="0%" y1="0%" x2="0%" y2="100%">
              <stop offset="0%" stopColor="#a5b4fc" />
              <stop offset="50%" stopColor="#1e1b4b" />
              <stop offset="100%" stopColor="#4338ca" />
            </linearGradient>
          </defs>

          <circle cx="100" cy="100" r="93" fill="none" stroke={`url(#${fid}-bezel)`} strokeWidth={5} />
          <circle cx="100" cy="100" r="88" fill={`url(#${fid}-face)`} className="macro-knob-face" />
          <circle cx="100" cy="100" r="88" fill={`url(#${fid}-sheen)`} />
          <circle cx="100" cy="100" r="88" fill={`url(#${fid}-vignette)`} />

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
        </svg>
        <span className="macro-knob__center-pct" aria-hidden>
          {pctDisplay}
        </span>
      </div>
    </div>
  )
}
