import PropTypes from 'prop-types'
import { Button, Input, HStack, useNumberInput } from '@chakra-ui/react'
import { useEffect, useState } from 'react'

export function NumberInput({ value, min = 0, max = 10, onSelect }) {
    const { getInputProps, getIncrementButtonProps, getDecrementButtonProps } =
        useNumberInput({
            step: 1,
            min: min,
            max: max,
            value: value, // Use the value prop instead of defaultValue
            onChange: (valueAsString, valueAsNumber) => {
                onSelect(valueAsNumber)
            },
        })

    const inc = getIncrementButtonProps()
    const dec = getDecrementButtonProps()
    const input = getInputProps()

    return (
        <HStack maxW="320px">
            <Button colorScheme="primary" {...dec}>
                -
            </Button>
            <Input
                backgroundColor="white"
                textAlign="right"
                maxW={20}
                {...input}
            />
            <Button colorScheme="primary" {...inc}>
                +
            </Button>
        </HStack>
    )
}

NumberInput.propTypes = {
    value: PropTypes.number, // Update the propType for value
    min: PropTypes.number,
    max: PropTypes.number,
    onSelect: PropTypes.func,
}

export function AliasNumberInput({ aliases, value, onSelect }) {
    const [currentAliasIndex, setCurrentAliasIndex] = useState(0)
    const [currentAlias, setCurrentAlias] = useState(aliases[currentAliasIndex])
    const { getIncrementButtonProps, getDecrementButtonProps } = useNumberInput(
        {
            step: 1,
            min: 0,
            max: aliases.length - 1,
            onChange: (valueString, valueNumber) => {
                setCurrentAliasIndex(valueNumber)
                setCurrentAlias(aliases[valueNumber])
                onSelect(valueNumber)
            },
        }
    )

    // Update currentAliasIndex and currentAlias when value changes
    useEffect(() => {
        if (value >= 0 && value < aliases.length) {
            setCurrentAliasIndex(value)
            setCurrentAlias(aliases[value])
        }
    }, [value, aliases])

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
            <Button colorScheme="primary" {...dec}>
                -
            </Button>
            <Input
                maxW={20}
                type="text"
                readOnly="readonly"
                value={currentAlias}
                backgroundColor="white"
            />
            <Button colorScheme="primary" {...inc}>
                +
            </Button>
        </HStack>
    )
}

AliasNumberInput.propTypes = {
    aliases: PropTypes.array,
    value: PropTypes.number,
    onSelect: PropTypes.func,
}
