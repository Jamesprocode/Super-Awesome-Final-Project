import './ui-theme.css'
import { WebParamKnob } from './WebParamKnob.jsx'
import './WebParamKnob.css'
import { BypassToggle } from './BypassToggle.jsx'
import { CollapsibleSection } from './CollapsibleSection.jsx'

export function DetailedView({ visible }) {
  if (!visible) return null

  const header = (relayId, label = 'Bypass') => <BypassToggle relayId={relayId} label={label} />

  return (
    <div className="safc-page">
      <div className="safc-muted" style={{ marginBottom: '0.85rem', textAlign: 'center' }}>
        Drag knobs vertically · each effect folds open from the ▶ header (bypass stays visible in the header)
      </div>

      <CollapsibleSection title="EQ" headerRight={header('eqBypass')} defaultOpen={false}>
        <div className="safc-detail-grid">
          <WebParamKnob relayId="lowFreq" label="Low Freq" />
          <WebParamKnob relayId="lowGain" label="Low Gain" />
          <WebParamKnob relayId="lowQ" label="Low Q" />
          <WebParamKnob relayId="lowMidFreq" label="LM Freq" />
          <WebParamKnob relayId="lowMidGain" label="LM Gain" />
          <WebParamKnob relayId="lowMidQ" label="LM Q" />
          <WebParamKnob relayId="highMidFreq" label="HM Freq" />
          <WebParamKnob relayId="highMidGain" label="HM Gain" />
          <WebParamKnob relayId="highMidQ" label="HM Q" />
          <WebParamKnob relayId="highFreq" label="High Freq" />
          <WebParamKnob relayId="highGain" label="High Gain" />
          <WebParamKnob relayId="highQ" label="High Q" />
        </div>
      </CollapsibleSection>

      <CollapsibleSection title="Compressor" headerRight={header('compBypass')} defaultOpen={false}>
        <div className="safc-detail-grid">
          <WebParamKnob relayId="threshold" label="Threshold" />
          <WebParamKnob relayId="ratio" label="Ratio" />
          <WebParamKnob relayId="attack" label="Attack" />
          <WebParamKnob relayId="release" label="Release" />
        </div>
      </CollapsibleSection>

      <CollapsibleSection title="Saturation" headerRight={header('satBypass')} defaultOpen={false}>
        <div className="safc-detail-grid">
          <WebParamKnob relayId="preGain" label="Pre-Gain" />
          <WebParamKnob relayId="postGain" label="Post-Gain" />
        </div>
      </CollapsibleSection>

      <CollapsibleSection title="Chorus" headerRight={header('chorusBypass')} defaultOpen={false}>
        <div className="safc-detail-grid">
          <WebParamKnob relayId="lforate" label="LFO Rate" />
          <WebParamKnob relayId="lfodepth" label="LFO Depth" />
          <WebParamKnob relayId="centerdelay" label="Centre Delay" />
          <WebParamKnob relayId="chorfeedback" label="Feedback" />
          <WebParamKnob relayId="chormix" label="Mix" />
        </div>
      </CollapsibleSection>

      <CollapsibleSection title="Reverb" headerRight={header('reverbBypass')} defaultOpen={false}>
        <div className="safc-detail-grid">
          <WebParamKnob relayId="roomSize" label="Room" />
          <WebParamKnob relayId="damping" label="Damping" />
          <WebParamKnob relayId="width" label="Width" />
          <WebParamKnob relayId="wet" label="Wet" />
          <WebParamKnob relayId="dry" label="Dry" />
        </div>
        <div style={{ marginTop: '0.65rem', display: 'flex', justifyContent: 'flex-start' }}>
          <BypassToggle relayId="freeze" label="Freeze" />
        </div>
      </CollapsibleSection>
    </div>
  )
}
