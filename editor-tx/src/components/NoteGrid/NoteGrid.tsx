import { useConfig } from '@/hooks/useConfig'
import {
  SCALE_INTERVALS,
  computeNoteMap,
  getScaleDegree,
  getNoteNameWithOctave,
} from '@/constants/scales'
import styles from './NoteGrid.module.css'

export function NoteGrid() {
  const { config, selectedBank } = useConfig()
  const bank = config.banks[selectedBank]

  const noteMap = computeNoteMap(
    bank.scale,
    bank.note,
    bank.oct,
    bank.flip_x === 1,
    bank.flip_y === 1,
    config.global.custom_scale1,
    config.global.custom_scale2,
  )

  // Select correct intervals for degree coloring
  let intervals: readonly number[]
  if (bank.scale === 17) {
    intervals = config.global.custom_scale1
  } else if (bank.scale === 18) {
    intervals = config.global.custom_scale2
  } else {
    intervals = SCALE_INTERVALS[bank.scale]
  }

  return (
    <div role="grid" className={styles.grid}>
      {noteMap.map((midiNote, i) => {
        const degree = getScaleDegree(midiNote, bank.note, intervals)
        const isInScale = degree !== null
        const noteName = getNoteNameWithOctave(midiNote)

        let ariaLabel: string
        if (degree === 0) {
          ariaLabel = `${noteName} - root note`
        } else if (isInScale) {
          ariaLabel = `${noteName} - in scale`
        } else {
          ariaLabel = `${noteName} - out of scale`
        }

        const bgStyle: React.CSSProperties | undefined = isInScale
          ? (() => {
              const hue = ((degree * 360) / intervals.length + 270) % 360
              return {
                background: `hsla(${hue}, 70%, 55%, 0.15)`,
                borderLeft: `3px solid hsl(${hue}, 70%, 55%)`,
              }
            })()
          : undefined

        return (
          <div
            key={i}
            role="gridcell"
            aria-label={ariaLabel}
            className={`${styles.cell}${isInScale ? '' : ` ${styles.muted}`}`}
            style={bgStyle}
          >
            <span className={styles.noteName}>{noteName}</span>
            <span className={styles.noteNumber}>{midiNote}</span>
          </div>
        )
      })}
    </div>
  )
}
