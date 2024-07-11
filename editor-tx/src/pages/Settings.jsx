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
    const { config, setConfig, startFullCalibration } = useContext(MidiContext)
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
                            entries={['Keyboard', 'Joystick', 'Faders']}
                            value={config.mode}
                            onChange={(value) => handleChange('mode', value)}
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
                        />
                    </ListItem>

                    <ListItem>
                        <ToggleCard
                            name="Enable Midi TRS"
                            value={config.midi_trs}
                            onChange={(value) =>
                                handleChange('midi_trs', value)
                            }
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
                        />
                    </ListItem>
                    <ListItem>
                        <ToggleCard
                            name="🔵🦷"
                            value={config.midi_ble}
                            onChange={(value) =>
                                handleChange('midi_ble', value)
                            }
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
                                1. Make sure no keys are pressed before starting
                                the calibration, then press the start button.
                                The keys idle position will be automatically
                                saved.
                                <br />
                                2. The first key will light up, hold it down and
                                press the MODE button to proceed to the next. Do
                                not release the key until the next one lights
                                up.
                                <br />
                                3. Repeat for all keys, the device will reboot
                                once done.
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
