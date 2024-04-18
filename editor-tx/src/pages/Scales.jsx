import { Grid, GridItem, Text, VStack } from '@chakra-ui/react'
import { useContext } from 'react'
import MidiContext from '../components/MidiProvider'
import EditableScaleDisplay from '../components/EditableScaleDisplay'

export default function Scales() {
    const { config, setConfig, selectedBank } = useContext(MidiContext)

    const handleNoteChange = (scaleIndex, noteIndex, direction) => {
        const scale =
            scaleIndex === 0 ? config.custom_scale1 : config.custom_scale2
        const newNoteIndex = Math.max(
            0,
            Math.min(scale[noteIndex] + direction, 71)
        )
        const updatedScale = [...scale]
        updatedScale[noteIndex] = newNoteIndex

        setConfig({
            ...config,
            [scaleIndex === 0 ? 'custom_scale1' : 'custom_scale2']:
                updatedScale,
        })
    }
    const octave = config.banks[selectedBank].octave
    const flipX = config.banks[selectedBank].flip_x
    const flipY = config.banks[selectedBank].flip_y

    return (
        <Grid templateColumns="repeat(2, 1fr)" gap={6} p={5} px="2vw">
            <GridItem colSpan={{ base: 2, lg: 1 }}>
                <VStack spacing={4} pr={{ base: 0, lg: 10 }}>
                    <Text fontWeight="bold">Custom Scale 1</Text>
                    <EditableScaleDisplay
                        scale={config.custom_scale1}
                        octave={octave}
                        flipX={flipX}
                        flipY={flipY}
                        onNoteChange={(noteIndex, direction) =>
                            handleNoteChange(0, noteIndex, direction)
                        }
                    />
                </VStack>
            </GridItem>
            <GridItem colSpan={{ base: 2, lg: 1 }}>
                <VStack spacing={4} pl={{ base: 0, lg: 10 }}>
                    <Text fontWeight="bold">Custom Scale 2</Text>
                    <EditableScaleDisplay
                        scale={config.custom_scale2}
                        octave={octave}
                        flipX={flipX}
                        flipY={flipY}
                        onNoteChange={(noteIndex, direction) =>
                            handleNoteChange(1, noteIndex, direction)
                        }
                    />
                </VStack>
            </GridItem>
        </Grid>
    )
}
