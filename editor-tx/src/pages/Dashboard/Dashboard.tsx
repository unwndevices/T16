import { useState } from 'react'
import { useConnection } from '@/hooks/useConnection'
import { useConfig } from '@/hooks/useConfig'
import {
  Tabs,
  TabsList,
  TabsTrigger,
  TabsContent,
  Button,
  Skeleton,
  Dialog,
  DialogTrigger,
  DialogContent,
  DialogTitle,
  DialogDescription,
  DialogClose,
} from '@/design-system'
import { DOMAIN, FIELD_GLOBAL, FIELD_BANK } from '@/protocol/sysex'
import { requestCalibration, requestFactoryReset } from '@/services/midi'
import { SCALES, NOTE_NAMES, getNoteNameWithOctave, computeNoteMap } from '@/constants/scales'
import { BankSelector } from '@/components/BankSelector/BankSelector'
import { NoteGrid } from '@/components/NoteGrid'
import { SelectCard } from '@/components/SelectCard/SelectCard'
import { SliderCard } from '@/components/SliderCard/SliderCard'
import { ToggleCard } from '@/components/ToggleCard/ToggleCard'
import { NumberCard } from '@/components/NumberCard/NumberCard'
import { KeyCard } from '@/components/KeyCard/KeyCard'
import { CcCard } from '@/components/CcCard/CcCard'
import styles from './Dashboard.module.css'

// --- Data constants ---

const VELOCITY_CURVES = ['Linear', 'Exponential', 'Logarithmic', 'Cubic']
const AFTERTOUCH_CURVES = ['Linear', 'Exponential', 'Logarithmic', 'Cubic']
const MODES = ['Keyboard', 'Strum', 'Joystick', 'Faders']
const TRS_TYPES = ['Type A', 'Type B']
const CC_NAMES = ['X', 'Y', 'Z', 'M', 'A', 'B', 'C', 'D']

function toSelectOptions(arr: readonly string[]) {
  return arr.map((label, i) => ({ value: String(i), label }))
}

// --- Empty state ---

function EmptyState({ onConnect, onConnectBLE }: { onConnect: () => void; onConnectBLE: () => void }) {
  return (
    <div className={styles.emptyState}>
      <h2 className={styles.emptyHeading}>No Device Connected</h2>
      <p className={styles.emptyBody}>
        Connect your T16 via USB or Bluetooth to start editing.
      </p>
      <div className={styles.connectActions}>
        <Button variant="primary" onClick={onConnect}>
          Connect USB
        </Button>
        <Button variant="secondary" onClick={onConnectBLE}>
          Connect BLE
        </Button>
      </div>
      <div className={styles.skeletonGrid}>
        {Array.from({ length: 6 }).map((_, i) => (
          <Skeleton key={i} width="100%" height={48} borderRadius={8} />
        ))}
      </div>
    </div>
  )
}

// --- Keyboard tab ---

