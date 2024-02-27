import { Grid, GridItem } from '@chakra-ui/react'
import { SelectCard } from '../components/SelectCard'
import { SliderCard } from '../components/SliderCard'
import { AliasNumberCard, NumberCard } from '../components/NumberCard'

export default function General() {
    return (
        <Grid templateColumns="repeat(4, 1fr)" p={10}>
            <GridItem colSpan={1} p={2}>
                <SelectCard
                    name="Default Mode"
                    entries={['Keyboard', 'Pad', 'Sliders']}
                />
            </GridItem>
            <GridItem colSpan={1} p={2}>
                <SliderCard name="Brightness" value={50} min={10} max={254} />
            </GridItem>
            <GridItem colSpan={1} p={2}>
                <NumberCard name="Global Channel" min={1} max={16} />
            </GridItem>
            <GridItem colSpan={1} p={2}>
                <NumberCard name="Base Octave" min={0} max={5} />
            </GridItem>
            <GridItem colSpan={1} p={2}>
                <AliasNumberCard
                    name="Root Note"
                    aliases={[
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
                    ]}
                />
            </GridItem>
            <GridItem colSpan={1} p={2}>
                <SelectCard
                    name="Velocity Curve"
                    entries={['Linear', 'Exponential', 'Logarithmic', 'Cubic']}
                />
            </GridItem>
        </Grid>
    )
}
