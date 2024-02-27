import { Grid, GridItem } from '@chakra-ui/react'
import { CcCard } from '../components/CcCard'
import { useContext } from 'react'
import MidiContext from '../components/MidiProvider'

export default function ControlChange() {
    const { config } = useContext(MidiContext)

    return (
        <Grid templateColumns="repeat(4, 1fr)" p={10}>
            <GridItem colSpan={1} p={2}>
                <CcCard
                    name="X"
                    channel={config.channels[0]}
                    cc={config.cc_ids[0]}
                    value={0}
                />
            </GridItem>
            <GridItem colSpan={1} p={2}>
                <CcCard
                    name="Y"
                    channel={config.channels[1]}
                    cc={config.cc_ids[1]}
                    value={0}
                />
            </GridItem>
            <GridItem colSpan={1} p={2}>
                <CcCard
                    name="P"
                    channel={config.channels[2]}
                    cc={config.cc_ids[2]}
                    value={0}
                />
            </GridItem>
            <GridItem colSpan={1} p={2}>
                <CcCard
                    name={'MOD'}
                    channel={config.channels[7]}
                    cc={config.cc_ids[7]}
                    value={0}
                />
            </GridItem>
            <GridItem colSpan={1} p={2}>
                <CcCard
                    name={'A'}
                    channel={config.channels[3]}
                    cc={config.cc_ids[3]}
                    value={0}
                />
            </GridItem>
            <GridItem colSpan={1} p={2}>
                <CcCard
                    name={'B'}
                    channel={config.channels[4]}
                    cc={config.cc_ids[4]}
                    value={0}
                />
            </GridItem>
            <GridItem colSpan={1} p={2}>
                <CcCard
                    name={'C'}
                    channel={config.channels[5]}
                    cc={config.cc_ids[5]}
                    value={0}
                />
            </GridItem>
            <GridItem colSpan={1} p={2}>
                <CcCard
                    name={'D'}
                    channel={config.channels[6]}
                    cc={config.cc_ids[6]}
                    value={0}
                />
            </GridItem>
        </Grid>
    )
}
