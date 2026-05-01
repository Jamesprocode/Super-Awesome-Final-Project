import { useCallback, useEffect, useMemo, useRef, useState } from 'react'
import * as Juce from 'juce-framework-frontend-mirror'
import './ui-theme.css'
import { CollapsibleSection } from './CollapsibleSection.jsx'

function curveExponentToShapeId(c) {
  const x = Number(c)
  if (Number.isNaN(x)) return 1
  if (x <= 1.0001 && x >= 0.999) return 1
  if (x <= 0.55 && x >= 0.45) return 2
  if (x <= 2.01 && x >= 1.99) return 3
  if (x < 1) return 2
  if (x > 1.5) return 3
  return 1
}

/** Fallback when JSON has no DSP range (e.g. offline preview). */
function getParamDomain(rangeMinRaw, rangeMaxRaw) {
  let lo = Number(rangeMinRaw)
  let hi = Number(rangeMaxRaw)
  if (!Number.isFinite(lo)) lo = 0
  if (!Number.isFinite(hi)) hi = 1
  if (hi < lo) [lo, hi] = [hi, lo]
  const span = hi - lo
  if (!(span > 1e-12)) return { lo: lo - 0.5, hi: lo + 0.5, span: 1 }
  return { lo, hi, span }
}

function clamp(n, a, b) {
  return Math.min(b, Math.max(a, n))
}

function fmtMappingValue(v) {
  const a = Math.abs(v)
  if (a >= 1000 || a >= 100) return Number(v.toFixed(1))
  if (a >= 10) return Number(v.toFixed(2))
  if (a >= 1) return Number(v.toFixed(3))
  return Number(v.toFixed(4))
}

/**
 * Horizontal double-ended slider: draggable low / high thumbs on APVTS min–max.
 * Commits mapped values on drag end via onCommitRange.
 */
function DoubleEndedMappingSlider({
  rangeMin,
  rangeMax,
  low,
  high,
  onAdjustRange,
  onCommitRange,
}) {
  const trackRef = useRef(null)
  const dragRoleRef = useRef(null)

  const domain = useMemo(() => getParamDomain(rangeMin, rangeMax), [rangeMin, rangeMax])
  const { lo: dmLo, hi: dmHi, span } = domain

  const norm = useCallback((v) => (v - dmLo) / span, [dmLo, span])
  const pxToValue = useCallback(
    (clientX) => {
      const el = trackRef.current
      if (!el) return dmLo
      const r = el.getBoundingClientRect()
      const w = Math.max(r.width, 1)
      const t = clamp((clientX - r.left) / w, 0, 1)
      return dmLo + t * span
    },
    [dmLo, span],
  )

  const finishDrag = useCallback(() => {
    if (!dragRoleRef.current) return
    dragRoleRef.current = null
    document.body.style.removeProperty('user-select')
    onCommitRange?.()
  }, [onCommitRange])

  /* Pointer may leave OS window — still finalize via window bubble. Track uses pointer capture for move. */
  useEffect(() => {
    const up = () => {
      finishDrag()
    }
    window.addEventListener('pointerup', up)
    window.addEventListener('pointercancel', up)
    return () => {
      window.removeEventListener('pointerup', up)
      window.removeEventListener('pointercancel', up)
    }
  }, [finishDrag])

  const applyDrag = useCallback(
    (clientX, role) => {
      const v = pxToValue(clientX)
      const currLo = Number(low)
      const currHi = Number(high)
      const gap = span * 0.00075 + 1e-9 * (Math.abs(dmLo) + Math.abs(dmHi) + 1)
      if (!(currLo <= currHi)) return
      if (role === 'low') {
        const nl = clamp(v, dmLo, currHi - gap)
        onAdjustRange(nl, currHi)
      } else {
        const nh = clamp(v, currLo + gap, dmHi)
        onAdjustRange(currLo, nh)
      }
    },
    [dmLo, dmHi, high, low, onAdjustRange, pxToValue, span],
  )

  const pickRole = useCallback(
    (clientX) => {
      const el = trackRef.current
      if (!el) return 'low'
      const r = el.getBoundingClientRect()
      const w = Math.max(r.width, 1)
      const plPos = clamp(norm(low), 0, 1)
      const phPos = clamp(norm(high), 0, 1)
      const lx = r.left + plPos * w
      const rx = r.left + phPos * w
      return Math.abs(clientX - lx) <= Math.abs(clientX - rx) ? 'low' : 'high'
    },
    [norm, high, low],
  )

  function onPointerDownTrack(e) {
    if (e.button !== 0) return
    dragRoleRef.current = pickRole(e.clientX)
    document.body.style.userSelect = 'none'
    applyDrag(e.clientX, dragRoleRef.current)
    const t = trackRef.current
    if (t?.setPointerCapture) t.setPointerCapture(e.pointerId)
    e.preventDefault()
  }

  function onPointerMoveTrack(e) {
    if (!dragRoleRef.current) return
    applyDrag(e.clientX, dragRoleRef.current)
  }

  function onPointerUpTrack(e) {
    finishDrag()
    const t = trackRef.current
    if (t?.hasPointerCapture?.(e.pointerId)) {
      try {
        t.releasePointerCapture(e.pointerId)
      } catch {
        /* ignore */
      }
    }
  }

  const pl = clamp(norm(low), 0, 1)
  const ph = clamp(norm(high), 0, 1)
  const leftPct = Math.min(pl, ph) * 100
  const widthPct = Math.abs(ph - pl) * 100

  return (
    <div className="safc-range-double">
      <div
        ref={trackRef}
        className="safc-range-double__track"
        onPointerCancel={onPointerUpTrack}
        onPointerDown={onPointerDownTrack}
        onPointerMove={onPointerMoveTrack}
        onPointerUp={onPointerUpTrack}
        role="presentation"
      >
        <div className="safc-range-double__rail" />
        <div
          className="safc-range-double__fill"
          style={{ left: `${leftPct}%`, width: `${widthPct}%` }}
        />
        <span className="safc-range-double__thumb" style={{ left: `${pl * 100}%` }} />
        <span className="safc-range-double__thumb" style={{ left: `${ph * 100}%` }} />
      </div>
    </div>
  )
}

