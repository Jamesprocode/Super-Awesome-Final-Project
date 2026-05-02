import { useCallback, useEffect, useMemo, useRef, useState } from 'react'
import * as Juce from 'juce-framework-frontend-mirror'
import './ui-theme.css'
import { BypassToggle } from './BypassToggle.jsx'
import { CollapsibleSection } from './CollapsibleSection.jsx'
import { MacroRailSlider } from './MacroRailSlider.jsx'
import { MiniMacroKnob } from './MiniMacroKnob.jsx'
import { PowerIcon } from './PowerIcon.jsx'
import {
  FX_CHAIN_BLOCK_TITLE_BY_EFFECT_INDEX,
  FX_CHAIN_RANK_MAX,
  LEGACY_CHAIN_INDEX,
  indexFromNormalised,
  indexToPermutation,
  normalisedFromIndex,
} from './effectChainPermutation.js'

/** Last edited min/max/curve while unmapped; survives toggling macro mapping within the SPA session */
const globalMappingDrafts = new Map()

const BLOCK_TITLE_TO_BYPASS_RELAY = {
  EQ: 'eqBypass',
  Compressor: 'compBypass',
  Saturator: 'satBypass',
  Chorus: 'chorusBypass',
  Reverb: 'reverbBypass',
}

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

/** Normalised slider 0→1 scaled with same domain helpers as macro mapping rail */
function scaledFromRelayNorm(norm01, dmLo, span) {
  if (norm01 == null || !Number.isFinite(norm01)) return null
  return dmLo + norm01 * span
}

function DoubleEndedMappingSlider({
  rangeMin,
  rangeMax,
  low,
  high,
  onAdjustRange,
  onCommitRange,
  currentScaledValue,
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

  let markerPos = null
  if (
    currentScaledValue != null &&
    Number.isFinite(Number(currentScaledValue))
  ) {
    markerPos = clamp(norm(Number(currentScaledValue)), 0, 1)
  }

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
        {markerPos != null ? (
          <span
            className="safc-range-double__marker"
            title="Current parameter value (with macro)"
            style={{ left: `${markerPos * 100}%` }}
          />
        ) : null}
        <span className="safc-range-double__thumb" style={{ left: `${pl * 100}%` }} />
        <span className="safc-range-double__thumb" style={{ left: `${ph * 100}%` }} />
      </div>
    </div>
  )
}

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

