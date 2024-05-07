import { Grid, GridItem } from '@chakra-ui/react'
import { CcCard } from '../components/CcCard'
import { useContext } from 'react'
import MidiContext from '../components/MidiProvider'

const ccNames = ['X', 'Y', 'Z', 'M', 'A', 'B', 'C', 'D']
const checkForDuplicateCcs = (config, bankIndex) => {
    const duplicates = new Set()
    const seen = new Map()

    config.banks[bankIndex].chs.forEach((channel, index) => {
        const cc = config.banks[bankIndex].ids[index]
        const key = `${channel}-${cc}`
        if (seen.has(key)) {
            duplicates.add(index) // Add the current duplicate index
            seen.get(key).forEach((dupIndex) => duplicates.add(dupIndex)) // Add all previous occurrences
        } else {
            seen.set(key, [index]) // Store the index in an array to handle multiple duplicates
        }
    })

    return duplicates
}

export default function ControlChange() {
    const { config, setConfig, selectedBank } = useContext(MidiContext)
    const bankIndex = selectedBank
    const duplicateCcs = checkForDuplicateCcs(config, bankIndex)

    const handleChange = (bankIndex, cardIndex, changes) => {
        setConfig((prevConfig) => {
            const updatedBanks = [...prevConfig.banks]
            const updatedBank = { ...updatedBanks[bankIndex] }

            if (changes.channel !== undefined) {
                updatedBank.chs[cardIndex] = changes.channel
            }
            if (changes.cc !== undefined) {
                updatedBank.ids[cardIndex] = changes.cc
            }

            updatedBanks[bankIndex] = updatedBank

            return { ...prevConfig, banks: updatedBanks }
        })
    }

    return (
        <Grid templateColumns="repeat(4, 1fr)" px={10} py={3}>
            {config.banks[0].chs.map((channel, index) => (
                <GridItem colSpan={1} p={1} key={index}>
                    <CcCard
                        name={ccNames[index]} // X, Y, P, etc.
                        channel={config.banks[0].chs[index]}
                        cc={config.banks[0].ids[index]}
                        value={0} // Assuming value is managed elsewhere or static
                        onChange={(changes) => handleChange(0, index, changes)}
                        isDuplicate={duplicateCcs.has(index)} // Pass isDuplicate prop
                    />
                </GridItem>
            ))}
        </Grid>
    )
}
