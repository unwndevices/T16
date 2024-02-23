import PropTypes from 'prop-types'
import {
    Box,
    Button,
    Input,
    HStack,
    useNumberInput,
    Text,
    VStack,
} from '@chakra-ui/react'

export function NumberInput({ def = 0, min = 0, max = 10 }) {
    const { getInputProps, getIncrementButtonProps, getDecrementButtonProps } =
        useNumberInput({
            step: 1,
            defaultValue: def,
            min: min,
            max: max,
        })

    const inc = getIncrementButtonProps()
    const dec = getDecrementButtonProps()
    const input = getInputProps()

    return (
        <HStack maxW="320px">
            <Button {...dec}>-</Button>
            <Input maxW={20} {...input} />
            <Button {...inc}>+</Button>
        </HStack>
    )
}

NumberInput.propTypes = {
    label: PropTypes.string,
    def: PropTypes.number,
    min: PropTypes.number,
    max: PropTypes.number,
}

function InputControl({ label, ...props }) {
    const { getInputProps, getIncrementButtonProps, getDecrementButtonProps } =
        useNumberInput(props)

    const inc = getIncrementButtonProps()
    const dec = getDecrementButtonProps()
    const input = getInputProps()

    return (
        <VStack>
            <Text fontSize="sm" fontStyle="italic" pt={2}>
                {label}
            </Text>
            <HStack maxW="320px">
                <Button {...dec}>-</Button>
                <Input maxW="4em" textAlign="center" {...input} />
                <Button {...inc}>+</Button>
            </HStack>
        </VStack>
    )
}

InputControl.propTypes = {
    label: PropTypes.string,
    min: PropTypes.number,
    max: PropTypes.number,
}

export function CCInput({ label = 'Default', def = 0 }) {
    return (
        <Box
            h="200px"
            borderWidth="1px"
            borderRadius="lg"
            p={2}
            textAlign="left"
        >
            <Text fontSize="lg" fontWeight="600">
                {label}
            </Text>
            <InputControl
                label="Channel"
                defaultValue={1}
                step={1}
                min={1}
                max={16}
            />
            <InputControl label="CC" defaultValue={def} step={1} />
        </Box>
    )
}

CCInput.propTypes = {
    label: PropTypes.string,
    def: PropTypes.number,
}
