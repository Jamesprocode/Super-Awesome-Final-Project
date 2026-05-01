import './ui-theme.css'
import { WebParamKnob } from './WebParamKnob.jsx'
import './WebParamKnob.css'
import { BypassToggle } from './BypassToggle.jsx'
import { CollapsibleSection } from './CollapsibleSection.jsx'

export function DetailedView({ visible }) {
  if (!visible) return null

  const header = (relayId, label = 'Bypass') => (
    <BypassToggle relayId={relayId} label={label} showLabel />
  )

  return (
    <div className="safc-page">
      <div className="safc-muted" style={{ marginBottom: '0.85rem', textAlign: 'center' }}>
        INSERT USEFUL DIRECTIONS HERE FOR NEW USERS
      </div>

      <CollapsibleSection title="EQ" headerRight={header('eqBypass')} defaultOpen={false}>
        {/* 3 rows × 4 cols: each column Low → LM → HM → High; rows Freq / Gain / Q */}
        <div
          className="safc-detail-eq-grid"
          role="group"
          aria-label="EQ bands: columns Low, Low-Mid, High-Mid, High; rows frequency, gain, Q"
        >
          <WebParamKnob relayId="lowFreq" label="Low Freq (Hz)" />
          <WebParamKnob relayId="lowMidFreq" label="Low-Mid Freq (Hz)" />
          <WebParamKnob relayId="highMidFreq" label="High-Mid Freq (Hz)" />
          <WebParamKnob relayId="highFreq" label="High Freq (Hz)" />
          <WebParamKnob relayId="lowGain" label="Low Gain (dB)" />
          <WebParamKnob relayId="lowMidGain" label="Low-Mid Gain (dB)" />
          <WebParamKnob relayId="highMidGain" label="High-Mid Gain (dB)" />
          <WebParamKnob relayId="highGain" label="High Gain (dB)" />
          <WebParamKnob relayId="lowQ" label="Low Q" />
          <WebParamKnob relayId="lowMidQ" label="Low-Mid Q" />
          <WebParamKnob relayId="highMidQ" label="High-Mid Q" />
          <WebParamKnob relayId="highQ" label="High Q" />
        </div>
      </CollapsibleSection>

      <CollapsibleSection title="Compressor" headerRight={header('compBypass')} defaultOpen={false}>
        <div className="safc-knob-cluster">
          <WebParamKnob relayId="threshold" label="Threshold (dB)" />
          <WebParamKnob relayId="ratio" label="Ratio" />
          <WebParamKnob relayId="attack" label="Attack (ms)" />
          <WebParamKnob relayId="release" label="Release (ms)" />
        </div>
      </CollapsibleSection>

      <CollapsibleSection title="Saturator" headerRight={header('satBypass')} defaultOpen={false}>
        <div className="safc-knob-cluster">
          <WebParamKnob relayId="preGain" label="Pre-Gain (dB)" />
          <WebParamKnob relayId="postGain" label="Post-Gain (dB)" />
        </div>
      </CollapsibleSection>

      <CollapsibleSection title="Chorus" headerRight={header('chorusBypass')} defaultOpen={false}>
        <div className="safc-detail-grid safc-detail-grid--5-cols">
          <WebParamKnob relayId="lforate" label="LFO Rate (Hz)" />
          <WebParamKnob relayId="lfodepth" label="LFO Depth (%)" />
          <WebParamKnob relayId="centerdelay" label="Centre Delay (ms)" />
          <WebParamKnob relayId="chorfeedback" label="Feedback (%)" />
          <WebParamKnob relayId="chormix" label="Mix (%)" />
        </div>
      </CollapsibleSection>

      <CollapsibleSection title="Reverb" headerRight={header('reverbBypass')} defaultOpen={false}>
        <div className="safc-detail-grid safc-detail-grid--5-cols">
          <WebParamKnob relayId="roomSize" label="Room Size" />
          <WebParamKnob relayId="damping" label="Damping" />
          <WebParamKnob relayId="width" label="Width" />
          <WebParamKnob relayId="wet" label="Wet" />
          <WebParamKnob relayId="dry" label="Dry" />
        </div>
        <div style={{ marginTop: '1rem', display: 'flex', justifyContent: 'flex-start' }}>
          <BypassToggle relayId="freeze" label="Freeze" showLabel />
        </div>
      </CollapsibleSection>
    </div>
  )
}
