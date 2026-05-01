import { useCallback, useEffect, useMemo, useState } from 'react'
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
        Map each DSP parameter to macro 0→1 (min/max + curve). Open a section below to edit its rows.
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
  const [minV, setMinV] = useState(() => existing?.minValue ?? 0)
  const [maxV, setMaxV] = useState(() => existing?.maxValue ?? 1)
  const [curveShape, setCurveShape] = useState(() =>
    curveExponentToShapeId(existing?.curveExponent ?? 1),
  )

  const label = mapped ? `${param.label} *` : param.label

  return (
    <div className="safc-row" style={{ flexDirection: 'column', alignItems: 'stretch', marginBottom: '0.65rem' }}>
      <div className="safc-row" style={{ marginBottom: 0 }}>
        <span className="safc-small-label" style={{ minWidth: '6.5rem' }}>
          {label}
        </span>
        <input
          className="safc-input-num"
          type="number"
          step="any"
          value={minV}
          onChange={(e) => setMinV(Number(e.target.value))}
        />
        <span className="safc-small-label">min</span>
        <input
          className="safc-input-num"
          type="number"
          step="any"
          value={maxV}
          onChange={(e) => setMaxV(Number(e.target.value))}
        />
        <span className="safc-small-label">max</span>
        <select
          className="safc-select"
          style={{ flex: '0 0 auto' }}
          value={curveShape}
          onChange={(e) => setCurveShape(Number(e.target.value))}
        >
          <option value={1}>Linear</option>
          <option value={2}>Logarithmic</option>
          <option value={3}>Exponential</option>
        </select>
        <button
          type="button"
          className="safc-btn"
          onClick={() =>
            onMap({
              targetParamID: param.id,
              minValue: minV,
              maxValue: maxV,
              curveShape,
            })
          }
        >
          Map
        </button>
        <button type="button" className="safc-btn secondary" onClick={() => onUnmap(param.id)}>
          Unmap
        </button>
      </div>
    </div>
  )
}
