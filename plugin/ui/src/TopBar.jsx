import { useCallback, useEffect, useMemo, useRef, useState } from 'react'
import * as Juce from 'juce-framework-frontend-mirror'
import { BypassToggle } from './BypassToggle.jsx'
import './TopBar.css'

const TABS = [
  { idx: 0, label: 'Macro' },
  { idx: 1, label: 'Mapping' },
]

function parseJson(raw, fallback) {
  try {
    const text = typeof raw === 'string' ? raw : String(raw ?? '')
    return JSON.parse(text)
  } catch {
    return fallback
  }
}

function normalisePreset(raw) {
  if (!raw?.name) return null
  const source = raw.source || (raw.builtIn ? 'factory' : 'user')
  return {
    ...raw,
    source,
    id: raw.id || `${source}:${raw.name}`,
    builtIn: source === 'factory',
  }
}

function parseNativeResponse(raw) {
  if (typeof raw === 'boolean') return { ok: raw }
  const parsed = parseJson(raw, null)
  if (parsed && typeof parsed === 'object') return parsed
  return { ok: Boolean(raw) }
}

export function TopBar({ tab, onTabChange }) {
  const [presets, setPresets] = useState([])
  const [selectedPresetId, setSelectedPresetId] = useState('')
  const [busyAction, setBusyAction] = useState('')
  const [presetStatus, setPresetStatus] = useState('')
  const [saveDialogOpen, setSaveDialogOpen] = useState(false)
  const [saveDraftName, setSaveDraftName] = useState('')
  const [deleteDialogOpen, setDeleteDialogOpen] = useState(false)
  const [presetMenuOpen, setPresetMenuOpen] = useState(false)
  const presetMenuRef = useRef(null)

  const refreshPresets = useCallback(async () => {
    const list = Juce.getNativeFunction?.('safc_listPresets')
    if (!list) {
      setPresets([])
      return []
    }

    const raw = await list()
    const arr = parseJson(raw, [])
    const next = Array.isArray(arr) ? arr.map(normalisePreset).filter(Boolean) : []
    setPresets(next)
    return next
  }, [])

  useEffect(() => {
    let cancelled = false
    refreshPresets().then(() => {
      if (cancelled) return
    })
    return () => {
      cancelled = true
    }
  }, [refreshPresets])

  useEffect(() => {
    const getJson = Juce.getNativeFunction?.('safc_getCurrentPresetJson')
    const getName = Juce.getNativeFunction?.('safc_getCurrentPresetName')
    if (!getJson && !getName) return
    let cancelled = false

    const loadCurrent = async () => {
      if (getJson) {
        const raw = await getJson()
        const current = parseJson(raw, null)
        if (cancelled || !current?.name) return
        if (current.id || current.source) {
          setSelectedPresetId(current.id || `${current.source}:${current.name}`)
        }
        return
      }

      const raw = await getName()
      if (cancelled) return
      const name = typeof raw === 'string' ? raw : String(raw ?? '')
      if (name) {
        setSelectedPresetId(`factory:${name}`)
      }
    }

    void loadCurrent()
    return () => {
      cancelled = true
    }
  }, [])

  const factoryPresets = useMemo(
    () => presets.filter((p) => p.source === 'factory'),
    [presets],
  )
  const userPresets = useMemo(
    () => presets.filter((p) => p.source === 'user'),
    [presets],
  )
  const selectedPreset = useMemo(
    () => presets.find((p) => p.id === selectedPresetId) || null,
    [presets, selectedPresetId],
  )
  const presetChoices = useMemo(
    () => [
      { id: '', name: 'Default Setting', source: 'default' },
      ...factoryPresets,
      ...userPresets,
    ],
    [factoryPresets, userPresets],
  )

  useEffect(() => {
    if (!presetMenuOpen) return undefined

    const onPointerDown = (e) => {
      if (!presetMenuRef.current?.contains(e.target)) {
        setPresetMenuOpen(false)
      }
    }
    const onKeyDown = (e) => {
      if (e.key === 'Escape') {
        setPresetMenuOpen(false)
      }
    }

    window.addEventListener('pointerdown', onPointerDown)
    window.addEventListener('keydown', onKeyDown)

    return () => {
      window.removeEventListener('pointerdown', onPointerDown)
      window.removeEventListener('keydown', onKeyDown)
    }
  }, [presetMenuOpen])

  const loadPreset = useCallback(async (preset) => {
    if (!preset) return
    setSelectedPresetId(preset.id)
    setPresetStatus('')
    setPresetMenuOpen(false)

    const load = Juce.getNativeFunction?.('safc_loadPreset')
    if (!load) return
    const ok = await load(preset.name, preset.source)
    setPresetStatus(ok ? 'Loaded' : 'Could not load')
    if (ok) {
      window.dispatchEvent(
        new CustomEvent('safc:preset-loaded', {
          detail: { name: preset.name, source: preset.source },
        }),
      )
    }
  }, [])

  const openSaveDialog = useCallback(() => {
    setSaveDraftName(selectedPreset?.source === 'user' ? selectedPreset.name : '')
    setPresetStatus('')
    setPresetMenuOpen(false)
    setSaveDialogOpen(true)
  }, [selectedPreset])

  const onCancelSavePreset = useCallback(() => {
    if (busyAction) return
    setSaveDialogOpen(false)
    setSaveDraftName('')
  }, [busyAction])

  const onConfirmSavePreset = useCallback(async () => {
    const name = saveDraftName.trim()
    if (!name) {
      setPresetStatus('Name required')
      return
    }

    const save = Juce.getNativeFunction?.('safc_saveUserPreset')
    if (!save) return

    setBusyAction('save')
    setPresetStatus('')
    try {
      const result = parseNativeResponse(await save(name))
      if (!result.ok) {
        setPresetStatus(result.message || 'Could not save')
        return
      }

      const savedName = result.name || name
      const savedId = result.id || `user:${savedName}`
      await refreshPresets()
      setSelectedPresetId(savedId)
      setPresetStatus('Saved')
      setSaveDialogOpen(false)
      setSaveDraftName('')
    } finally {
      setBusyAction('')
    }
  }, [refreshPresets, saveDraftName])

  const openDeleteDialog = useCallback(() => {
    if (!selectedPreset || selectedPreset.source !== 'user') return
    setPresetMenuOpen(false)
    setDeleteDialogOpen(true)
  }, [selectedPreset])

  const onCancelDeletePreset = useCallback(() => {
    if (busyAction) return
    setDeleteDialogOpen(false)
  }, [busyAction])

  const onConfirmDeletePreset = useCallback(async () => {
    if (!selectedPreset || selectedPreset.source !== 'user') return
    const remove = Juce.getNativeFunction?.('safc_deleteUserPreset')
    if (!remove) return

    setBusyAction('delete')
    setPresetStatus('')
    try {
      const result = parseNativeResponse(await remove(selectedPreset.name))
      if (!result.ok) {
        setPresetStatus(result.message || 'Could not delete')
        return
      }

      await refreshPresets()
      setSelectedPresetId('')
      setPresetStatus('Deleted')
      setDeleteDialogOpen(false)
    } finally {
      setBusyAction('')
    }
  }, [refreshPresets, selectedPreset])

  const onReset = useCallback(async () => {
    const reset = Juce.getNativeFunction?.('safc_resetAll')
    if (!reset) return
    setPresetMenuOpen(false)
    await reset()
    setSelectedPresetId('')
    setPresetStatus('')
    window.dispatchEvent(new CustomEvent('safc:preset-loaded', { detail: { name: '' } }))
  }, [])

  const cyclePreset = useCallback((direction) => {
    if (busyAction || presetChoices.length === 0) return

    const selectedIndex = presetChoices.findIndex((p) => p.id === selectedPresetId)
    const currentIndex = selectedIndex >= 0 ? selectedIndex : 0
    const nextIndex = (currentIndex + direction + presetChoices.length) % presetChoices.length
    const nextPreset = presetChoices[nextIndex]

    if (nextPreset.source === 'default') {
      void onReset()
      return
    }

    void loadPreset(nextPreset)
  }, [busyAction, loadPreset, onReset, presetChoices, selectedPresetId])

  const onPresetControlKeyDown = useCallback((e) => {
    if (e.key === 'ArrowLeft') {
      e.preventDefault()
      cyclePreset(-1)
    } else if (e.key === 'ArrowRight') {
      e.preventDefault()
      cyclePreset(1)
    } else if (e.key === 'ArrowDown') {
      e.preventDefault()
      setPresetMenuOpen(true)
    }
  }, [cyclePreset])

  const canSave = !busyAction
  const canDelete = selectedPreset?.source === 'user' && !busyAction
  const presetButtonLabel = selectedPreset?.name || 'Default Setting'

  return (
    <>
      <header className="app-topbar">
        <div className="app-topbar__slot app-topbar__slot--left">
          <div className="app-preset-menu" ref={presetMenuRef}>
            <div className="app-preset-menu__control" onKeyDown={onPresetControlKeyDown}>
              <button
                type="button"
                className="app-preset-menu__step"
                aria-label="Previous preset"
                onClick={() => cyclePreset(-1)}
                disabled={Boolean(busyAction)}
              >
                ‹
              </button>
              <button
                type="button"
                className="app-preset-menu__button"
                aria-haspopup="menu"
                aria-expanded={presetMenuOpen}
                onClick={() => setPresetMenuOpen((open) => !open)}
                title="Preset menu"
              >
                <span className="app-preset-menu__button-label">{presetButtonLabel}</span>
              </button>
              <button
                type="button"
                className="app-preset-menu__step"
                aria-label="Next preset"
                onClick={() => cyclePreset(1)}
                disabled={Boolean(busyAction)}
              >
                ›
              </button>
            </div>

            {presetMenuOpen ? (
              <div className="app-preset-menu__panel" role="menu" aria-label="Preset menu">
                <div className="app-preset-menu__submenu">
                  <button type="button" className="app-preset-menu__item" role="menuitem">
                    Factory
                    <span aria-hidden>›</span>
                  </button>
                  <div className="app-preset-menu__flyout" role="menu" aria-label="Factory presets">
                    {factoryPresets.map((p) => (
                      <button
                        key={p.id}
                        type="button"
                        className={`app-preset-menu__item ${selectedPresetId === p.id ? 'is-selected' : ''}`}
                        role="menuitem"
                        onClick={() => void loadPreset(p)}
                      >
                        <span>{selectedPresetId === p.id ? '✓ ' : ''}{p.name}</span>
                      </button>
                    ))}
                  </div>
                </div>

                <div className="app-preset-menu__submenu">
                  <button type="button" className="app-preset-menu__item" role="menuitem">
                    User Presets
                    <span aria-hidden>›</span>
                  </button>
                  <div className="app-preset-menu__flyout" role="menu" aria-label="User presets">
                    {userPresets.length ? userPresets.map((p) => (
                      <button
                        key={p.id}
                        type="button"
                        className={`app-preset-menu__item ${selectedPresetId === p.id ? 'is-selected' : ''}`}
                        role="menuitem"
                        onClick={() => void loadPreset(p)}
                      >
                        <span>{selectedPresetId === p.id ? '✓ ' : ''}{p.name}</span>
                      </button>
                    )) : (
                      <span className="app-preset-menu__empty">No saved presets</span>
                    )}
                  </div>
                </div>

                <button
                  type="button"
                  className="app-preset-menu__item"
                  role="menuitem"
                  onClick={onReset}
                >
                  {selectedPresetId ? 'Default Setting' : '✓ Default Setting'}
                </button>

                <div className="app-preset-menu__separator" role="separator" />

                <button
                  type="button"
                  className="app-preset-menu__item app-preset-menu__item--primary"
                  role="menuitem"
                  onClick={openSaveDialog}
                  disabled={!canSave}
                >
                  Save As...
                </button>

                <div className="app-preset-menu__submenu">
                  <button type="button" className="app-preset-menu__item" role="menuitem">
                    Options
                    <span aria-hidden>›</span>
                  </button>
                  <div className="app-preset-menu__flyout app-preset-menu__flyout--options" role="menu" aria-label="Preset options">
                    <button
                      type="button"
                      className="app-preset-menu__item"
                      role="menuitem"
                      onClick={openDeleteDialog}
                      disabled={!canDelete}
                    >
                      Delete Current
                    </button>
                    <button
                      type="button"
                      className="app-preset-menu__item"
                      role="menuitem"
                      onClick={onReset}
                    >
                      Reset All
                    </button>
                  </div>
                </div>
              </div>
            ) : null}
          </div>

          {presetStatus ? (
            <span className="app-topbar__preset-status" aria-live="polite">
              {presetStatus}
            </span>
          ) : null}
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

      {saveDialogOpen ? (
        <div className="app-preset-dialog" role="dialog" aria-modal="true" aria-labelledby="preset-save-title">
          <form
            className="app-preset-dialog__panel"
            onSubmit={(e) => {
              e.preventDefault()
              void onConfirmSavePreset()
            }}
          >
            <h2 id="preset-save-title" className="app-preset-dialog__title">
              Save Preset
            </h2>
            <input
              className="app-preset-dialog__input"
              value={saveDraftName}
              onChange={(e) => {
                setSaveDraftName(e.target.value)
                setPresetStatus('')
              }}
              placeholder="Preset name"
              aria-label="Preset name"
              autoFocus
              spellCheck={false}
            />
            <div className="app-preset-dialog__actions">
              <button
                type="button"
                className="app-preset-dialog__button"
                onClick={onCancelSavePreset}
                disabled={Boolean(busyAction)}
              >
                Cancel
              </button>
              <button
                type="submit"
                className="app-preset-dialog__button app-preset-dialog__button--primary"
                disabled={Boolean(busyAction)}
              >
                {busyAction === 'save' ? 'Saving' : 'Save'}
              </button>
            </div>
          </form>
        </div>
      ) : null}

      {deleteDialogOpen ? (
        <div className="app-preset-dialog" role="dialog" aria-modal="true" aria-labelledby="preset-delete-title">
          <form
            className="app-preset-dialog__panel"
            onSubmit={(e) => {
              e.preventDefault()
              void onConfirmDeletePreset()
            }}
          >
            <h2 id="preset-delete-title" className="app-preset-dialog__title">
              Delete Preset
            </h2>
            <p className="app-preset-dialog__message">
              {selectedPreset?.name || 'Selected preset'}
            </p>
            <div className="app-preset-dialog__actions">
              <button
                type="button"
                className="app-preset-dialog__button"
                onClick={onCancelDeletePreset}
                disabled={Boolean(busyAction)}
              >
                Cancel
              </button>
              <button
                type="submit"
                className="app-preset-dialog__button app-preset-dialog__button--danger"
                disabled={Boolean(busyAction)}
              >
                {busyAction === 'delete' ? 'Deleting' : 'Delete'}
              </button>
            </div>
          </form>
        </div>
      ) : null}
    </>
  )
}
