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
    onNoteChange,
}) {
    const getNoteInfoForIndex = (index) => {
        let x = index % 4
        let y = Math.floor(index / 4)
        if (flipX) x = 3 - x
        if (flipY) y = 3 - y
        const flippedIndex = y * 4 + x
        const noteIndex = scale[flippedIndex]
        const noteName = noteNames[noteIndex % 12]
        const octave = Math.floor(noteIndex / 12)
        const isRoot = noteIndex === 0
        return { noteName, octave, isRoot }
    }

    return (
        <Box>
            <Grid templateColumns="repeat(4, 1fr)" gap={2}>
                {Array.from({ length: 16 }, (_, index) => {
                    const { noteName, octave, isRoot } =
                        getNoteInfoForIndex(index)
                    return (
                        <GridItem key={index}>
                            <EditableKeyCard
                                name={`Key ${index + 1}`}
                                output={`${noteName}${octave}`}
                                root={isRoot}
                                onDecrement={() => onNoteChange(index, -1)}
                                onIncrement={() => onNoteChange(index, 1)}
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
    flipX: PropTypes.number,
    flipY: PropTypes.number,
    onNoteChange: PropTypes.func.isRequired,
}
