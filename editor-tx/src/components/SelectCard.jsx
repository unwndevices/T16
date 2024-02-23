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

export function SelectCard({ name, entries }) {
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
                            <Select size="lg">
                                {entries.map((entry) => (
                                    <option key={entry}>{entry}</option>
                                ))}
                            </Select>
                        </Flex>
                    </CardBody>
                </Stack>
            </Flex>
        </Card>
    )
}

SelectCard.propTypes = {
    name: PropTypes.string,
    entries: PropTypes.array,
}
