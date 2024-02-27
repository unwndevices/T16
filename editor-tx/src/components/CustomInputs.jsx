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
import { useState } from 'react'

export function NumberInput({ def = 0, min = 0, max = 10 }) {
    const { getInputProps, getIncrementButtonProps, getDecrementButtonProps } =
        useNumberInput({
            step: 1,
            defaultValue: def,
            value: def,
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
export function AliasNumberInput({ aliases }) {
    const [currentAliasIndex, setCurrentAliasIndex] = useState(0)
    const [currentAlias, setCurrentAlias] = useState(aliases[currentAliasIndex])
    const { getIncrementButtonProps, getDecrementButtonProps } = useNumberInput(
        {
            step: 1,
            defaultValue: 0,
            min: 0,
            max: aliases.length - 1,
            onChange: (index) => {
                setCurrentAliasIndex(index)
                setCurrentAlias(aliases[index])
            },
        }
    )

    const inc = getIncrementButtonProps({
        onClick: () => {
            setCurrentAliasIndex(
                (prevIndex) => (prevIndex + 1) % aliases.length
            )
        },
    })
    const dec = getDecrementButtonProps({
        onClick: () => {
            setCurrentAliasIndex(
                (prevIndex) => (prevIndex - 1 + aliases.length) % aliases.length
            )
        },
    })

    return (
        <HStack maxW="320px">
            <Button {...dec}>-</Button>
            <Input
                maxW={20}
                type="text"
                readOnly="readonly"
                value={currentAlias}
            />
            <Button {...inc}>+</Button>
        </HStack>
    )
}

AliasNumberInput.propTypes = {
    aliases: PropTypes.array,
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
