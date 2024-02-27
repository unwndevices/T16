import { Grid, GridItem } from '@chakra-ui/react'
import Chart from 'react-apexcharts'
import { SliderCard } from '../components/SliderCard'
import { SelectCard } from '../components/SelectCard'

export default function Dashboard() {
    return (
        <Grid templateColumns="repeat(4, 1fr)" p={10}>
            <GridItem colSpan={1} p={2}>
                <SelectCard
                    name="Mode"
                    entries={['General', 'Pad', 'Sliders']}
                />
            </GridItem>
            <GridItem colSpan={1} p={2}>
                <SliderCard name="Brightness" value={50} min={10} max={254} />
            </GridItem>
        </Grid>
    )
}
