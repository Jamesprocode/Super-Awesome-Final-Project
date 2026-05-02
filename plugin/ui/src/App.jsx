import { useState } from 'react'
import { MacroKnob } from './MacroKnob'
import { MappingView } from './MappingView.jsx'
import { TopBar } from './TopBar.jsx'
import './App.css'

export default function App() {
  const [tab, setTab] = useState(0)

  return (
    <div className="app-shell">
      <TopBar tab={tab} onTabChange={setTab} />
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
