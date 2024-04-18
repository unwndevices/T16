import { Box, HStack, Text } from '@chakra-ui/react'
import { PropTypes } from 'prop-types'
import { NumberInput, AliasNumberInput } from './CustomInputs'
import SkeletonLoader from './SkeletonLoader'

export function NumberCard({ name, min = 0, max = 127, value, onChange }) {
    const handleSelect = (value) => {
        onChange(value)
    }

    return (
        <HStack justifyContent="space-between" alignItems="center">
            <Box minW="8vw">
                <Text>{name}</Text>
            </Box>
            <SkeletonLoader>
                <NumberInput
                    min={min}
                    max={max}
                    value={value} // Pass the value prop here
                    onSelect={handleSelect}
                    
                />
            </SkeletonLoader>
        </HStack>
    )
}

NumberCard.propTypes = {
    name: PropTypes.string,
    min: PropTypes.number,
    max: PropTypes.number,
    value: PropTypes.number,
    onChange: PropTypes.func,
}

export function AliasNumberCard({ name, aliases, value, onChange }) {
    const handleSelect = (index) => {
        onChange(index)
    }

    return (
        <HStack justifyContent="space-between" alignItems="baseline">
            <Text>{name}</Text>
            <SkeletonLoader>
                <AliasNumberInput
                    aliases={aliases}
                    value={value}
                    onSelect={handleSelect}
                />
            </SkeletonLoader>
        </HStack>
    )
}

AliasNumberCard.propTypes = {
    name: PropTypes.string,
    aliases: PropTypes.array,
    value: PropTypes.number,
    onChange: PropTypes.func,
}
