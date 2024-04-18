import {
    Card,
    CardHeader,
    CardBody,
    Flex,
    Text,
    Button,
    Spacer,
    Progress,
    Stack,
    StackDivider,
} from '@chakra-ui/react'
import { NumberInput } from '../components/CustomInputs'
import { PropTypes } from 'prop-types'
import { useContext, useMemo, useState, useEffect } from 'react'
import MidiContext from './MidiProvider'
import SkeletonLoader from './SkeletonLoader'

export function CcCard({ name, channel, cc, value, onChange }) {
    const { ccMessages, config } = useContext(MidiContext)
    const handleSelect = (value) => {
        onChange(value)
    }

    const updatedValue = useMemo(() => {
        const ccMessage = ccMessages.find((message) => message.index === cc)
        return ccMessage ? Math.round(ccMessage.value * 100) : value
    }, [ccMessages, cc, value])

    return (
        <Card
            as={Flex}
            justifyContent="space-between"
            borderRadius={20}
            borderColor="gray.100"
            borderBottomWidth={4}
            height="250px"
        >
            <CardHeader pt={4} pb={1}>
                <Flex
                    justifyContent="space-between"
                    alignItems="center"
                    alignContent="center"
                    wrap="wrap"
                >
                    <Button
                        colorScheme="primary"
                        flexGrow="0"
                        size="lg"
                        px={3}
                        borderRadius="full"
                    >
                        {name}
                    </Button>
                    <Spacer />
                    <Progress
                        flexGrow="5"
                        colorScheme="primary"
                        height={3}
                        value={updatedValue}
                    />
                </Flex>
            </CardHeader>
            <Stack divider={<StackDivider />} spacing="3" pb={2}>
                <CardBody py={3}>
                    <Flex justifyContent="space-between" align="center">
                        <Text mr="10px">CH</Text>
                        <SkeletonLoader>
                            <NumberInput
                                def={channel}
                                min={1}
                                max={16}
                                value={channel} // Set the value of the input component
                                onSelect={handleSelect}
                            />
                        </SkeletonLoader>
                    </Flex>
                </CardBody>
                <CardBody py={3}>
                    <Flex justifyContent="space-between" align="center">
                        <Text mr="4px">CC</Text>
                        <SkeletonLoader>
                            <NumberInput
                                def={cc}
                                min={0}
                                max={127}
                                value={cc} // Set the value of the input component
                                onSelect={handleSelect}
                            />
                        </SkeletonLoader>
                    </Flex>
                </CardBody>
            </Stack>
        </Card>
    )
}

CcCard.propTypes = {
    name: PropTypes.string,
    channel: PropTypes.number,
    cc: PropTypes.number,
    value: PropTypes.number,
}
