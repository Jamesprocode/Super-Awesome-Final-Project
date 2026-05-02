import { useCallback, useEffect, useId, useState } from 'react'
import * as Juce from 'juce-framework-frontend-mirror'
import { BypassToggle } from './BypassToggle.jsx'
import {
  FX_CHAIN_BLOCK_TITLE_BY_EFFECT_INDEX,
  LEGACY_CHAIN_INDEX,
  indexFromNormalised,
  indexToPermutation,
  normalisedFromIndex,
  permutationToIndex,
  reorderPermutation,
} from './effectChainPermutation.js'

const BYPASS_RELAYS = Object.freeze([
  'eqBypass',
  'compBypass',
  'satBypass',
  'chorusBypass',
  'reverbBypass',
])

const EFFECT_SLOTS = Object.freeze(
  FX_CHAIN_BLOCK_TITLE_BY_EFFECT_INDEX.map((label, effectIndex) => ({
    effectIndex,
    label,
    bypassRelayId: BYPASS_RELAYS[effectIndex],
  })),
)

const EFFECT_BY_IDX = EFFECT_SLOTS.reduce((acc, s) => {
  acc[s.effectIndex] = s
  return acc
}, {})

function stopDragProp(e) {
  e.stopPropagation()
}

export function FxEffectChain() {
  const [order, setOrder] = useState(() => indexToPermutation(LEGACY_CHAIN_INDEX))
  const titleIdRaw = useId()
  const titleId = titleIdRaw.replace(/:/g, '')

  useEffect(() => {
    const s = Juce.getSliderState?.('fxChainOrder')
    if (!s) return undefined

    const pull = () => {
      const norm = s.getNormalisedValue?.() ?? normalisedFromIndex(LEGACY_CHAIN_INDEX)
      const idx = indexFromNormalised(norm)
      setOrder(indexToPermutation(idx))
    }

    pull()
    const lid = s.valueChangedEvent.addListener(pull)
    return () => s.valueChangedEvent.removeListener(lid)

  }, [])

  const commitOrder = useCallback((nextPerm) => {
    const idx = permutationToIndex(nextPerm)
    setOrder([...nextPerm])
    const s = Juce.getSliderState?.('fxChainOrder')
    if (s?.setNormalisedValue) s.setNormalisedValue(normalisedFromIndex(idx))
  }, [])

  const onDragStart = useCallback((e, indexInChain) => {
    e.dataTransfer.effectAllowed = 'move'
    e.dataTransfer.setData('text/plain', String(indexInChain))

  }, [])

  const onDragOver = useCallback((e) => {
    e.preventDefault()
    e.dataTransfer.dropEffect = 'move'

  }, [])

  const onDrop = useCallback(
    (e, dropIdx) => {
      e.preventDefault()
      const from = Number.parseInt(e.dataTransfer.getData('text/plain'), 10)
      if (!Number.isFinite(from)) return
      commitOrder(reorderPermutation(order, from, dropIdx))
    },
    [commitOrder, order],
  )

  return (
    <section className="macro-fx-chain" aria-labelledby={titleId}>
      <ol className="macro-fx-chain__list" aria-label="Effect processing order">
        {order.map((effectIndex, i) => {
          const slot = EFFECT_BY_IDX[effectIndex]
          if (!slot) return null

          const isLast = i === order.length - 1

          return (
            <li
              key={`step-${i}-fx-${effectIndex}`}
              className="macro-fx-chain__slot"
              onDragOver={onDragOver}
              onDrop={(e) => onDrop(e, i)}
            >
              <div
                className="macro-fx-chain__pill"
                draggable
                onDragStart={(e) => onDragStart(e, i)}
                title={`Drag ${slot.label}`}
              >
                <span className="macro-fx-chain__grip" aria-hidden>
                  ⋮⋮
                </span>
                <span className="macro-fx-chain__label">{slot.label}</span>
                <div className="macro-fx-chain__bypass-slot" onPointerDown={stopDragProp}>
                  <BypassToggle relayId={slot.bypassRelayId} label={`Bypass ${slot.label}`} compact />
                </div>
              </div>
              {!isLast ? <span className="macro-fx-chain__arrow" aria-hidden>›</span> : null}
            </li>
          )
        })}
      </ol>
    </section>
  )
}
