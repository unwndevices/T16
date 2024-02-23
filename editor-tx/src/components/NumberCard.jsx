import {
    Card,
    CardHeader,
    CardBody,
    Flex,
    Button,
    Stack,
    StackDivider,
    Select,
    Spacer,
} from '@chakra-ui/react'

import { PropTypes } from 'prop-types'
import { NumberInput } from './CustomInputs'

export function NumberCard({ name, min = 0, max = 127 }) {
    return (
        <Card borderRadius={20} borderColor="gray.100" borderBottomWidth={4}>
            <Flex flexDirection="column" height="150px">
                <CardHeader pt={4} pb={1}>
                    <Flex
                        justifyContent="space-between"
                        alignItems="center"
                        alignContent="center"
                        wrap="wrap"
                    >
                        <Button
                            flexGrow="0"
                            colorScheme="green"
                            size="md"
                            borderRadius="full"
                        >
                            {name}
                        </Button>
                    </Flex>
                </CardHeader>
                <Spacer />
                <Stack divider={<StackDivider />} spacing="0">
                    <CardBody py={3}>
                        <Flex justifyContent="space-between" align="center">
                            <NumberInput min={min} max={max} def={1} />
                        </Flex>
                    </CardBody>
                </Stack>
            </Flex>
        </Card>
    )
}

NumberCard.propTypes = {
    name: PropTypes.string,
    min: PropTypes.number,
    max: PropTypes.number,
}
