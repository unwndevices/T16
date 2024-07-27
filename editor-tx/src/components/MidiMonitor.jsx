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
    PopoverHeader,
    Checkbox,
    HStack,
} from '@chakra-ui/react'
import MidiContext from './MidiProvider'

const MidiMonitor = () => {
    const { input } = useContext(MidiContext)
    const [midiMessages, setMidiMessages] = useState([])
    const [filters, setFilters] = useState({
        noteOnOff: true,
        aftertouch: true,
        controlChange: true,
    })

    useEffect(() => {
        if (input) {
            const addMessage = (message, type) => {
                setMidiMessages((prev) => {
                    return [{ message, type }, ...prev].slice(0, 20)
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
                    )})`,
                    'noteOn'
                )
            }

            const handleNoteOff = (e) => {
                addMessage(`Note Off: ${formatNoteName(e.note)}`, 'noteOff')
            }

            const handleAftertouch = (e) => {
                addMessage(
                    `Aftertouch: ${Math.round(e.value * 127)}`,
                    'aftertouch'
                )
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

    const handleFilterChange = (filterName) => {
        setFilters((prev) => ({ ...prev, [filterName]: !prev[filterName] }))
    }

    const formatMessage = (msg) => {
        const parts = msg.message.split(':')
        const type = parts[0] + ':'
        const value = parts.slice(1).join(':').trim()

        return (
            <Text key={msg.id} fontSize="sm">
                <Text as="span" color={getMessageColor(msg.type)}>
                    {type}
                </Text>
                <Text as="span" color="gray.700">
                    {' ' + value}
                </Text>
            </Text>
        )
    }

    const getMessageColor = (type) => {
        switch (type) {
            case 'noteOn':
                return 'green.500'
            case 'noteOff':
                return 'red.500'
            case 'aftertouch':
                return 'orange.500'
            default:
                return 'gray.500'
        }
    }

    const filteredMessages = midiMessages.filter((msg) => {
        if (
            (msg.type === 'noteOn' || msg.type === 'noteOff') &&
            !filters.noteOnOff
        )
            return false
        if (msg.type === 'aftertouch' && !filters.aftertouch) return false
        return true
    })

    return (
        <Popover>
            <PopoverTrigger>
                <Button colorScheme="primary" size="sm" mr={4}>
                    Midi Monitor
                </Button>
            </PopoverTrigger>
            <PopoverContent width="400px">
                <PopoverArrow />
                <PopoverHeader>
                    <HStack spacing={2}>
                        <Checkbox
                            isChecked={filters.noteOnOff}
                            onChange={() => handleFilterChange('noteOnOff')}
                            colorScheme="primary"
                            size="sm"
                            color={getMessageColor('noteOn')}
                        >
                            Note On/Off
                        </Checkbox>
                        <Checkbox
                            isChecked={filters.aftertouch}
                            onChange={() => handleFilterChange('aftertouch')}
                            colorScheme="primary"
                            size="sm"
                            color={getMessageColor('aftertouch')}
                        >
                            Aftertouch
                        </Checkbox>
                    </HStack>
                </PopoverHeader>
                <PopoverBody>
                    <Box height="200px" overflowY="auto">
                        {filteredMessages.map((msg) => formatMessage(msg))}
                    </Box>
                </PopoverBody>
            </PopoverContent>
        </Popover>
    )
}

export default MidiMonitor
