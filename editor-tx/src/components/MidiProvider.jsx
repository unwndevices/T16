import { createContext, useState, useEffect } from 'react'
import { WebMidi } from 'webmidi'
import PropTypes from 'prop-types'

const MidiContext = createContext()

export const MidiProvider = ({ children }) => {
    const [input, setInput] = useState(null)
    const [output, setOutput] = useState(null)
    const [isConnected, setIsConnected] = useState(false)
    const [ccMessages, setCcMessages] = useState([]) // New state variable for CC messages
    const [config, setConfig] = useState({}) // Initialize config as an empty object

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
        await WebMidi.enable({ sysex: true })
            .then(() => console.log('WebMidi with sysex enabled!'))
            .catch((err) => alert(err))
        onEnabled()
        const _input = WebMidi.getInputByName('unwn TX16')
        setInput(_input)
        const _output = WebMidi.getOutputByName('unwn TX16')
        setOutput(_output)
        setIsConnected(_input.connection)
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
                        index === existingMessageIndex ? newMessage : message
                    )
                } else {
                    // If it doesn't exist, add the new message to the array
                    return [...prevMessages, newMessage]
                }
            })
        })
    }

    const disconnect = async () => {
        WebMidi.disable()
        setInput(null)
        setOutput(null)
        setIsConnected(false)
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
            setConfig(deserializedData) // Set the config dump state variable
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
        } else {
            console.log('Output is not connected.')
        }
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
