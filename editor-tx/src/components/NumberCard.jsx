import { Box, Circle, HStack, Text } from '@chakra-ui/react'
import { PropTypes } from 'prop-types'
import { NumberInput, AliasNumberInput } from './CustomInputs'
import SkeletonLoader from './SkeletonLoader'

export function NumberCard({
    name,
    min = 0,
    max = 127,
    value,
    onChange,
    isSynced,
}) {
    const handleSelect = (value) => {
        onChange(value)
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
    isSynced: PropTypes.bool,
}

export function AliasNumberCard({ name, aliases, value, onChange, isSynced }) {
    const handleSelect = (index) => {
        onChange(index)
    }

    const getDotColor = () => {
        return isSynced ? 'green.500' : 'red.500'
    }

    return (
        <HStack justifyContent="space-between" alignItems="baseline">
            <HStack spacing={2}>
                <Circle size="10px" bg={getDotColor()} />
                <Text>{name}</Text>
            </HStack>
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
    isSynced: PropTypes.bool,
}
