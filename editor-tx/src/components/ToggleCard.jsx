import {
    Box,
    Circle,
    Flex,
    HStack,
    Switch,
    Text,
    Tooltip,
} from '@chakra-ui/react'
import { PropTypes } from 'prop-types'
import SkeletonLoader from './SkeletonLoader'

export function ToggleCard({
    name,
    id,
    value,
    onChange,
    isSynced,
    isDisabled = false,
    tooltipText,
}) {
    const handleToggle = () => {
        const newValue = value === 1 ? 0 : 1
        onChange(newValue)
    }

    const getDotColor = () => {
        return isSynced ? 'green.500' : 'red.500'
    }

    return (
        <Flex justifyContent="space-between" alignItems="center">
            <Box minW="8vw">
                <HStack spacing={2}>
                    <Circle size="10px" bg={getDotColor()} />
                    <Tooltip
                        hasArrow
                        label={tooltipText}
                        aria-label={`Tooltip for ${name}`}
                        borderRadius="md"
                        isDisabled={!tooltipText}
                    >
                        <Text pr={4}>{name}</Text>
                    </Tooltip>
                </HStack>
            </Box>
            <SkeletonLoader>
                <Switch
                    id={id}
                    size="lg"
                    colorScheme="primary"
                    isDisabled={isDisabled}
                    isChecked={value === 1}
                    onChange={handleToggle}
                />
            </SkeletonLoader>
        </Flex>
    )
}

ToggleCard.propTypes = {
    name: PropTypes.string,
    id: PropTypes.string,
    value: PropTypes.number.isRequired,
    onChange: PropTypes.func.isRequired,
    isSynced: PropTypes.bool,
    isDisabled: PropTypes.bool,
    tooltipText: PropTypes.string,
}