export function MappingView() {
  const [state, setState] = useState({ blocks: [], mappings: [] })
  /** Rank of DSP effect chain (0 … 119); matches Macro FX strip + APVTS `fxChainOrder`. */
  const [fxChainRank, setFxChainRank] = useState(LEGACY_CHAIN_INDEX)
  /** Normalised snapshot taken only when attaching mapping — restored on unmap (immune to mount listener noise). */
  const unmapRestoreSnapRef = useRef(new Map())
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
      const data = JSON.parse(text)
      const fo = Number(data.fxChainOrder)
      setState({
        blocks: data.blocks ?? [],
        mappings: data.mappings ?? [],
        ...(Number.isFinite(fo)
          ? { fxChainOrder: Math.max(0, Math.min(FX_CHAIN_RANK_MAX, Math.round(fo))) }
          : {}),
      })
    } catch {
      setState({ blocks: [], mappings: [] })
    }
  }, [])

  useEffect(() => {
    void fetchState()
  }, [fetchState])

  useEffect(() => {
    const od = Number(state.fxChainOrder)
    if (!Number.isFinite(od)) return undefined
    setFxChainRank(Math.max(0, Math.min(FX_CHAIN_RANK_MAX, Math.round(od))))
    return undefined
  }, [state.fxChainOrder])

  useEffect(() => {
    const s = Juce.getSliderState?.('fxChainOrder')
    if (!s) return undefined

    const pull = () =>
      setFxChainRank(
        indexFromNormalised(s.getNormalisedValue?.() ?? normalisedFromIndex(LEGACY_CHAIN_INDEX)),
      )
    pull()
    const lid = s.valueChangedEvent.addListener(pull)

    return () => s.valueChangedEvent.removeListener(lid)


  }, [])

  const sortedBlocks = useMemo(() => {
    const blocks = state.blocks ?? []
    if (!blocks.length) return []

    const perm = indexToPermutation(fxChainRank)
    const posByTitle = new Map()
    perm.forEach((effectIdx, chainSlot) => {
      posByTitle.set(FX_CHAIN_BLOCK_TITLE_BY_EFFECT_INDEX[effectIdx], chainSlot)
    })

    return [...blocks].sort((a, b) => {
      const da = posByTitle.has(a.title) ? posByTitle.get(a.title) : 999
      const db = posByTitle.has(b.title) ? posByTitle.get(b.title) : 999
      if (da !== db) return da - db
      return String(a.title ?? '').localeCompare(String(b.title ?? ''))
    })

  }, [state.blocks, fxChainRank])

  const mappingsByTarget = useMemo(() => {
    const m = new Map()
    for (const row of state.mappings || []) {
      m.set(row.targetParamID, row)
    }
    return m
  }, [state.mappings])

  const applyFn = useCallback(
    async (payload) => {
      const f = Juce.getNativeFunction?.('safc_applyMacroMapping')
      if (!f) return
      await f(payload)
      await fetchState()
    },
    [fetchState],
  )

  const removeFn = useCallback(
    async (paramId) => {
      const f = Juce.getNativeFunction?.('safc_removeMacroMapping')
      if (!f) return
      await f(paramId)
      await fetchState()
    },
    [fetchState],
  )

  return (
    <div className="safc-page safc-mapping-layout">
      <p className="safc-muted" style={{ textAlign: 'center', marginBottom: '1rem' }}>
        REPLACE THIS TEXT WITH THE NEW TEXT
      </p>

      {sortedBlocks.map((block) => {
        const mappedInBlock =
          block.params?.filter((p) => mappingsByTarget.has(p.id)).length ?? 0
        const bypassRelayId = BLOCK_TITLE_TO_BYPASS_RELAY[block.title]

        return (
          <CollapsibleSection
            key={block.title}
            title={block.title}
            headerRight={
              <>
                <MappedCountBadge count={mappedInBlock} />
                {bypassRelayId ? (
                  <BypassToggle
                    relayId={bypassRelayId}
                    label='Bypass'
                    showLabel
                  />
                ) : null}
              </>
            }
            compactTitle
            defaultOpen={false}
            panelClassName="safc-mapping-block"
          >
            {(block.params || []).map((p) => (
              <MappingRow
                key={p.id}
                param={p}
                existing={mappingsByTarget.get(p.id)}
                onMap={applyFn}
                onUnmap={removeFn}
                unmapRestoreSnapRef={unmapRestoreSnapRef}
              />
            ))}
          </CollapsibleSection>
        )
      })}

      {!Juce.getNativeFunction?.('safc_getMappingStateJson') && (
        <p className="safc-preview-note">Preview (no JUCE backend) — mapping controls are inert.</p>
      )}

      <footer className="safc-mapping-macro-footer">
        <MiniMacroKnob />
      </footer>
    </div>
  )
}

