import { MdDelete, MdPause, MdPlayArrow } from 'react-icons/md'
import { useConnection } from '@/hooks/useConnection'
import { useMidiMonitor } from '@/hooks/useMidiMonitor'
import type { MidiMessage } from '@/hooks/useMidiMonitor'
import { Button } from '@/design-system'
import { getNoteNameWithOctave } from '@/constants/scales'
import styles from './Monitor.module.css'

function formatTimestamp(ts: number): string {
  return new Date(ts).toLocaleTimeString('en-GB')
}

function getRowClass(type: MidiMessage['type']): string {
  switch (type) {
    case 'noteon':
      return styles.rowNoteOn
    case 'noteoff':
      return styles.rowNoteOff
    case 'cc':
      return styles.rowCc
  }
}

function getBarClass(type: MidiMessage['type']): string {
  return type === 'cc' ? styles.barCc : styles.barNote
}

function getTypeBadgeClass(type: MidiMessage['type']): string {
  switch (type) {
    case 'noteon':
      return styles.badgeNoteOn
    case 'noteoff':
      return styles.badgeNoteOff
    case 'cc':
      return styles.badgeCc
  }
}

function getTypeLabel(type: MidiMessage['type']): string {
  switch (type) {
    case 'noteon':
      return 'NoteOn'
    case 'noteoff':
      return 'NoteOff'
    case 'cc':
      return 'CC'
  }
}

function getDetail(msg: MidiMessage): string {
  switch (msg.type) {
    case 'cc':
      return `Ch${msg.channel} CC#${msg.data1} = ${msg.data2}`
    case 'noteon':
      return `Ch${msg.channel} ${getNoteNameWithOctave(msg.data1)} vel=${msg.data2}`
    case 'noteoff':
      return `Ch${msg.channel} ${getNoteNameWithOctave(msg.data1)}`
  }
}

function getBarValue(msg: MidiMessage): number {
  switch (msg.type) {
    case 'cc':
      return msg.data2
    case 'noteon':
      return msg.data2
    case 'noteoff':
      return 0
  }
}

function MessageRow({ msg }: { msg: MidiMessage }) {
  const value = getBarValue(msg)
  const widthPercent = (value / 127) * 100

  return (
    <div className={`${styles.row} ${getRowClass(msg.type)}`}>
      <span className={styles.timestamp}>{formatTimestamp(msg.timestamp)}</span>
      <span className={`${styles.typeBadge} ${getTypeBadgeClass(msg.type)}`}>
        {getTypeLabel(msg.type)}
      </span>
      <span className={styles.detail}>{getDetail(msg)}</span>
      <div
        className={styles.barContainer}
        role="meter"
        aria-valuenow={value}
        aria-valuemin={0}
        aria-valuemax={127}
        aria-label={`Value: ${value}`}
      >
        <div
          className={`${styles.barFill} ${getBarClass(msg.type)}`}
          style={{ width: `${widthPercent}%` }}
        />
      </div>
    </div>
  )
}

export function Monitor() {
  const { input } = useConnection()
  const { messages, paused, setPaused, clear } = useMidiMonitor(input)

  return (
    <div className={styles.page}>
      <h1 className={styles.title}>MIDI Monitor</h1>

      <div className={styles.toolbar}>
        <Button variant="secondary" size="sm" onClick={clear}>
          <MdDelete size={16} />
          Clear
        </Button>
        <Button variant="secondary" size="sm" onClick={() => setPaused(!paused)}>
          {paused ? <MdPlayArrow size={16} /> : <MdPause size={16} />}
          {paused ? 'Resume' : 'Pause'}
        </Button>
        <span className={styles.messageCount}>
          {messages.length} messages
        </span>
        {paused && <span className={styles.pausedBadge}>Paused</span>}
      </div>

      {messages.length === 0 ? (
        <div className={styles.emptyState}>
          <h2 className={styles.emptyHeading}>No MIDI Messages</h2>
          <p className={styles.emptyBody}>
            Play your T16 to see incoming MIDI messages here. Connect via USB or BLE to start.
          </p>
        </div>
      ) : (
        <div
          className={styles.messageList}
          role="log"
          aria-live={paused ? 'off' : 'polite'}
        >
          {messages.map((msg) => (
            <MessageRow key={msg.id} msg={msg} />
          ))}
        </div>
      )}
    </div>
  )
}
