/**
 * T16 Configuration Types
 * Generated from schema/t16-config.schema.json
 */

export interface T16Config {
    global: GlobalConfig
    banks: BankConfig[]
}

export interface GlobalConfig {
    version: number
    mode: number
    sensitivity: number
    brightness: number
    midi_trs: number
    trs_type: number
    passthrough: number
    midi_ble: number
    custom_scale1: number[]
    custom_scale2: number[]
}

export interface BankConfig {
    pal: number
    ch: number
    scale: number
    oct: number
    note: number
    vel: number
    at: number
    flip_x: number
    flip_y: number
    koala_mode: number
    chs: number[]
    ids: number[]
}
