/** Lex / factorial-number encoding of five effects — must mirror `permutationIndexToOrder` in `PluginProcessor.cpp`. */

const FACT = Object.freeze([1, 1, 2, 6, 24, 120])

export const FX_CHAIN_RANK_MAX = 119

/** Default matches legacy DSP: Eq → Comp → Sat → Reverb → Chorus */
export const LEGACY_CHAIN_INDEX = 1

/** Block titles from native `appendMappingBlocks` — index matches `FxEffectId` / chain permutation digits. */
export const FX_CHAIN_BLOCK_TITLE_BY_EFFECT_INDEX = Object.freeze([
  'EQ',
  'Compressor',
  'Saturator',
  'Chorus',
  'Reverb',
])

export function indexToPermutation(rawIndex) {
  const index = Math.max(0, Math.min(FX_CHAIN_RANK_MAX, Math.round(Number(rawIndex))))
  const pool = [0, 1, 2, 3, 4]
  let poolSize = 5
  let k = index
  const out = []

  for (let pi = 0; pi < 5; pi++) {
    const f = FACT[poolSize - 1]
    const pos = Math.floor(k / f)
    k %= f
    out.push(pool[pos])
    for (let j = pos + 1; j < poolSize; j++) pool[j - 1] = pool[j]
    poolSize--
  }

  return out
}

export function permutationToIndex(perm) {
  const elems = [0, 1, 2, 3, 4]
  let rank = 0

  for (let i = 0; i < 5; i++) {
    const pos = elems.indexOf(perm[i])
    rank += pos * FACT[5 - 1 - i]
    elems.splice(pos, 1)
  }

  return rank
}

/** APVTS int 0…119 normalized for WebSliderRelay. */
export function normalisedFromIndex(idx) {
  const j = Math.max(0, Math.min(FX_CHAIN_RANK_MAX, Math.round(Number(idx))))
  return j / FX_CHAIN_RANK_MAX
}

export function indexFromNormalised(norm) {
  const n = Math.max(0, Math.min(1, Number(norm)))
  return Math.round(n * FX_CHAIN_RANK_MAX)
}

export function reorderPermutation(permutation, fromIndex, toIndex) {
  if (
    !Array.isArray(permutation) ||
    permutation.length !== 5 ||
    fromIndex === toIndex ||
    fromIndex < 0 ||
    fromIndex >= 5 ||
    toIndex < 0 ||
    toIndex >= 5
  ) {
    return permutation.slice()
  }

  const next = permutation.slice()
  const [moved] = next.splice(fromIndex, 1)
  next.splice(toIndex, 0, moved)
  return next
}