/** Small badge listing how many mapped params live in this block (visible when collapsed too). */
function MappedCountBadge({ count }) {
  if (count <= 0) return null
  return (
    <span
      className="safc-mapping-badge"
      title={`${count} mapped parameter${count === 1 ? '' : 's'} in this group`}
      aria-hidden
    >
      {count} mapped
    </span>
  )
}

/** IEC standby–style icon (minimal). */
function PowerIcon() {
  return (
    <svg
      xmlns="http://www.w3.org/2000/svg"
      width="22"
      height="22"
      viewBox="0 0 24 24"
      aria-hidden
    >
      <path
        d="M12 5v11"
        fill="none"
        stroke="currentColor"
        strokeWidth="2.2"
        strokeLinecap="round"
      />
      <path
        d="M7.94 13.93a6 6 0 1 1 8.12 0"
        fill="none"
        stroke="currentColor"
        strokeWidth="2.2"
        strokeLinecap="round"
      />
    </svg>
  )
}

export function MappingView({ visible }) {
  const [state, setState] = useState({ blocks: [], mappings: [] })

  const fetchState = useCallback(async () => {
    const fn = Juce.getNativeFunction?.('safc_getMappingStateJson')
    if (!fn) {
      setState({
        blocks: [],
        mappings: [],
      })
      return
    }
    try {
      const raw = await fn()
      const text = typeof raw === 'string' ? raw : String(raw ?? '')
      setState(JSON.parse(text))
    } catch {
      setState({ blocks: [], mappings: [] })
    }
  }, [])

  useEffect(() => {
    if (!visible) return
    fetchState()
  }, [visible, fetchState])

  const mappingsByTarget = useMemo(() => {
    const m = new Map()
    for (const row of state.mappings || []) {
      m.set(row.targetParamID, row)
    }
    return m
  }, [state.mappings])

  const applyFn = useCallback(async (payload) => {
    const f = Juce.getNativeFunction?.('safc_applyMacroMapping')
    if (!f) return
    await f(payload)
    await fetchState()
  }, [fetchState])

  const removeFn = useCallback(async (paramId) => {
    const f = Juce.getNativeFunction?.('safc_removeMacroMapping')
    if (!f) return
    await f(paramId)
    await fetchState()
  }, [fetchState])

  if (!visible) return null

  return (
    <div className="safc-page">
      <p className="safc-muted" style={{ textAlign: 'center', marginBottom: '1rem' }}>
        Map each DSP parameter to macro 0→1 using the green range strip. Use the curve shape, then
        tap the power control to attach or detach mapping.
      </p>

      {(state.blocks || []).map((block) => {
        const mappedInBlock =
          block.params?.filter((p) => mappingsByTarget.has(p.id)).length ?? 0

        return (
          <CollapsibleSection
            key={block.title}
            title={block.title}
            headerRight={<MappedCountBadge count={mappedInBlock} />}
            compactTitle
            defaultOpen={false}
            panelClassName="safc-mapping-block"
          >
            {(block.params || []).map((p) => (
              <MappingRow
                key={
                  mappingsByTarget.get(p.id)
                    ? `${p.id}_${mappingsByTarget.get(p.id).minValue}_${mappingsByTarget.get(p.id).maxValue}_${mappingsByTarget.get(p.id).curveExponent}`
                    : `${p.id}_unset`
                }
                param={p}
                existing={mappingsByTarget.get(p.id)}
                onMap={applyFn}
                onUnmap={removeFn}
              />
            ))}
          </CollapsibleSection>
        )
      })}

      {!Juce.getNativeFunction?.('safc_getMappingStateJson') && (
        <p className="safc-preview-note">Preview (no JUCE backend) — mapping controls are inert.</p>
      )}
    </div>
  )
}

