import { createContext, useState } from 'react'
import { PropTypes } from 'prop-types'

const SerialContext = createContext()

export const SerialProvider = ({ children }) => {
    const [port, setPort] = useState(null)
    const [inputStream, setInputStream] = useState(null)
    const [outputStream, setOutputStream] = useState(null)
    const [isConnected, setIsConnected] = useState(false)

    const connect = async () => {
        try {
            const newPort = await navigator.serial.requestPort()
            await newPort.open({ baudRate: 115200 })

            let decoder = new TextDecoderStream()
            let encoder = new TextEncoderStream()

            setInputStream(newPort.readable.pipeThrough(decoder).readable)
            setOutputStream(encoder.writable)

            setPort(newPort)
            setIsConnected(true)
        } catch (error) {
            console.error('Error connecting:', error)
            setIsConnected(false)
        }
    }

    const disconnect = async () => {
        try {
            if (port) {
                await port.close()
                setPort(null)
                setIsConnected(false)
            }
        } catch (error) {
            console.error('Error disconnecting:', error)
        }
    }

    const read = async () => {
        try {
            if (!port) {
                throw new Error('No port connected')
            }

            const reader = inputStream.getReader()
            const { value, done } = await reader.read()
            reader.releaseLock()

            if (done) {
                return null
            }

            return value
        } catch (error) {
            console.error('Error reading:', error)
            return null
        }
    }

    const write = async (data) => {
        try {
            if (!port) {
                throw new Error('No port connected')
            }

            const writer = outputStream.getWriter()
            await writer.write(data + '\r')
            writer.releaseLock()
        } catch (error) {
            console.error('Error writing:', error)
        }
    }
    // Context value with the updated isConnected state
    const contextValue = {
        connect,
        disconnect,
        read,
        write,
        isConnected,
    }

    return (
        <SerialContext.Provider value={contextValue}>
            {children}
        </SerialContext.Provider>
    )
}

SerialProvider.propTypes = {
    children: PropTypes.node.isRequired,
}

export default SerialContext
