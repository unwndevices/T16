import {
    Card,
    CardBody,
    CardHeader,
    HStack,
    IconButton,
    Text,
    VStack,
} from '@chakra-ui/react'
import { PropTypes } from 'prop-types'
import SkeletonLoader from './SkeletonLoader'
import { MdArrowLeft, MdArrowRight } from 'react-icons/md'

export default function KeyCard({ name, output, isRoot }) {
    return (
        <Card borderRadius={20} borderColor="gray.100" borderBottomWidth={4}>
            <CardHeader
                borderTopRadius={20}
                textAlign="left"
                backgroundColor={isRoot ? 'primary.800' : 'primary.400'}
                color="white"
                py={3}
            >
                {name}
            </CardHeader>
            <CardBody textAlign="center" fontWeight="600">
                <SkeletonLoader>{output}</SkeletonLoader>
            </CardBody>
        </Card>
    )
}

export function EditableKeyCard({
    name,
    index,
    output,
    isRoot,
    onDecrement,
    onIncrement,
}) {
    return (
        <Card borderRadius={20} borderColor="gray.100" borderBottomWidth={4}>
            <CardHeader
                borderTopRadius={20}
                textAlign="left"
                backgroundColor={isRoot ? 'primary.800' : 'primary.400'}
                color="white"
                py={3}
            >
                {name}
            </CardHeader>
            <CardBody textAlign="center" fontWeight="600" p={3}>
                <HStack justifyContent="space-between">
                    <IconButton
                        onClick={onDecrement}
                        icon={<MdArrowLeft />}
                        size="lg"
                    />
                    <SkeletonLoader>
                        <VStack gap={0}>
                            <Text fontSize="lg">{index}</Text>
                            <Text fontSize="sm" color="gray.500">
                                {output}
                            </Text>
                        </VStack>
                    </SkeletonLoader>
                    <IconButton
                        onClick={onIncrement}
                        icon={<MdArrowRight />}
                        size="lg"
                    />
                </HStack>
            </CardBody>
        </Card>
    )
}

KeyCard.propTypes = {
    name: PropTypes.string,
    output: PropTypes.string,
    active: PropTypes.bool,
    isRoot: PropTypes.bool,
}

EditableKeyCard.propTypes = {
    name: PropTypes.string,
    output: PropTypes.string,
    index: PropTypes.number,
    active: PropTypes.bool,
    isRoot: PropTypes.bool,
    onDecrement: PropTypes.func,
    onIncrement: PropTypes.func,
}
