import { Grid, GridItem } from '@chakra-ui/react'
import KeyCard from './KeyCard'
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

export default function ScaleDisplay({
    scales,
    selectedScale,
    selectedRoot,
    octave,
    flipX,
    flipY,
}) {
    const selectedScaleSteps = scales[selectedScale].steps
    let previousNoteIndex = -1
    let displayOctave = octave
    // Helper function to get the note name, root flag, and octave for a given index
    const getNoteInfoForIndex = (index) => {
        let x = index % 4
        let y = Math.floor(index / 4)

        // Flip the x and y coordinates based on flipX and flipY props
        if (flipX) {
            x = 3 - x
        }
        if (flipY) {
            y = 3 - y
        }

        // Calculate the correct index based on flipped x and y coordinates
        const flippedIndex = y * 4 + x
        const noteIndex =
            selectedScaleSteps[flippedIndex % selectedScaleSteps.length]
        const noteNameIndex = (noteIndex + selectedRoot) % 12
        const noteName = noteNames[noteNameIndex]
        const isRoot =
            flippedIndex % selectedScaleSteps.length === 0 ||
            noteNameIndex % 12 === 0
        // Calculate the octave based on the note index and the passed octave
        if (noteNameIndex <= previousNoteIndex) {
            displayOctave += 1
        }
        previousNoteIndex = noteNameIndex

        return { noteName, isRoot, displayOctave }
    }

    return (
        <Grid templateColumns="repeat(4, 1fr)" gap={2}>
            {Array.from({ length: 16 }, (_, index) => {
                const { noteName, isRoot, displayOctave } =
                    getNoteInfoForIndex(index)
                return (
                    <GridItem key={index}>
                        <KeyCard
                            name={`Key ${index + 1}`}
                            output={`${noteName}${displayOctave}`}
                            root={isRoot}
                        />
                    </GridItem>
                )
            })}
        </Grid>
    )
}
ScaleDisplay.propTypes = {
    scales: PropTypes.arrayOf(
        PropTypes.shape({
            name: PropTypes.string.isRequired,
            steps: PropTypes.arrayOf(PropTypes.number).isRequired,
        })
    ).isRequired,
    selectedScale: PropTypes.number.isRequired,
    selectedRoot: PropTypes.number.isRequired,
    octave: PropTypes.number.isRequired,
    flipX: PropTypes.number,
    flipY: PropTypes.number,
}
