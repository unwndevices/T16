import '@fontsource/inter/index.css'
import '@fontsource/poppins/index.css'
import * as React from 'react'
import { ChakraProvider, extendTheme } from '@chakra-ui/react'
import * as ReactDOM from 'react-dom/client'
import App from './App.jsx'

import { MidiProvider } from './components/MidiProvider.jsx'

const colors = {
    black: '#100f10',
    gray: {
        50: '#faf9fe',
        100: '#f2f0fd',
        200: '#e8e6f3',
        300: '#d4d2df',
        400: '#adacb6',
        500: '#7f7e85',
        600: '#555459',
        700: '#373639',
        800: '#202022',
        900: '#19191b',
    },
    purple: {
        50: '#f9f5fe',
        100: '#e8d9fb',
        200: '#d7bcf8',
        300: '#be91f4',
        400: '#ab73f1',
        500: '#9247ed',
        600: '#7c2bdf',
        700: '#6623b8',
        800: '#541d97',
        900: '#3f1671',
    },
    red: {
        50: '#fef5f4',
        100: '#fbd9d4',
        200: '#f7b6ad',
        300: '#f2887b',
        400: '#ef6c5a',
        500: '#e4412c',
        600: '#c13725',
        700: '#9b2d1e',
        800: '#842619',
        900: '#601c12',
    },
    teal: {
        50: '#f0fcfe',
        100: '#bdf1f8',
        200: '#7ce4f2',
        300: '#2cd0e5',
        400: '#26b1c3',
        500: '#2096a6',
        600: '#1a7a87',
        700: '#145f69',
        800: '#114f57',
        900: '#0e4148',
    },
    primary: {
        50: '#ded9fb',
        100: '#cfc6f9',
        200: '#bfb4f6',
        300: '#b0a2f4',
        400: '#a08ff2',
        500: '#917df0',
        600: '#816bee',
        700: '#7258ec',
        800: '#6246ea',
        900: '#5234e8',
    },
    secondary: {
        50: '#fee9fa',
        100: '#fcd1f5',
        200: '#fbb8ef',
        300: '#faa0ea',
        400: '#f888e4',
        500: '#f770df',
        600: '#f658da',
        700: '#f440d4',
        800: '#f328cf',
        900: '#f20fc9',
    },
    accent: {
        50: '#f2f968',
        100: '#f0f855',
        200: '#eef741',
        300: '#ecf62d',
        400: '#eaf51a',
        500: '#e5f10a',
        600: '#d3dd09',
        700: '#c0ca08',
        800: '#adb608',
        900: '#9ba207',
    },
}

const theme = extendTheme({
    colors,
    fonts: {
        heading: "'Poppins', sans-serif",
        body: "'Inter', sans-serif",
        mono: 'Menlo, monospace', // For code blocks
    },

    fontSizes: {
        xs: '0.75rem',
        sm: '0.875rem',
        md: '1rem',
        lg: '1.125rem',
        xl: '1.25rem',
        '2xl': '1.5rem',
        '3xl': '1.875rem',
        '4xl': '2.25rem',
        '5xl': '3rem',
        '6xl': '3.75rem',
        '7xl': '4.5rem',
        '8xl': '6rem',
        '9xl': '8rem',
    },

    fontWeights: {
        hairline: 100,
        thin: 200,
        light: 300,
        normal: 400,
        medium: 500,
        semibold: 600,
        bold: 700,
        extrabold: 800,
        black: 900,
    },
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