function KeyboardTab() {
  const { config, deviceConfig, selectedBank, updateParam } = useConfig()
  const bank = config.banks[selectedBank]
  const deviceBank = deviceConfig?.banks[selectedBank]

  const isSyncedField = (field: keyof typeof bank): boolean => {
    if (!deviceBank) return true
    return bank[field] === deviceBank[field]
  }

  const handleBankParam = (field: number, value: number) => {
    updateParam(DOMAIN.BANK_KB, selectedBank, field, value)
  }

  // Compute note grid using firmware-matching algorithm
  const keyNotes = computeNoteMap(
    bank.scale,
    bank.note,
    bank.oct,
    bank.flip_x === 1,
    bank.flip_y === 1,
    config.global.custom_scale1,
    config.global.custom_scale2,
  )

  return (
    <div className={styles.tabContent}>
      <BankSelector />

      <div className={styles.noteGridSection}>
        <h3 className={styles.sectionHeading}>Note Mapping</h3>
        <NoteGrid />
      </div>

      <div className={styles.keyGrid}>
        {keyNotes.map((note, i) => (
          <KeyCard
            key={i}
            keyIndex={i}
            noteName={getNoteNameWithOctave(note)}
            noteNumber={note}
            isRoot={note % 12 === bank.note}
            onDecrement={() => handleBankParam(FIELD_BANK.NOTE, Math.max(0, bank.note - 1))}
            onIncrement={() => handleBankParam(FIELD_BANK.NOTE, Math.min(11, bank.note + 1))}
          />
        ))}
      </div>

      <div className={styles.cardGrid}>
        <SelectCard
          label="Scale"
          value={String(bank.scale)}
          onValueChange={(v) => handleBankParam(FIELD_BANK.SCALE, Number(v))}
          options={toSelectOptions(SCALES)}
          showSyncDot={!isSyncedField('scale')}
        />
        <SelectCard
          label="Root Note"
          value={String(bank.note)}
          onValueChange={(v) => handleBankParam(FIELD_BANK.NOTE, Number(v))}
          options={NOTE_NAMES.map((n, i) => ({ value: String(i), label: n }))}
          showSyncDot={!isSyncedField('note')}
        />
        <NumberCard
          label="Octave"
          value={bank.oct}
          onChange={(v) => handleBankParam(FIELD_BANK.OCTAVE, v)}
          min={0}
          max={5}
          showSyncDot={!isSyncedField('oct')}
        />
        <NumberCard
          label="Channel"
          value={bank.ch}
          onChange={(v) => handleBankParam(FIELD_BANK.CHANNEL, v)}
          min={1}
          max={16}
          showSyncDot={!isSyncedField('ch')}
        />
        <SelectCard
          label="Velocity Curve"
          value={String(bank.vel)}
          onValueChange={(v) => handleBankParam(FIELD_BANK.VELOCITY_CURVE, Number(v))}
          options={toSelectOptions(VELOCITY_CURVES)}
          showSyncDot={!isSyncedField('vel')}
        />
        <SelectCard
          label="Aftertouch Curve"
          value={String(bank.at)}
          onValueChange={(v) => handleBankParam(FIELD_BANK.AFTERTOUCH_CURVE, Number(v))}
          options={toSelectOptions(AFTERTOUCH_CURVES)}
          showSyncDot={!isSyncedField('at')}
        />
        <ToggleCard
          label="Flip X"
          checked={bank.flip_x === 1}
          onCheckedChange={(c) => handleBankParam(FIELD_BANK.FLIP_X, c ? 1 : 0)}
          showSyncDot={!isSyncedField('flip_x')}
        />
        <ToggleCard
          label="Flip Y"
          checked={bank.flip_y === 1}
          onCheckedChange={(c) => handleBankParam(FIELD_BANK.FLIP_Y, c ? 1 : 0)}
          showSyncDot={!isSyncedField('flip_y')}
        />
        <ToggleCard
          label="Koala Mode"
          checked={bank.koala_mode === 1}
          onCheckedChange={(c) => handleBankParam(FIELD_BANK.KOALA_MODE, c ? 1 : 0)}
          disabled={bank.scale !== 0}
          showSyncDot={!isSyncedField('koala_mode')}
        />
      </div>
    </div>
  )
}

// --- Custom Scales tab ---

