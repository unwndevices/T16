import * as React from 'react'
import { ChakraProvider } from '@chakra-ui/react'
import * as ReactDOM from 'react-dom/client'
import App from './App.jsx'

import { MidiProvider } from './components/MidiProvider.jsx'

const rootElement = document.getElementById('root')
ReactDOM.createRoot(rootElement).render(
    <ChakraProvider>
        <MidiProvider>
            <React.StrictMode>
                <App />
            </React.StrictMode>
        </MidiProvider>
    </ChakraProvider>
)
