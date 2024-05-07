import { createContext, useState, useEffect } from 'react'
import { useToast } from '@chakra-ui/react'
import { WebMidi } from 'webmidi'
import PropTypes from 'prop-types'

const MidiContext = createContext()

export const MidiProvider = ({ children }) => {
    const toast = useToast()
    const [isDemo, setDemo] = useState(false)
    const [input, setInput] = useState(null)
    const [output, setOutput] = useState(null)
    const [isConnected, setIsConnected] = useState(false)
    const [selectedBank, setSelectedBank] = useState(0)
    const [ccMessages, setCcMessages] = useState([]) // New state variable for CC messages
    const [config, setConfig] = useState({
        version: 0,
        mode: 0,
        midi_trs: 0,
        trs_type: 0,
        passthrough: 0,
        midi_ble: 0,
        brightness: 0,
        banks: [
            {
                ch: 1,
                oct: 0,
                scale: 0,
                note: 0,
                vel: 0,
                at: 0,
                flip_x: 0,
                flip_y: 0,
                chs: [1, 1, 1, 1, 1, 1, 1, 1],
                ids: [13, 14, 15, 16, 17, 18, 19, 20],
            },
            {
                ch: 1,
                oct: 0,
                scale: 0,
                note: 0,
                vel: 0,
                at: 0,
                flip_x: 0,
                flip_y: 0,
                chs: [1, 1, 1, 1, 1, 1, 1, 1],
                ids: [21, 22, 23, 24, 25, 26, 27, 28],
            },
            {
                ch: 1,
                oct: 0,
                scale: 0,
                note: 0,
                vel: 0,
                at: 0,
                flip_x: 0,
                flip_y: 0,
                chs: [1, 1, 1, 1, 1, 1, 1, 1],
                ids: [21, 22, 23, 24, 25, 26, 27, 28],
            },
            {
                ch: 1,
                oct: 0,
                scale: 0,
                note: 0,
                vel: 0,
                at: 0,
                flip_x: 0,
                flip_y: 0,
                chs: [1, 1, 1, 1, 1, 1, 1, 1],
                ids: [31, 32, 33, 34, 35, 36, 37, 38],
            },
        ],
        custom_scale1: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15],
        custom_scale2: [
            0, 3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45,
        ],
    })

    useEffect(() => {
        console.log('Config updated:', config)
    }, [config])

    function onEnabled() {
        // Inputs
        WebMidi.inputs.forEach((input) =>
            console.log(input.manufacturer, input.name)
        )

        // Outputs
        WebMidi.outputs.forEach((output) =>
            console.log(output.manufacturer, output.name)
        )
    }

    const connect = async () => {
        if (isDemo) {
            // If demo mode is enabled, bypass the MIDI device checks
            setIsConnected(true)
            toast({
                title: 'Demo Mode',
                description: 'Demo mode enabled. Using default configuration.',
                status: 'info',
                duration: 3000,
                isClosable: true,
            })
        } else {
            // If demo mode is disabled, proceed with the normal connection process
            await WebMidi.enable({ sysex: true })
                .then(() => console.log('WebMidi with sysex enabled!'))
                .catch((err) => alert(err))
            onEnabled()

            // Get the MIDIAccess object
            const access = await navigator.requestMIDIAccess()

            // Listen for the statechange event on the MIDIAccess object
            access.addEventListener('statechange', (event) => {
                console.log('MIDIAccess state changed:', event.port.state)
                if (event.port.state === 'connected') {
                    // Check if the connected port is the input or output you're looking for
                    if (event.port.name === 'Topo T16') {
                        if (event.port.type === 'input') {
                            setInput(event.port)
                            setIsConnected(true)
                        } else if (event.port.type === 'output') {
                            setOutput(event.port)
                        }
                    }
                } else if (event.port.state === 'disconnected') {
                    // Check if the disconnected port is the input or output you're using
                    if (event.port === input) {
                        setInput(null)
                        setIsConnected(false)
                    } else if (event.port === output) {
                        setOutput(null)
                    }
                    disconnect()
                }
            })

            const _input = WebMidi.getInputByName('Topo T16')
            setInput(_input)
            const _output = WebMidi.getOutputByName('Topo T16')
            setOutput(_output)
            setIsConnected(_input.connection)

            // Add event listener to handle connection state changes
            _input.addListener('statechange', (event) => {
                setIsConnected(event.target.connection) // Update isConnected state
                console.log(
                    'Connection state changed:',
                    event.target.connection
                )
            })

            _input.addListener('sysex', onSysex)
            _input.addListener('controlchange', (e) => {
                console.log(`Received 'controlchange' message.`, e)
                setCcMessages((prevMessages) => {
                    const newMessage = {
                        index: e.controller.number,
                        value: e.value,
                    }

                    // Check if a message with the same index already exists
                    const existingMessageIndex = prevMessages.findIndex(
                        (message) => message.index === newMessage.index
                    )

                    if (existingMessageIndex !== -1) {
                        // If it exists, replace it with the new message
                        return prevMessages.map((message, index) =>
                            index === existingMessageIndex
                                ? newMessage
                                : message
                        )
                    } else {
                        // If it doesn't exist, add the new message to the array
                        return [...prevMessages, newMessage]
                    }
                })
            })

            toast({
                title: 'Connection Successful',
                description: 'MIDI device connected successfully.',
                status: 'success',
                duration: 3000,
                isClosable: true,
            })
        }
    }

    const disconnect = async () => {
        WebMidi.disable()
        setInput(null)
        setOutput(null)
        setIsConnected(false)
        toast({
            title: 'Disconnected',
            description: 'MIDI device disconnected.',
            status: 'error',
            duration: 3000,
            isClosable: true,
        })
    }

    const sendSysex = (data) => {
        if (output) {
            output.sendSysex(data)
        }
    }

    useEffect(() => {
        const requestSysex = () => {
            if (output) {
                console.log('Requesting sysex message.')
                output.sendSysex(0x7e, [0x7f, 0x06, 0x01])
                output.sendSysex(0x7e, [0x7f, 0x07, 0x03])
            } else {
                console.log('Output is not connected.')
            }
        }

        // Call requestSysex after output has been set
        if (output) {
            requestSysex()
        }
    }, [output]) // Added output as dependency for useEffect

    const onSysex = (e) => {
        console.log('Received a sysex message.', e)
        if (e.data[1] === 127 && e.data[2] === 7 && e.data[3] === 4) {
            console.log('Received sysex configuration dump.')
            const deserializedData = deserializeSysex(
                e.data.slice(4, e.data.length - 1)
            )
            setConfig(deserializedData) // If deserializedData is null, keep the current config
            console.log('Deserialized config:', deserializedData) // Print the deserialized data
        }
    }

    const deserializeSysex = (sysex) => {
        const jsonString = sysex
            .map((byte) => String.fromCharCode(byte))
            .join('')
        console.log('JSON string:', jsonString)
        try {
            return JSON.parse(jsonString)
        } catch (error) {
            console.error('Error parsing JSON:', error)
            return null
        }
    }

    const sendConfig = () => {
        if (output) {
            console.log('Sending config dump.')
            const serializedData = JSON.stringify(config)
            console.log('Serialized config:', serializedData)
            const sysex = [127, 7, 5]
            const data = serializedData
                .split('')
                .map((char) => char.charCodeAt(0))
            output.sendSysex(0x7e, [...sysex, ...data])

            toast({
                title: 'Configuration Sent',
                description: 'Configuration successfully sent to the device.',
                status: 'success',
                duration: 3000,
                isClosable: true,
            })
        } else {
            console.log('Output is not connected.')
        }
    }

    const downloadConfig = () => {
        const serializedData = JSON.stringify(config, null, 2)
        const date = new Date().toISOString().slice(0, 10).replace(/-/g, '')
        const filename = `tx16-${date}.topo`
        const blob = new Blob([serializedData], { type: 'application/json' })
        const url = URL.createObjectURL(blob)
        const link = document.createElement('a')
        link.href = url
        link.setAttribute('download', filename)
        link.click()
        URL.revokeObjectURL(url)
    }

    const uploadConfig = () => {
        const input = document.createElement('input')
        input.type = 'file'
        input.accept = '.topo'
        input.onchange = (event) => {
            const file = event.target.files[0]
            const reader = new FileReader()
            reader.onload = (e) => {
                const fileContent = e.target.result
                try {
                    const parsedConfig = JSON.parse(fileContent)
                    setConfig(parsedConfig)
                    toast({
                        title: 'Configuration Loaded',
                        description:
                            'Configuration successfully loaded from file.',
                        status: 'success',
                        duration: 3000,
                        isClosable: true,
                    })
                } catch (error) {
                    console.error('Error parsing configuration file:', error)
                    toast({
                        title: 'Error Loading Configuration',
                        description: 'Failed to parse the configuration file.',
                        status: 'error',
                        duration: 3000,
                        isClosable: true,
                    })
                }
            }
            reader.readAsText(file)
        }
        input.click()
    }

    const contextValue = {
        WebMidi,
        input,
        output,
        connect,
        disconnect,
        isConnected,
        sendSysex,
        ccMessages,
        config,
        sendConfig,
        setConfig,
        selectedBank,
        setSelectedBank,
        isDemo,
        setDemo,
        downloadConfig,
        uploadConfig,
    }

    return (
        <MidiContext.Provider value={contextValue}>
            {children}
        </MidiContext.Provider>
    )
}

MidiProvider.propTypes = {
    children: PropTypes.node.isRequired,
}

export default MidiContext