function CustomScalesTab() {
  const { config, updateParam } = useConfig()

  const handleNoteChange = (scaleIndex: number, noteIndex: number, direction: number) => {
    const scale = scaleIndex === 0 ? config.global.custom_scale1 : config.global.custom_scale2
    const newValue = Math.max(0, Math.min(71, scale[noteIndex] + direction))
    // Custom scales are stored in global config.
    // For now, we use a convention: scale field IDs 7+ for custom scale notes
    // This would need proper protocol support for custom scale updates.
    // Using direct config update via updateParam with appropriate field mapping.
    const fieldBase = scaleIndex === 0 ? 100 : 116 // convention for custom scale fields
    updateParam(DOMAIN.GLOBAL, 0, fieldBase + noteIndex, newValue)
  }

  const renderScaleGrid = (scale: readonly number[], scaleIndex: number, title: string) => (
    <div className={styles.scaleSection}>
      <h3 className={styles.sectionHeading}>{title}</h3>
      <div className={styles.scaleGrid}>
        {scale.map((noteValue, noteIndex) => (
          <KeyCard
            key={noteIndex}
            keyIndex={noteIndex}
            noteName={getNoteNameWithOctave(noteValue)}
            noteNumber={noteValue}
            isRoot={noteIndex === 0}
            onDecrement={() => handleNoteChange(scaleIndex, noteIndex, -1)}
            onIncrement={() => handleNoteChange(scaleIndex, noteIndex, 1)}
          />
        ))}
      </div>
    </div>
  )

  return (
    <div className={styles.tabContent}>
      <div className={styles.scalesLayout}>
        {renderScaleGrid(config.global.custom_scale1, 0, 'Custom Scale 1')}
        {renderScaleGrid(config.global.custom_scale2, 1, 'Custom Scale 2')}
      </div>
    </div>
  )
}

// --- CC Mapping tab ---

function CcMappingTab() {
  const { config, selectedBank, updateCCParam } = useConfig()
  const bank = config.banks[selectedBank]

  // Check for duplicate CC assignments
  const duplicates = new Set<number>()
  const seen = new Map<string, number[]>()
  bank.chs.forEach((ch, i) => {
    const key = `${ch}-${bank.ids[i]}`
    const prev = seen.get(key)
    if (prev) {
      duplicates.add(i)
      prev.forEach((idx) => duplicates.add(idx))
    } else {
      seen.set(key, [i])
    }
  })

  const handleChannelChange = (ccIndex: number, channel: number) => {
    updateCCParam(selectedBank, ccIndex, channel, bank.ids[ccIndex])
  }

  const handleIdChange = (ccIndex: number, id: number) => {
    updateCCParam(selectedBank, ccIndex, bank.chs[ccIndex], id)
  }

  return (
    <div className={styles.tabContent}>
      <BankSelector />
      <div className={styles.ccGrid}>
        {CC_NAMES.map((name, index) => (
          <CcCard
            key={index}
            index={index}
            name={name}
            channel={bank.chs[index]}
            ccId={bank.ids[index]}
            onChannelChange={(ch) => handleChannelChange(index, ch)}
            onIdChange={(id) => handleIdChange(index, id)}
            isDuplicate={duplicates.has(index)}
          />
        ))}
      </div>
    </div>
  )
}

// --- Settings tab ---

