import { useContext } from 'react'
import MidiContext from '../components/MidiProvider'
import { Divider, List, ListItem } from '@chakra-ui/react'
import { SelectCard } from '../components/SelectCard'
import { SliderCard } from '../components/SliderCard'
import { ToggleCard } from '../components/ToggleCard'

export default function Settings() {
    const { config, setConfig } = useContext(MidiContext)

    const handleChange = (id, value) => {
        setConfig((prevConfig) => ({
            ...prevConfig,
            [id]: value,
        }))
    }

    return (
        <List spacing={3}>
            <ListItem>
                <SelectCard
                    name="Default Mode"
                    entries={['Keyboard', 'Joystick', 'Faders']}
                    value={config.mode}
                    onChange={(value) => handleChange('mode', value)}
                />
            </ListItem>

            <Divider my={6} />

            <ListItem>
                <SliderCard
                    name="Brightness"
                    min={10}
                    max={254}
                    value={config.brightness}
                    onChange={(value) => handleChange('brightness', value)}
                />
            </ListItem>

            <Divider my={6} />

            <ListItem>
                <ToggleCard
                    name="Enable Midi TRS"
                    value={config.midi_trs}
                    onChange={(value) => handleChange('midi_trs', value)}
                />
            </ListItem>

            <Divider my={6} />

            <ListItem>
                <ToggleCard
                    name="Midi TRS Type"
                    value={config.trs_type}
                    onChange={(value) => handleChange('trs_type', value)}
                />
            </ListItem>

            <Divider my={6} />

            <ListItem>
                <ToggleCard
                    name="USB to TRS passthrough"
                    value={config.passthrough}
                    onChange={(value) => handleChange('passthrough', value)}
                />
            </ListItem>
            <ListItem>
                <ToggleCard
                    name="🔵🦷"
                    value={config.midi_ble}
                    onChange={(value) => handleChange('midi_ble', value)}
                />
            </ListItem>
        </List>
    )
}
