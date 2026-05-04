import { useCallback, useEffect, useId, useState } from 'react'
import * as Juce from 'juce-framework-frontend-mirror'
import './SatTypeSelector.css'

/** Must mirror the StringArray order in PluginProcessor.cpp `satType` choice param. */
const SAT_TYPES = ['Cubic', 'Soft', 'Tape', 'Tube', 'Hard']
const MAX_INDEX = SAT_TYPES.length - 1

const indexToNorm = (idx) => idx / MAX_INDEX
const normToIndex = (n) => Math.max(0, Math.min(MAX_INDEX, Math.round(Number(n) * MAX_INDEX)))

export function SatTypeSelector() {
  const id = useId()
  const [index, setIndex] = useState(0)

  useEffect(() => {
    const s = Juce.getSliderState?.('satType')
    if (!s) return undefined
    setIndex(normToIndex(s.getNormalisedValue()))
    const lid = s.valueChangedEvent.addListener(() => {
      setIndex(normToIndex(s.getNormalisedValue()))
    })
    return () => s.valueChangedEvent.removeListener(lid)
  }, [])

  const onChange = useCallback((e) => {
    const next = SAT_TYPES.indexOf(e.target.value)
    if (next < 0) return
    setIndex(next)
    const s = Juce.getSliderState?.('satType')
    if (s) s.setNormalisedValue(indexToNorm(next))
  }, [])

  return (
    <div className="sat-type-selector">
      <label className="sat-type-selector__label" htmlFor={id}>
        Mode
      </label>
      <select
        id={id}
        className="sat-type-selector__select"
        value={SAT_TYPES[index] ?? 'Soft'}
        onChange={onChange}
      >
        {SAT_TYPES.map((name) => (
          <option key={name} value={name}>
            {name}
          </option>
        ))}
      </select>
    </div>
  )
}
