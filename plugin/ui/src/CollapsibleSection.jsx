import { useCallback, useId, useState } from 'react'

/**
 * Accessible expand/collapse for effect groups (Detailed + Mapping views).
 */
export function CollapsibleSection({
  title,
  children,
  headerRight,
  defaultOpen = false,
  /** Use slightly smaller title style (mapping blocks). */
  compactTitle = false,
  panelClassName = '',
}) {
  const [open, setOpen] = useState(defaultOpen)
  const uid = useId().replace(/:/g, '')
  const labelId = `safc-collbl-${uid}`
  const panelId = `safc-colpn-${uid}`

  const toggle = useCallback(() => setOpen((v) => !v), [])

  const titleClass = compactTitle ? 'safc-mapping-title' : 'safc-section-title'

  return (
    <section className={`safc-panel ${panelClassName}`.trim()} data-expanded={open}>
      <header className="safc-section-head">
        <button
          type="button"
          className="safc-collapse-trigger"
          onClick={toggle}
          aria-expanded={open}
          aria-controls={panelId}
          id={labelId}
        >
          <span className="safc-collapse-chevron" data-open={open} aria-hidden>
            ▶
          </span>
          <span className={titleClass}>{title}</span>
        </button>
        <div className="safc-collapse-head-right">{headerRight}</div>
      </header>
      <div className="safc-collapsible-body" data-open={open} aria-hidden={!open}>
        <div className="safc-collapsible-inner">
          <div
            className="safc-collapsible-inner-pad"
            id={panelId}
            role="region"
            aria-labelledby={labelId}
          >
            {children}
          </div>
        </div>
      </div>
    </section>
  )
}