function MappingRow({
  param,
  existing,
  onMap,
  onUnmap,
  unmapRestoreSnapRef,
}) {
  const mapped = Boolean(existing)
  const rMin = param.rangeMin
  const rMax = param.rangeMax
  const bounds = useMemo(() => getParamDomain(rMin, rMax), [rMin, rMax])
  const isFreezeParam = param.id === 'freeze'

  const [minV, setMinV] = useState(() => {
    if (existing && Number.isFinite(Number(existing.minValue))) {
      const lo = Number(existing.minValue)
      const hi = Number(existing.maxValue)
      return Math.min(lo, hi)
    }
    const draft = globalMappingDrafts.get(param.id)
    if (draft && Number.isFinite(draft.min)) return draft.min
    return bounds.lo
  })
  const [maxV, setMaxV] = useState(() => {
    if (existing && Number.isFinite(Number(existing.maxValue))) {
      const lo = Number(existing.minValue)
      const hi = Number(existing.maxValue)
      return Math.max(lo, hi)
    }
    const draft = globalMappingDrafts.get(param.id)
    if (draft && Number.isFinite(draft.max)) return draft.max
    return bounds.hi
  })
  const [curveShape, setCurveShape] = useState(() => {
    const draft = globalMappingDrafts.get(param.id)
    if (draft && Number.isFinite(draft.curve)) return draft.curve
    return curveExponentToShapeId(existing?.curveExponent ?? 1)
  })
  const [inverted, setInverted] = useState(() => {
    const draft = globalMappingDrafts.get(param.id)
    if (draft && draft.inverted != null) return Boolean(draft.inverted)
    return Boolean(existing?.inverted)
  })

  /** Live parameter value derived from slider relay norm (tracks macro while mapped). */
  const [relayNorm, setRelayNorm] = useState(null)

  useEffect(() => {
    if (!existing) return
    const lo = Number(existing.minValue)
    const hi = Number(existing.maxValue)
    setMinV(Math.min(lo, hi))
    setMaxV(Math.max(lo, hi))
    setCurveShape(curveExponentToShapeId(existing.curveExponent ?? 1))
    setInverted(Boolean(existing.inverted))

    // eslint-disable-next-line react-hooks/exhaustive-deps -- `existing` is a new object on every JSON fetch; compare fields only
  }, [
    existing?.curveExponent,
    existing?.inverted,
    existing?.maxValue,
    existing?.minValue,
    existing?.targetParamID,
  ])

  useEffect(() => {
    globalMappingDrafts.set(param.id, {
      min: minV,
      max: maxV,
      curve: curveShape,
      inverted,

    })

  }, [curveShape, inverted, maxV, minV, param.id])

  useEffect(() => {
    if (isFreezeParam) {
      const t = Juce.getToggleState?.(param.id)
      if (!t) {
        setRelayNorm(null)
        return undefined
      }
      const sync = () => setRelayNorm(t.getValue() ? 1 : 0)
      sync()
      const lid = t.valueChangedEvent.addListener(sync)
      return () => t.valueChangedEvent.removeListener(lid)
    }

    const s = Juce.getSliderState?.(param.id)
    if (!s) {
      setRelayNorm(null)
      return undefined
    }
    const syncSlider = () => setRelayNorm(s.getNormalisedValue())
    syncSlider()
    const lid = s.valueChangedEvent.addListener(syncSlider)
    return () => s.valueChangedEvent.removeListener(lid)
  }, [isFreezeParam, param.id])

  const sortedMin = minV
  const sortedMax = maxV
  const currentScaledLive = scaledFromRelayNorm(relayNorm, bounds.lo, bounds.span)

  const emitMap = useCallback(() => {
    void onMap({
      targetParamID: param.id,
      minValue: sortedMin,
      maxValue: sortedMax,
      curveShape,
      inverted,
    })

  }, [curveShape, inverted, onMap, param.id, sortedMax, sortedMin])

  const handleRangeAdjust = useCallback((a, b) => {
    setMinV(Math.min(a, b))
    setMaxV(Math.max(a, b))
  }, [])

  /** Snapshot before entering mapped mode — bypasses corrupted cache after unmap (Relay listeners). */
  const readNormSnapshotBeforeMap = () => {
    try {
      if (isFreezeParam) {
        const t = Juce.getToggleState?.(param.id)
        if (!t?.getValue) return 0
        return t.getValue() ? 1 : 0
      }
      const s = Juce.getSliderState?.(param.id)
      if (!s?.getNormalisedValue) return 0.5
      const n = s.getNormalisedValue()
      return Number.isFinite(n) ? n : 0.5
    } catch {
      return isFreezeParam ? 0 : 0.5
    }
  }

  const handleTogglePower = () => {
    const nextMapped = !mapped

    if (nextMapped) {
      unmapRestoreSnapRef.current.set(param.id, readNormSnapshotBeforeMap())
      void emitMap()
      return
    }

    void (async () => {
      await onUnmap(param.id)
      const saved = unmapRestoreSnapRef.current.get(param.id)
      if (saved === undefined) return
      requestAnimationFrame(() => {
        try {
          if (isFreezeParam) {
            const t = Juce.getToggleState?.(param.id)
            if (t?.setValue) t.setValue(saved > 0.5)
            return
          }
          const s = Juce.getSliderState?.(param.id)
          s?.setNormalisedValue?.(saved)
        } catch {
          /* ignore */
        }
      })
    })()
  }

  const formatFixedNorm = useCallback(
    (norm01) => `${fmtMappingValue(bounds.lo + norm01 * bounds.span)}`,
    [bounds.lo, bounds.span],
  )

  const currentValueDisplay =
    relayNorm != null && Number.isFinite(relayNorm)
      ? isFreezeParam
        ? relayNorm >= 0.5
          ? 'On'
          : 'Off'
        : currentScaledLive != null && Number.isFinite(currentScaledLive)
          ? fmtMappingValue(currentScaledLive)
          : '—'
      : null

  return (
    <div className="safc-mapping-row">
      <div className="safc-mapping-row__header">
        <span className="safc-mapping-row__title">{param.label}</span>
        {mapped ? (
          <div className="safc-mapping-row__curve-tools">
            <select
              className="safc-select safc-mapping-row__curve"
              value={curveShape}
              disabled={isFreezeParam}
              onChange={(e) => {
                const next = Number(e.target.value)
                setCurveShape(next)
                void onMap({
                  targetParamID: param.id,
                  minValue: sortedMin,
                  maxValue: sortedMax,
                  curveShape: next,
                  inverted,
                })

              }}

            >
              <option value={1}>Linear</option>


              <option value={2}>Logarithmic</option>

              <option value={3}>Exponential</option>
            </select>
            <label className="safc-mapping-row__invert">
              <input
                type="checkbox"
                checked={inverted}
                onChange={(e) => {
                  const next = e.target.checked
                  setInverted(next)
                  void onMap({
                    targetParamID: param.id,
                    minValue: sortedMin,

                    maxValue: sortedMax,
                    curveShape,
                    inverted: next,
                  })


                }}


              />
              Inverse
            </label>
          </div>
        ) : null}
        <button
          type="button"
          className={`safc-power-toggle ${mapped ? 'is-on' : ''}`}
          aria-pressed={mapped}
          title={
            mapped
              ? 'Mapped to macro — click to disconnect'
              : 'Not mapped — set range / value, then toggle on to map'
          }
          onClick={handleTogglePower}
        >
          <PowerIcon />
        </button>
      </div>
      <div className="safc-mapping-row__slider">
        {mapped ? (
          <>
            <div className="safc-mapping-macro-rail-slot">
              <DoubleEndedMappingSlider
                rangeMin={rMin}
                rangeMax={rMax}
                low={sortedMin}
                high={sortedMax}
                currentScaledValue={
                  currentScaledLive !== null && Number.isFinite(currentScaledLive)
                    ? currentScaledLive
                    : null
                }
                onAdjustRange={handleRangeAdjust}
                onCommitRange={() => {
                  if (mapped) emitMap()
                }}
              />
            </div>
            <div className="safc-mapping-row__value-row" aria-live="polite">
              <div className="safc-mapping-row__current-slot">
                <span className="safc-muted">Current Value:</span>{' '}
                <strong className="safc-mapping-row__current-strong">
                  {currentValueDisplay ?? '—'}
                </strong>
              </div>
              <div className="safc-mapping-row__mapped-range">
                <span className="safc-muted">Mapped range:</span>{' '}
                <span className="safc-mapping-row__numbers">
                  {fmtMappingValue(sortedMin)} to {fmtMappingValue(sortedMax)}
                </span>
              </div>
            </div>
          </>
        ) : isFreezeParam ? (
          <>
            <div className="safc-mapping-row__freeze">
              <BypassToggle relayId="freeze" label="Freeze" showLabel />
            </div>
            <div className="safc-mapping-row__under-slider" aria-live="polite">
              <span className="safc-muted">Current Value:</span>{' '}
              <strong className="safc-mapping-row__current-strong">
                {currentValueDisplay ?? '—'}
              </strong>
            </div>
          </>
        ) : (
          <>
            <div className="safc-mapping-macro-rail-slot">
              <MacroRailSlider
                relayId={param.id}
                showLabelRow={false}
                formatNormalized={(n) => formatFixedNorm(n)}
                sensitivity={0.003}
                resetNormalized={null}
              />
            </div>
            <div className="safc-mapping-row__under-slider" aria-live="polite">
              <span className="safc-muted">Current Value:</span>{' '}
              <strong className="safc-mapping-row__current-strong">
                {currentValueDisplay ?? '—'}
              </strong>
            </div>
          </>
        )}
      </div>
    </div>
  )
}
