import { Box, HStack, Select, Text } from '@chakra-ui/react'
import { PropTypes } from 'prop-types'
import SkeletonLoader from './SkeletonLoader'

export function SelectCard({ name, value, entries, onChange }) {
    const handleChange = (event) => {
        const index = entries.indexOf(event.target.value)
        onChange(index)
    }

    return (
        <HStack justifyContent="space-between" alignItems="center">
            <Box minW="8vw">
                <Text>{name}</Text>
            </Box>
            <SkeletonLoader>
                <Select
                    backgroundColor="white"
                    value={entries[value]} // Set the selected value based on the index
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
}
