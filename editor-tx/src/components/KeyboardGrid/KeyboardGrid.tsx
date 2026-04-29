import type { ReactNode } from 'react'
import styles from './KeyboardGrid.module.css'

export type KeyboardGridProps = {
  keys: number
  cols: number
  rows: number
  renderKey: (index: number) => ReactNode
  'aria-label': string
}

export function KeyboardGrid({ keys, cols, rows, renderKey, ...rest }: KeyboardGridProps) {
  // Defensive: keys must equal cols * rows. Throw in dev for early catch.
  if (cols * rows !== keys && import.meta.env.DEV) {
    // eslint-disable-next-line no-console
    console.error(`[KeyboardGrid] cols (${cols}) * rows (${rows}) !== keys (${keys})`)
  }
  const cells: ReactNode[] = []
  for (let i = 0; i < keys; i++) {
    cells.push(
      <div key={i} className={styles.cell}>
        {renderKey(i)}
      </div>,
    )
  }
  return (
    <div
      role="grid"
      aria-label={rest['aria-label']}
      className={styles.grid}
      data-keys={keys}
      data-cols={cols}
      style={{ gridTemplateColumns: `repeat(${cols}, 1fr)` }}
    >
      {cells}
    </div>
  )
}
