import { Grid, GridItem } from '@chakra-ui/react'
import { SelectCard } from '../components/SelectCard'
import { NumberCard } from '../components/NumberCard'

export default function Keyboard() {
    return (
        <Grid templateColumns="repeat(4, 1fr)" p={10}>
            <GridItem colSpan={1} p={2}>
                <NumberCard name="Channel" min={1} max={16} />
            </GridItem>
            <GridItem colSpan={1} p={2}>
                <NumberCard name="Octave" min={1} max={8} />
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
