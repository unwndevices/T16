import { Divider, Grid, GridItem, HStack, Stack } from '@chakra-ui/react'
import { ToggleCard } from './../components/ToggleCard'
import { AliasNumberCard, NumberCard } from '../components/NumberCard'
import { SelectCard } from '../components/SelectCard'
import { useContext, useEffect } from 'react'
import MidiContext from '../components/MidiProvider'
import ScaleDisplay from '../components/ScaleDisplay'

const scales = [
    {
        name: 'Chromatic',
        steps: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11],
    },
    {
        name: 'Ionian',
        steps: [0, 2, 4, 5, 7, 9, 11],
    },
    {
        name: 'Dorian',
        steps: [0, 2, 3, 5, 7, 9, 10],
    },
    {
        name: 'Phrygian',
        steps: [0, 1, 3, 5, 7, 8, 10],
    },
    {
        name: 'Lydian',
        steps: [0, 2, 4, 6, 7, 9, 11],
    },
    {
        name: 'Mixolydian',
        steps: [0, 2, 4, 5, 7, 9, 10],
    },
    {
        name: 'Aeolian',
        steps: [0, 2, 3, 5, 7, 8, 10],
    },
    {
        name: 'Locrian',
        steps: [0, 1, 3, 5, 6, 8, 10],
    },
    {
        name: 'Major Pentatonic',
        steps: [0, 2, 4, 7, 9],
    },
    {
        name: 'Minor Pentatonic',
        steps: [0, 3, 5, 7, 10],
    },
    {
        name: 'Blues',
        steps: [0, 3, 5, 6, 7, 10],
    },
    {
        name: 'Whole Tone',
        steps: [0, 2, 4, 6, 8, 10],
    },
    {
        name: 'Diminished',
        steps: [0, 2, 3, 5, 6, 8, 9, 11],
    },
    {
        name: 'Augmented',
        steps: [0, 3, 4, 7, 8, 11],
    },
    {
        name: 'Harmonic Minor',
        steps: [0, 2, 3, 5, 7, 8, 11],
    },
    {
        name: 'Melodic Minor',
        steps: [0, 2, 3, 5, 7, 9, 11],
    },
    {
        name: 'Japanese',
        steps: [0, 1, 5, 7, 8],
    },
]

const noteNames = [
    'C',
    'C#',
    'D',
    'D#',
    'E',
    'F',
    'F#',
    'G',
    'G#',
    'A',
    'A#',
    'B',
]

export default function Keyboard() {
    const { config, setConfig, selectedBank, syncStatus } =
        useContext(MidiContext)

    // Create an array of scale objects including the custom scales
    const allScales = [
        ...scales,
        {
            name: 'Custom Scale 1',
            steps: config.custom_scale1,
        },
        {
            name: 'Custom Scale 2',
            steps: config.custom_scale2,
        },
    ]

    const scaleNames = allScales.map((scale) => scale.name)

    const handleChange = (id, value) => {
        setConfig((prevConfig) => ({
            ...prevConfig,
            banks: prevConfig.banks.map((bank, index) => {
                if (index === selectedBank) {
                    // Update the configuration of the selected bank
                    return {
                        ...bank,
                        [id]: value,
                    }
                }
                return bank // Return unchanged for other banks
            }),
        }))
    }

    const currentScale = config.banks[selectedBank].scale
    const currentRoot = config.banks[selectedBank].note
    const currentOctave = config.banks[selectedBank].oct
    const flipX = config.banks[selectedBank].flip_x
    const flipY = config.banks[selectedBank].flip_y

    useEffect(() => {
        const currentBank = config.banks[selectedBank]
        if (currentBank.scale !== 0 && currentBank.koala_mode !== 0) {
            handleChange('koala_mode', 0)
        }
    })

    return (
        <Grid templateColumns="repeat(5, 1fr)" p={5} px="2vw">
            <GridItem colSpan={{ base: 5, lg: 3 }}>
                <Stack
                    spacing={{ base: 6, lg: 3 }}
                    pr={{ base: 0, lg: 10 }}
                    py={3}
                >
                    <SelectCard
                        name="Scale"
                        entries={scaleNames}
                        value={config.banks[selectedBank].scale}
                        onChange={(value) => handleChange('scale', value)}
                        isSynced={syncStatus.banks[selectedBank].scale}
                    />
                    <Divider />
                    <AliasNumberCard
                        name="Root Note"
                        aliases={noteNames}
                        value={config.banks[selectedBank].note}
                        onChange={(value) => handleChange('note', value)}
                        isSynced={syncStatus.banks[selectedBank].note}
                    />
                    <Divider />
                    <NumberCard
                        name="Octave"
                        min={0}
                        max={5}
                        value={config.banks[selectedBank].oct}
                        onChange={(value) => handleChange('oct', value)}
                        isSynced={syncStatus.banks[selectedBank].oct}
                    />
                    <Divider />
                    <HStack justifyContent="space-between">
                        <ToggleCard
                            name="Flip X"
                            id="flip_x"
                            value={config.banks[selectedBank].flip_x}
                            onChange={(value) => handleChange('flip_x', value)}
                            isSynced={syncStatus.banks[selectedBank].flip_x}
                        />
                        <ToggleCard
                            name="Flip Y"
                            id="flip_y"
                            value={config.banks[selectedBank].flip_y}
                            onChange={(value) => handleChange('flip_y', value)}
                            isSynced={syncStatus.banks[selectedBank].flip_y}
                        />
                    </HStack>
                    <Divider />
                    <NumberCard
                        name="Channel"
                        min={1}
                        max={16}
                        value={config.banks[selectedBank].ch}
                        onChange={(value) => handleChange('ch', value)}
                        isSynced={syncStatus.banks[selectedBank].ch}
                    />
                    <Divider />
                    <SelectCard
                        name="Velocity Curve"
                        entries={[
                            'Linear',
                            'Exponential',
                            'Logarithmic',
                            'Cubic',
                        ]}
                        value={config.banks[selectedBank].vel}
                        onChange={(value) => handleChange('vel', value)}
                        isSynced={syncStatus.banks[selectedBank].vel}
                    />
                    <Divider />
                    <SelectCard
                        name="Aftertouch Curve"
                        entries={[
                            'Linear',
                            'Exponential',
                            'Logarithmic',
                            'Cubic',
                        ]}
                        value={config.banks[selectedBank].at}
                        onChange={(value) => handleChange('at', value)}
                        isSynced={syncStatus.banks[selectedBank].at}
                    />
                    <Divider />
                    <ToggleCard
                        name="Koala Mode"
                        id="koala_mode"
                        value={config.banks[selectedBank].koala_mode}
                        onChange={(value) => handleChange('koala_mode', value)}
                        isSynced={syncStatus.banks[selectedBank].koala_mode}
                        isDisabled={config.banks[selectedBank].scale !== 0}
                        tooltipText={
                            'Koala mode is only available when using the Chromatic scale. Turns the octave slider into a "page" slider.'
                        }
                    />
                </Stack>
            </GridItem>
            <GridItem colSpan={{ base: 5, lg: 2 }} pl={{ base: 0, lg: 10 }}>
                <ScaleDisplay
                    scales={allScales}
                    selectedRoot={currentRoot}
                    selectedScale={currentScale}
                    octave={currentOctave}
                    flipX={flipX}
                    flipY={flipY}
                />
            </GridItem>
        </Grid>
    )
}
