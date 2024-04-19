import { Grid, GridItem, Box } from '@chakra-ui/react'
import { EditableKeyCard } from './KeyCard'
import { PropTypes } from 'prop-types'

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

export default function EditableScaleDisplay({
    scale,
    flipX,
    flipY,
    octave,
    root,
    onNoteChange,
}) {
    const getNoteInfoForIndex = (index) => {
        let x = index % 4
        let y = Math.floor(index / 4)
        if (flipX) x = 3 - x
        if (flipY) y = 3 - y
        const flippedIndex = y * 4 + x
        const noteIndex = scale[flippedIndex]
        const noteName = noteNames[(noteIndex + root) % 12]
        const calculatedOctave = Math.floor(noteIndex / 12)
        const isRoot = noteIndex === 0 || noteIndex % 12 === 0
        return { noteName, noteIndex, calculatedOctave, isRoot }
    }

    return (
        <Box>
            <Grid templateColumns="repeat(4, 1fr)" gap={4}>
                {Array.from({ length: 16 }, (_, index) => {
                    const { noteName, noteIndex, calculatedOctave, isRoot } =
                        getNoteInfoForIndex(index)
                    const displayOctave = octave + calculatedOctave
                    return (
                        <GridItem key={index}>
                            <EditableKeyCard
                                name={`Key ${index + 1}`}
                                index={noteIndex}
                                output={`${noteName}${displayOctave}`}
                                isRoot={isRoot}
                                onDecrement={() => onNoteChange(index, -1)}
                                onIncrement={() => onNoteChange(index, 1)}
                                onIndexChange={(index) =>
                                    onNoteChange(index, 0)
                                }
                            />
                        </GridItem>
                    )
                })}
            </Grid>
        </Box>
    )
}

EditableScaleDisplay.propTypes = {
    scale: PropTypes.arrayOf(PropTypes.number).isRequired,
    octave: PropTypes.number.isRequired,
    root: PropTypes.number.isRequired,
    flipX: PropTypes.number,
    flipY: PropTypes.number,
    onNoteChange: PropTypes.func.isRequired,
}
