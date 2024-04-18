import { Box, Flex, Switch, Text } from '@chakra-ui/react'
import { PropTypes } from 'prop-types'
import SkeletonLoader from './SkeletonLoader'

export function ToggleCard({ name, id, value, onChange }) {
    const handleToggle = () => {
        const newValue = value === 1 ? 0 : 1
        onChange(newValue)
    }

    return (
        <Flex justifyContent="space-between" alignItems="center">
            <Box minW="8vw">
                <Text pr={4}>{name}</Text>
            </Box>
            <SkeletonLoader>
                <Switch
                    id={id}
                    size="lg"
                    colorScheme="primary"
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
}
