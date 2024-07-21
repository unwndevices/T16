import { useState, useEffect, useContext } from 'react'
import {
    Popover,
    PopoverTrigger,
    PopoverContent,
    PopoverBody,
    PopoverArrow,
    Button,
    Box,
    Text,
} from '@chakra-ui/react'
import MidiContext from './MidiProvider'

const MidiMonitor = () => {
    const { input } = useContext(MidiContext)
    const [midiMessages, setMidiMessages] = useState([])

    useEffect(() => {
        if (input) {
            const addMessage = (message) => {
                setMidiMessages((prev) => {
                    // Prepend new message and limit to 20 items
                    return [message, ...prev].slice(0, 20)
                })
            }

            const formatNoteName = (note) => {
                const noteName = note.name
                const sharpSign = note.accidental === '#' ? '#' : ''
                return `${noteName}${sharpSign}${note.octave}`
            }

            const handleNoteOn = (e) => {
                addMessage(
                    `Note On: ${formatNoteName(e.note)} (${Math.round(
                        e.velocity * 127
                    )})`
                )
            }

            const handleNoteOff = (e) => {
                addMessage(`Note Off: ${formatNoteName(e.note)}`)
            }

            const handleAftertouch = (e) => {
                addMessage(`Aftertouch: ${Math.round(e.value * 127)}`)
            }

            input.addListener('noteon', handleNoteOn)
            input.addListener('noteoff', handleNoteOff)
            input.addListener('keyaftertouch', handleAftertouch)

            return () => {
                input.removeListener('noteon', handleNoteOn)
                input.removeListener('noteoff', handleNoteOff)
                input.removeListener('keyaftertouch', handleAftertouch)
            }
        }
    }, [input])

    return (
        <Popover>
            <PopoverTrigger>
                <Button colorScheme="primary" size="sm" mr={4}>
                    Midi Monitor
                </Button>
            </PopoverTrigger>
            <PopoverContent>
                <PopoverArrow />
                <PopoverBody>
                    <Box height="200px" overflowY="auto">
                        {midiMessages.map((msg, index) => (
                            <Text key={index}>{msg}</Text>
                        ))}
                    </Box>
                </PopoverBody>
            </PopoverContent>
        </Popover>
    )
}

export default MidiMonitor
