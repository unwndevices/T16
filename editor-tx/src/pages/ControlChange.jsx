import { Grid, GridItem } from '@chakra-ui/react'
import { CcCard } from '../components/CcCard'
import { useContext, useMemo } from 'react'
import MidiContext from '../components/MidiProvider'

//object config for the 16 keys, each key has a CC number and a channel

const keys = [
    { cc: 0, channel: 1, value: 0 },
    { cc: 1, channel: 1, value: 10 },
    { cc: 2, channel: 1, value: 22 },
    { cc: 3, channel: 1, value: 18 },
    { cc: 4, channel: 1, value: 45 },
    { cc: 5, channel: 1, value: 100 },
    { cc: 6, channel: 1, value: 2 },
    { cc: 7, channel: 1, value: 90 },
    { cc: 8, channel: 1, value: 4 },
    { cc: 9, channel: 1, value: 10 },
    { cc: 10, channel: 1, value: 8 },
    { cc: 11, channel: 1, value: 2 },
    { cc: 12, channel: 1, value: 10 },
    { cc: 13, channel: 1, value: 87 },
    { cc: 14, channel: 1, value: 10 },
    { cc: 15, channel: 1, value: 62 },
]

export default function ControlChange() {
    const { ccMessages } = useContext(MidiContext)

    const cc13ValueMemo = useMemo(() => {
        return Math.round(
            (ccMessages.find((message) => message.index === 13)?.value || 0) *
                100
        )
    }, [ccMessages])

    const cc14ValueMemo = useMemo(() => {
        return Math.round(
            (ccMessages.find((message) => message.index === 14)?.value || 0) *
                100
        )
    }, [ccMessages])

    const cc15ValueMemo = useMemo(() => {
        return Math.round(
            (ccMessages.find((message) => message.index === 15)?.value || 0) *
                100
        )
    }, [ccMessages])

    return (
        <Grid templateColumns="repeat(6, 1fr)" p={10}>
            <GridItem colSpan={2}>
                <GridItem colSpan={2} p={2}>
                    <CcCard
                        name="X"
                        channel={1}
                        cc={13}
                        value={cc13ValueMemo}
                    />
                </GridItem>
                <GridItem colSpan={2} p={2}>
                    <CcCard
                        name="Y"
                        channel={1}
                        cc={14}
                        value={cc14ValueMemo}
                    />
                </GridItem>
                <GridItem colSpan={2} p={2}>
                    <CcCard
                        name="P"
                        channel={1}
                        cc={15}
                        value={cc15ValueMemo}
                    />
                </GridItem>
            </GridItem>
            <GridItem colSpan={4}>
                <Grid templateColumns="repeat(4, 1fr)">
                    {keys.map((key, index) => (
                        <GridItem colSpan={1} key={key.cc} p={2}>
                            <CcCard
                                name={`${index + 1}`}
                                channel={key.channel}
                                cc={key.cc}
                                value={key.value}
                            />
                        </GridItem>
                    ))}
                </Grid>
            </GridItem>
        </Grid>
    )
}
