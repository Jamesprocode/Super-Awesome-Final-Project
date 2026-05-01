import { useEffect, useState } from 'react'
import { MacroKnob } from './MacroKnob'
import { MappingView } from './MappingView.jsx'
import './App.css'

/** Tab indices must stay in sync with PluginEditor.{h,cpp}: macroPageIndex / mapPageIndex */
function clampTab(v) {
  const n = Number(v)
  if (n === 0 || n === 1) return n
  return 0
}

export default function App() {
  const [tab, setTab] = useState(0)

  useEffect(() => {
    window.__SAFC_SET_TAB__ = (idx) => setTab(clampTab(idx))
    return () => delete window.__SAFC_SET_TAB__
  }, [])

  return (
    <div className="app-shell">
      <main className="app-main">
        <div className={tab === 0 ? 'app-panel' : 'app-panel-hidden'}>
          <MacroKnob />
        </div>
        <div className={tab === 1 ? 'app-panel' : 'app-panel-hidden'}>
          <MappingView />
        </div>
      </main>
    </div>
  )
}