function MappingRow({ param, existing, onMap, onUnmap }) {
  const mapped = Boolean(existing)
  const rMin = param.rangeMin
  const rMax = param.rangeMax
  const bounds = useMemo(() => getParamDomain(rMin, rMax), [rMin, rMax])

  const [minV, setMinV] = useState(() => {
    const lo = Number.isFinite(Number(existing?.minValue))
      ? Number(existing.minValue)
      : bounds.lo
    const hi = Number.isFinite(Number(existing?.maxValue))
      ? Number(existing.maxValue)
      : bounds.hi
    return Math.min(lo, hi)
  })
  const [maxV, setMaxV] = useState(() => {
    const lo = Number.isFinite(Number(existing?.minValue))
      ? Number(existing.minValue)
      : bounds.lo
    const hi = Number.isFinite(Number(existing?.maxValue))
      ? Number(existing.maxValue)
      : bounds.hi
    return Math.max(lo, hi)
  })
  const [curveShape, setCurveShape] = useState(() =>
    curveExponentToShapeId(existing?.curveExponent ?? 1),
  )

  const sortedMin = minV
  const sortedMax = maxV

  const emitMap = useCallback(() => {
    void onMap({
      targetParamID: param.id,
      minValue: sortedMin,
      maxValue: sortedMax,
      curveShape,
    })
  }, [curveShape, onMap, param.id, sortedMax, sortedMin])

  const handleRangeAdjust = useCallback((a, b) => {
    setMinV(Math.min(a, b))
    setMaxV(Math.max(a, b))
  }, [])

  const handleTogglePower = () => {
    if (mapped) void onUnmap(param.id)
    else void emitMap()
  }

  return (
    <div className="safc-mapping-row">
      <div className="safc-mapping-row__header">
        <span className="safc-mapping-row__title">{param.label}</span>
        <button
          type="button"
          className={`safc-power-toggle ${mapped ? 'is-on' : ''}`}
          aria-pressed={mapped}
          title={
            mapped
              ? 'Mapped to macro — click to disconnect'
              : 'Not mapped — set range, then toggle on to map'
          }
          onClick={handleTogglePower}
        >
          <PowerIcon />
        </button>
        <select
          className="safc-select safc-mapping-row__curve"
          value={curveShape}
          onChange={(e) => {
            const next = Number(e.target.value)
            setCurveShape(next)
            if (mapped)
              void onMap({
                targetParamID: param.id,
                minValue: sortedMin,
                maxValue: sortedMax,
                curveShape: next,
              })
          }}
        >
          <option value={1}>Linear</option>
          <option value={2}>Logarithmic</option>
          <option value={3}>Exponential</option>
        </select>
      </div>
      <div className="safc-mapping-row__slider">
        <DoubleEndedMappingSlider
          rangeMin={rMin}
          rangeMax={rMax}
          low={minV}
          high={maxV}
          onAdjustRange={handleRangeAdjust}
          onCommitRange={() => {
            if (mapped) emitMap()
          }}
        />
        <div className="safc-mapping-row__range-readout" aria-live="polite">
          <span className="safc-muted">Range:</span>{' '}
          <span className="safc-mapping-row__numbers">
            {fmtMappingValue(sortedMin)} - {fmtMappingValue(sortedMax)}
          </span>
          <span className="safc-muted safc-mapping-row__bounds">
            (Minimum: {fmtMappingValue(bounds.lo)} | Maximum: {fmtMappingValue(bounds.hi)})
          </span>
        </div>
      </div>
    </div>
  )
}
