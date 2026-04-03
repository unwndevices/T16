import { Button } from '@/design-system'
import styles from './KeyCard.module.css'

interface KeyCardProps {
  keyIndex: number
  noteName: string
  noteNumber: number
  isRoot: boolean
  onDecrement: () => void
  onIncrement: () => void
}

export function KeyCard({
  keyIndex,
  noteName,
  noteNumber,
  isRoot,
  onDecrement,
  onIncrement,
}: KeyCardProps) {
  return (
    <div className={`${styles.card} ${isRoot ? styles.root : ''}`}>
      <div className={`${styles.header} ${isRoot ? styles.rootHeader : ''}`}>
        Key {keyIndex + 1}
      </div>
      <div className={styles.body}>
        <Button
          variant="ghost"
          size="sm"
          onClick={onDecrement}
          aria-label={`Decrease note for key ${keyIndex + 1}`}
        >
          -
        </Button>
        <div className={styles.noteInfo}>
          <span className={styles.noteNumber}>{noteNumber}</span>
          <span className={styles.noteName}>{noteName}</span>
        </div>
        <Button
          variant="ghost"
          size="sm"
          onClick={onIncrement}
          aria-label={`Increase note for key ${keyIndex + 1}`}
        >
          +
        </Button>
      </div>
    </div>
  )
}
