import { useEffect, useState } from 'react'
import * as Juce from 'juce-framework-frontend-mirror'

export function BypassToggle({ relayId, label = 'Bypass' }) {
  const [on, setOn] = useState(false)
  useEffect(() => {
    const t = Juce.getToggleState?.(relayId)
    if (!t) {
      setOn(false)
      return
    }
    setOn(t.getValue())
    const lid = t.valueChangedEvent.addListener(() => setOn(t.getValue()))
    return () => t.valueChangedEvent.removeListener(lid)
  }, [relayId])

  return (
    <label className="safc-toggle">
      <input
        type="checkbox"
        checked={on}
        onChange={(e) => {
          const t = Juce.getToggleState?.(relayId)
          if (t) t.setValue(e.target.checked)
          else setOn(e.target.checked)
        }}
      />
      <span>{label}</span>
    </label>
  )
}
