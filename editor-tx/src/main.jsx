import * as React from 'react'
import { ChakraProvider, extendTheme } from '@chakra-ui/react'
import * as ReactDOM from 'react-dom/client'
import App from './App.jsx'

import { MidiProvider } from './components/MidiProvider.jsx'

const colors = {}

const fonts = {}

const theme = extendTheme({
    colors,
    fonts,
})

const rootElement = document.getElementById('root')
ReactDOM.createRoot(rootElement).render(
    <ChakraProvider theme={theme}>
        <MidiProvider>
            <React.StrictMode>
                <App />
            </React.StrictMode>
        </MidiProvider>
    </ChakraProvider>
)
