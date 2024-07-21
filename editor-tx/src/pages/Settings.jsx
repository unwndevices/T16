import { useContext, useRef } from 'react'
import MidiContext from '../components/MidiProvider'
import {
    AlertDialog,
    AlertDialogBody,
    AlertDialogContent,
    AlertDialogFooter,
    AlertDialogHeader,
    AlertDialogOverlay,
    Button,
    Divider,
    List,
    ListItem,
    useDisclosure,
    Box,
} from '@chakra-ui/react'
import { SelectCard } from '../components/SelectCard'
import { SliderCard } from '../components/SliderCard'
import { ToggleCard } from '../components/ToggleCard'

export default function Settings() {
    const { config, setConfig, startFullCalibration, syncStatus } =
        useContext(MidiContext)
    const { isOpen, onOpen, onClose } = useDisclosure()
    const cancelRef = useRef()

    const handleChange = (id, value) => {
        setConfig((prevConfig) => ({
            ...prevConfig,
            [id]: value,
        }))
    }

    return (
        <Box display="flex" justifyContent="flex-start">
            <Box w="50vw" pt={5}>
                <List spacing={3}>
                    <ListItem>
                        <SelectCard
                            name="Default Mode"
                            entries={[
                                'Keyboard',
                                'Strum',
                                'Joystick',
                                'Faders',
                            ]}
                            value={config.mode}
                            onChange={(value) => handleChange('mode', value)}
                            isSynced={syncStatus.mode}
                        />
                    </ListItem>

                    <Divider my={6} />

                    <ListItem>
                        <SliderCard
                            name="Brightness"
                            min={1}
                            max={8}
                            value={config.brightness}
                            onChange={(value) =>
                                handleChange('brightness', value)
                            }
                            isSynced={syncStatus.brightness}
                        />
                    </ListItem>

                    <Divider my={6} />

                    <ListItem>
                        <SliderCard
                            name="Key Sensitivity"
                            min={1}
                            max={8}
                            value={config.sensitivity}
                            onChange={(value) =>
                                handleChange('sensitivity', value)
                            }
                            isSynced={syncStatus.sensitivity}
                        />
                    </ListItem>

                    <ListItem>
                        <ToggleCard
                            name="Enable Midi TRS"
                            value={config.midi_trs}
                            onChange={(value) =>
                                handleChange('midi_trs', value)
                            }
                            isSynced={syncStatus.midi_trs}
                        />
                    </ListItem>

                    <Divider my={6} />

                    <ListItem>
                        <ToggleCard
                            name="Midi TRS Type"
                            value={config.trs_type}
                            onChange={(value) =>
                                handleChange('trs_type', value)
                            }
                            isSynced={syncStatus.trs_type}
                        />
                    </ListItem>

                    <Divider my={6} />

                    <ListItem>
                        <ToggleCard
                            name="USB to TRS passthrough"
                            value={config.passthrough}
                            onChange={(value) =>
                                handleChange('passthrough', value)
                            }
                            isSynced={syncStatus.passthrough}
                        />
                    </ListItem>
                    <ListItem>
                        <ToggleCard
                            name="🔵🦷"
                            value={config.midi_ble}
                            onChange={(value) =>
                                handleChange('midi_ble', value)
                            }
                            isSynced={syncStatus.midi_ble}
                        />
                    </ListItem>
                    <Divider my={6} />
                    <ListItem>
                        <Button onClick={onOpen} colorScheme="primary">
                            Keyboard Calibration
                        </Button>
                    </ListItem>
                </List>
                <AlertDialog
                    isOpen={isOpen}
                    leastDestructiveRef={cancelRef}
                    onClose={onClose}
                    motionPreset="slideInBottom"
                >
                    <AlertDialogOverlay>
                        <AlertDialogContent>
                            <AlertDialogHeader>
                                Keyboard Calibration
                            </AlertDialogHeader>
                            <AlertDialogBody>
                                This will start the keyboard magnetic sensors
                                calibration process.
                                <br />
                                1. Press the start button to commence the
                                calibration routine.
                                <br />
                                2. The key to calibrate will light up, press it
                                multiple times to set the min and max values and
                                then click the PLAY MODE button to save and
                                continue to the next key.
                                <br />
                                3. Repeat for all keys, then the device will
                                reboot once done.
                            </AlertDialogBody>
                            <AlertDialogFooter>
                                <Button
                                    ref={cancelRef}
                                    onClick={onClose}
                                    mx={3}
                                >
                                    Cancel
                                </Button>
                                <Button
                                    onClick={startFullCalibration}
                                    mx={3}
                                    colorScheme="secondary"
                                >
                                    Start
                                </Button>
                            </AlertDialogFooter>
                        </AlertDialogContent>
                    </AlertDialogOverlay>
                </AlertDialog>
            </Box>
        </Box>
    )
}
