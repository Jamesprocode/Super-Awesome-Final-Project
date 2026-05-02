import { useEffect, useState } from 'react'
import * as Juce from 'juce-framework-frontend-mirror'
import { PowerIcon } from './PowerIcon.jsx'

export function BypassToggle({
  relayId,
  label = 'Bypass',
  showLabel = false,
  onBypassSynced,
  compact = false,
}) {
  const [bypassOn, setBypassOn] = useState(false)

  useEffect(() => {
    const t = Juce.getToggleState?.(relayId)
    if (!t) {
      setBypassOn(false)
      return undefined
    }
    const apply = (v) => {
      setBypassOn(v)
      onBypassSynced?.(v)
    }
    apply(t.getValue())
    const lid = t.valueChangedEvent.addListener(() => apply(t.getValue()))
    return () => t.valueChangedEvent.removeListener(lid)
  }, [relayId, onBypassSynced])

  function flip() {
    const t = Juce.getToggleState?.(relayId)
    const next = !bypassOn
    if (t) t.setValue(next)
    else {
      setBypassOn(next)
      onBypassSynced?.(next)
    }
  }

  const wrapCls = [
    'safc-bypass-toggle',
    compact && 'safc-bypass-toggle--compact',
    showLabel && 'safc-bypass-toggle--labeled',
  ]
    .filter(Boolean)
    .join(' ')

  return (
    <span className={wrapCls}>
      <button
        type="button"
        className={`safc-power-toggle ${compact ? 'safc-power-toggle--compact' : ''} ${bypassOn ? 'is-bypass-active' : ''}`}
        aria-pressed={bypassOn}
        aria-label={label}
        title={bypassOn ? `${label}: on (bypassed)` : `${label}: off (active)`}
        onClick={flip}
      >
        <PowerIcon />
      </button>
      {showLabel ? <span className="safc-bypass-toggle__label">{label}</span> : null}
    </span>
  )
}
