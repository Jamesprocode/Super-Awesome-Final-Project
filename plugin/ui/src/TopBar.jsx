import { useCallback, useEffect, useId, useState } from 'react'
import * as Juce from 'juce-framework-frontend-mirror'
import { BypassToggle } from './BypassToggle.jsx'
import './TopBar.css'

const TABS = [
  { idx: 0, label: 'Macro' },
  { idx: 1, label: 'Mapping' },
]

export function TopBar({ tab, onTabChange }) {
  const presetSelectId = useId()
  const [presets, setPresets] = useState([])
  const [selectedPreset, setSelectedPreset] = useState('Default')

  useEffect(() => {
    const list = Juce.getNativeFunction?.('safc_listPresets')
    if (!list) return
    let cancelled = false
    list().then((raw) => {
      if (cancelled) return
      try {
        const text = typeof raw === 'string' ? raw : String(raw ?? '')
        const arr = JSON.parse(text)
        setPresets(Array.isArray(arr) ? arr : [])
      } catch {
        setPresets([])
      }
    })
    return () => {
      cancelled = true
    }
  }, [])

  useEffect(() => {
    const get = Juce.getNativeFunction?.('safc_getCurrentPresetName')
    if (!get) return
    let cancelled = false
    get().then((raw) => {
      if (cancelled) return
      const name = typeof raw === 'string' ? raw : String(raw ?? '')
      if (name) setSelectedPreset(name)
    })
    return () => {
      cancelled = true
    }
  }, [])

  const onPickPreset = useCallback(async (e) => {
    const name = e.target.value
    setSelectedPreset(name)
    if (!name) return
    const load = Juce.getNativeFunction?.('safc_loadPreset')
    if (!load) return
    await load(name)
    window.dispatchEvent(new CustomEvent('safc:preset-loaded', { detail: { name } }))
  }, [])

  return (
    <header className="app-topbar">
      <div className="app-topbar__slot app-topbar__slot--left">
        <label className="app-topbar__preset-label" htmlFor={presetSelectId}>
          Preset
        </label>
        <select
          id={presetSelectId}
          className="app-topbar__preset-select"
          value={selectedPreset}
          onChange={onPickPreset}
        >
          {presets.map((p) => (
            <option key={p.name} value={p.name}>
              {p.name}
            </option>
          ))}
        </select>
      </div>

      <div className="app-topbar__slot app-topbar__slot--center">
        <div className="app-topbar__tabs" role="tablist" aria-label="Plugin views">
          {TABS.map((t) => (
            <button
              key={t.idx}
              type="button"
              role="tab"
              aria-selected={tab === t.idx}
              className={`app-topbar__tab ${tab === t.idx ? 'is-active' : ''}`}
              onClick={() => onTabChange(t.idx)}
            >
              {t.label}
            </button>
          ))}
        </div>
      </div>

      <div className="app-topbar__slot app-topbar__slot--right">
        <span className="app-topbar__bypass-label">Bypass</span>
        <BypassToggle relayId="allFxBypass" label="Bypass all effects" />
      </div>
    </header>
  )
}
