import {
    Flex,
    Slider,
    SliderTrack,
    SliderFilledTrack,
    SliderThumb,
    Text,
    Input,
} from '@chakra-ui/react'
import { PropTypes } from 'prop-types'
import SkeletonLoader from './SkeletonLoader'

export function SliderCard({ name, value, min = 0, max = 127, onChange }) {
    const handleSliderChange = (newValue) => {
        onChange(newValue)
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
                <Text minW="150px">{name}</Text>
                <Input
                    type="number"
                    value={value}
                    maxW="75px"
                    mr="20px"
                    onChange={handleInputChange}
                />
                <Slider
                    aria-label={name}
                    min={min}
                    max={max}
                    defaultValue={value}
                    colorScheme="primary"
                    onChange={handleSliderChange}
                >
                    <SliderTrack height={5} rounded="full">
                        <SliderFilledTrack />
                    </SliderTrack>
                    <SliderThumb boxSize={5} backgroundColor={'primary.300'} />
                </Slider>
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
}
