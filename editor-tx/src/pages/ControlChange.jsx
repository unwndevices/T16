import { Grid, GridItem } from '@chakra-ui/react'
import { CcCard } from '../components/CcCard'
import { useContext } from 'react'
import MidiContext from '../components/MidiProvider'

export default function ControlChange() {
    const { config } = useContext(MidiContext)

    return (
        <Grid templateColumns="repeat(4, 1fr)" px={10} py={3}>
            <GridItem colSpan={1} p={1}>
                <CcCard
                    name="X"
                    channel={config.banks[0].channels[0]}
                    cc={config.banks[0].ids[0]}
                    value={0}
                />
            </GridItem>
            <GridItem colSpan={1} p={1}>
                <CcCard
                    name="Y"
                    channel={config.banks[0].channels[1]}
                    cc={config.banks[0].ids[1]}
                    value={0}
                />
            </GridItem>
            <GridItem colSpan={1} p={1}>
                <CcCard
                    name="P"
                    channel={config.banks[0].channels[2]}
                    cc={config.banks[0].ids[2]}
                    value={0}
                />
            </GridItem>
            <GridItem colSpan={1} p={1}>
                <CcCard
                    name={'MOD'}
                    channel={config.banks[0].channels[7]}
                    cc={config.banks[0].ids[7]}
                    value={0}
                />
            </GridItem>
            <GridItem colSpan={1} p={1}>
                <CcCard
                    name={'A'}
                    channel={config.banks[0].channels[3]}
                    cc={config.banks[0].ids[3]}
                    value={0}
                />
            </GridItem>
            <GridItem colSpan={1} p={1}>
                <CcCard
                    name={'B'}
                    channel={config.banks[0].channels[4]}
                    cc={config.banks[0].ids[4]}
                    value={0}
                />
            </GridItem>
            <GridItem colSpan={1} p={1}>
                <CcCard
                    name={'C'}
                    channel={config.banks[0].channels[5]}
                    cc={config.banks[0].ids[5]}
                    value={0}
                />
            </GridItem>
            <GridItem colSpan={1} p={1}>
                <CcCard
                    name={'D'}
                    channel={config.banks[0].channels[6]}
                    cc={config.banks[0].ids[6]}
                    value={0}
                />
            </GridItem>
        </Grid>
    )
}
