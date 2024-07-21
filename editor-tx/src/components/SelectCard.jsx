import { Box, HStack, Select, Text, Circle } from '@chakra-ui/react'
import { PropTypes } from 'prop-types'
import SkeletonLoader from './SkeletonLoader'

export function SelectCard({ name, value, entries, onChange, isSynced }) {
    const handleChange = (event) => {
        const index = entries.indexOf(event.target.value)
        onChange(index)
    }

    const getDotColor = () => {
        return isSynced ? 'green.500' : 'red.500'
    }

    return (
        <HStack justifyContent="space-between" alignItems="center">
            <Box minW="8vw">
                <HStack spacing={2}>
                    <Circle size="10px" bg={getDotColor()} />
                    <Text>{name}</Text>
                </HStack>
            </Box>
            <SkeletonLoader>
                <Select
                    backgroundColor="white"
                    value={entries[value]}
                    onChange={handleChange}
                >
                    {entries.map((entry) => (
                        <option key={entry} value={entry}>
                            {entry}
                        </option>
                    ))}
                </Select>
            </SkeletonLoader>
        </HStack>
    )
}

SelectCard.propTypes = {
    name: PropTypes.string,
    value: PropTypes.number,
    entries: PropTypes.array,
    onChange: PropTypes.func,
    isSynced: PropTypes.bool,
}
