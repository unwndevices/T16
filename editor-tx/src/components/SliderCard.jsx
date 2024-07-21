import {
    Flex,
    Slider,
    SliderTrack,
    SliderFilledTrack,
    SliderThumb,
    Text,
    Input,
    HStack,
    Circle,
} from '@chakra-ui/react'
import { PropTypes } from 'prop-types'
import SkeletonLoader from './SkeletonLoader'

export function SliderCard({
    name,
    value,
    min = 0,
    max = 127,
    onChange,
    isSynced,
}) {
    const handleSliderChange = (newValue) => {
        onChange(newValue)
    }

    const getDotColor = () => {
        return isSynced ? 'green.500' : 'red.500'
    }

    const handleInputChange = (event) => {
        const inputValue = parseInt(event.target.value, 10)
        if (!isNaN(inputValue) && inputValue >= min && inputValue <= max) {
            onChange(inputValue)
        }
    }

    return (
        <SkeletonLoader>
            <Flex justifyContent="space-between" alignItems="center">
                <HStack spacing={2}>
                    <Circle size="10px" bg={getDotColor()} />
                    <Text minW="150px">{name}</Text>
                </HStack>
                <Slider
                    aria-label={name}
                    min={min}
                    max={max}
                    value={value}
                    colorScheme="primary"
                    onChange={handleSliderChange}
                >
                    <SliderTrack height={5} rounded="full">
                        <SliderFilledTrack />
                    </SliderTrack>
                    <SliderThumb boxSize={5} backgroundColor={'primary.300'} />
                </Slider>
                <Input
                    type="number"
                    value={value}
                    maxW="75px"
                    ml="20px"
                    onChange={handleInputChange}
                />
            </Flex>
        </SkeletonLoader>
    )
}

SliderCard.propTypes = {
    name: PropTypes.string,
    value: PropTypes.number,
    max: PropTypes.number,
    min: PropTypes.number,
    onChange: PropTypes.func,
    isSynced: PropTypes.bool,
}