function SettingsTab() {
  const { config, deviceConfig, updateParam } = useConfig()
  const { output, transport } = useConnection()
  const sender = transport ?? output
  const [calibrationOpen, setCalibrationOpen] = useState(false)
  const [resetOpen, setResetOpen] = useState(false)

  const g = config.global
  const dg = deviceConfig?.global

  const isSyncedGlobal = (field: keyof typeof g): boolean => {
    if (!dg) return true
    return g[field] === dg[field]
  }

  const handleGlobalParam = (field: number, value: number) => {
    updateParam(DOMAIN.GLOBAL, 0, field, value)
  }

  return (
    <div className={styles.tabContent}>
      <div className={styles.cardGrid}>
        <SelectCard
          label="Default Mode"
          value={String(g.mode)}
          onValueChange={(v) => handleGlobalParam(FIELD_GLOBAL.MODE, Number(v))}
          options={toSelectOptions(MODES)}
          showSyncDot={!isSyncedGlobal('mode')}
        />
        <SliderCard
          label="Key Sensitivity"
          value={g.sensitivity}
          onValueChange={(v) => handleGlobalParam(FIELD_GLOBAL.SENSITIVITY, v)}
          min={1}
          max={8}
          showSyncDot={!isSyncedGlobal('sensitivity')}
        />
        <SliderCard
          label="Brightness"
          value={g.brightness}
          onValueChange={(v) => handleGlobalParam(FIELD_GLOBAL.BRIGHTNESS, v)}
          min={1}
          max={8}
          showSyncDot={!isSyncedGlobal('brightness')}
        />
        <ToggleCard
          label="MIDI TRS"
          checked={g.midi_trs === 1}
          onCheckedChange={(c) => handleGlobalParam(FIELD_GLOBAL.MIDI_TRS, c ? 1 : 0)}
          showSyncDot={!isSyncedGlobal('midi_trs')}
        />
        <SelectCard
          label="TRS Type"
          value={String(g.trs_type)}
          onValueChange={(v) => handleGlobalParam(FIELD_GLOBAL.TRS_TYPE, Number(v))}
          options={toSelectOptions(TRS_TYPES)}
          showSyncDot={!isSyncedGlobal('trs_type')}
        />
        <ToggleCard
          label="USB to TRS Passthrough"
          checked={g.passthrough === 1}
          onCheckedChange={(c) => handleGlobalParam(FIELD_GLOBAL.PASSTHROUGH, c ? 1 : 0)}
          showSyncDot={!isSyncedGlobal('passthrough')}
        />
        <ToggleCard
          label="Bluetooth MIDI"
          checked={g.midi_ble === 1}
          onCheckedChange={(c) => handleGlobalParam(FIELD_GLOBAL.MIDI_BLE, c ? 1 : 0)}
          showSyncDot={!isSyncedGlobal('midi_ble')}
        />
      </div>

      <div className={styles.dangerZone}>
        <Dialog open={calibrationOpen} onOpenChange={setCalibrationOpen}>
          <DialogTrigger>
            <Button variant="destructive">Start Calibration</Button>
          </DialogTrigger>
          <DialogContent>
            <DialogTitle>Start Calibration</DialogTitle>
            <DialogDescription>
              This will reset all key sensitivity data. The device will restart and guide you
              through the calibration process. Continue?
            </DialogDescription>
            <div className={styles.dialogActions}>
              <DialogClose>
                <Button variant="ghost">Cancel</Button>
              </DialogClose>
              <Button
                variant="destructive"
                onClick={() => {
                  if (sender) requestCalibration(sender)
                  setCalibrationOpen(false)
                }}
              >
                Start Calibration
              </Button>
            </div>
          </DialogContent>
        </Dialog>

        <Dialog open={resetOpen} onOpenChange={setResetOpen}>
          <DialogTrigger>
            <Button variant="destructive">Factory Reset</Button>
          </DialogTrigger>
          <DialogContent>
            <DialogTitle>Factory Reset</DialogTitle>
            <DialogDescription>
              This will erase all configuration and restore defaults. This cannot be undone. Reset
              device?
            </DialogDescription>
            <div className={styles.dialogActions}>
              <DialogClose>
                <Button variant="ghost">Cancel</Button>
              </DialogClose>
              <Button
                variant="destructive"
                onClick={() => {
                  if (sender) requestFactoryReset(sender)
                  setResetOpen(false)
                }}
              >
                Reset Device
              </Button>
            </div>
          </DialogContent>
        </Dialog>
      </div>
    </div>
  )
}

// --- Main Dashboard ---

export function Dashboard() {
  const { isConnected, isDemo, connect, connectBLE } = useConnection()

  if (!isConnected && !isDemo) {
    return <EmptyState onConnect={connect} onConnectBLE={connectBLE} />
  }

  return (
    <div className={styles.dashboard}>
      <Tabs defaultValue="keyboard">
        <TabsList>
          <TabsTrigger value="keyboard">Keyboard</TabsTrigger>
          <TabsTrigger value="scales">Custom Scales</TabsTrigger>
          <TabsTrigger value="cc">CC Mapping</TabsTrigger>
          <TabsTrigger value="settings">Settings</TabsTrigger>
        </TabsList>
        <TabsContent value="keyboard">
          <KeyboardTab />
        </TabsContent>
        <TabsContent value="scales">
          <CustomScalesTab />
        </TabsContent>
        <TabsContent value="cc">
          <CcMappingTab />
        </TabsContent>
        <TabsContent value="settings">
          <SettingsTab />
        </TabsContent>
      </Tabs>
    </div>
  )
}
